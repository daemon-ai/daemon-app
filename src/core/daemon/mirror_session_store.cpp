// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/mirror_session_store.h"

#include "daemon/repositories.h"
#include "daemon/transcript_projection.h"
#include "mirror/ingestor.h"
#include "mirror/store.h"
#include "outbox.h"
#include "verb_class.h"

#include <algorithm>
#include <QDateTime>
#include <QHash>
#include <QJsonDocument>
#include <QSet>
#include <vector>

namespace daemonapp::daemon {
namespace {

// The journal consumer name for the watermark registration (§4.4).
constexpr auto kConsumer = "mirror_session_store";

// Canonical mirror strings (entities_map.cpp) -> domain enums. The same mapping the legacy
// CachedSessionStore applies to its cache columns, so a ported consumer sees identical values.
domain::SessionState stateFromString(const QString& s) {
    if (s == QStringLiteral("Active")) {
        return domain::SessionState::Active;
    }
    if (s == QStringLiteral("Suspended")) {
        return domain::SessionState::Suspended;
    }
    if (s == QStringLiteral("Ready")) {
        return domain::SessionState::Ready;
    }
    if (s == QStringLiteral("Completed")) {
        return domain::SessionState::Completed;
    }
    return domain::SessionState::Unknown;
}

domain::Lifecycle lifecycleFromString(const QString& s) {
    return s == QStringLiteral("Live") ? domain::Lifecycle::Live : domain::Lifecycle::Durable;
}

domain::SessionRole roleFromString(const QString& s) {
    if (s == QStringLiteral("ManagedChild")) {
        return domain::SessionRole::ManagedChild;
    }
    if (s == QStringLiteral("EphemeralSubagent")) {
        return domain::SessionRole::EphemeralSubagent;
    }
    return domain::SessionRole::Primary;
}

// Project ONE generated mirror row onto the interface DTO (the 6→1 read: every consumer sees this
// projection of mirror::Session — no per-surface shape). `content`/`tagIds`/`unitId` stay empty:
// the documented M4 degradations (transcript re-homing is sub-gate 6; tags are a client-local
// sidecar; the legacy daemon store never set unitId either).
domain::Session projectRow(const mirror::Session& s) {
    domain::Session out;
    out.sessionId = domain::SessionId(s.session);
    out.title = s.title;
    out.boundProfile = domain::ProfileRef(s.bound_profile);
    out.state = stateFromString(s.state);
    out.lifecycle = lifecycleFromString(s.lifecycle);
    out.role = roleFromString(s.role);
    if (!s.parent.isEmpty()) {
        out.parent = domain::SessionId(s.parent);
    }
    out.isPinned = s.pinned;
    out.isArchived = s.archived;
    out.lastActivityMs = static_cast<qint64>(s.last_activity_ms);
    if (s.last_activity_ms > 0) {
        out.modified = QDateTime::fromMSecsSinceEpoch(out.lastActivityMs);
        out.created = out.modified;
    }
    return out;
}

} // namespace

MirrorSessionStore::MirrorSessionStore(mirror::Store* store, mirror::Ingestor* ingestor,
                                       SessionRepository* sessions, QObject* parent)
    : persistence::ISessionStore(parent), m_mirror(store), m_ingestor(ingestor),
      m_sessions(sessions) {
    if (m_mirror != nullptr) {
        m_mirror->journal().registerConsumer(QLatin1String(kConsumer));
        m_watermark = m_mirror->journal().headRev();
        m_mirror->journal().advanceWatermark(QLatin1String(kConsumer), m_watermark);
        connect(m_mirror, &mirror::Store::committed, this, &MirrorSessionStore::onCommitted);
    }
    if (m_ingestor != nullptr) {
        // Long-lived shared roster projection: declare visibility so staleness-driven fetches run
        // at visible-surface priority (§5.8), and record the node-resolved ByTransport sets.
        m_ingestor->beginObserving(QStringLiteral("sessions"));
        m_ingestor->beginObserving(QStringLiteral("fleet"));
        connect(m_ingestor, &mirror::Ingestor::transportSessionsResolved, this,
                [this](const QString& transportId, const QSet<QString>& ids) {
                    m_transportSessions.insert(transportId, ids);
                    rebuildFromSnapshot();
                    emit changed();
                });
    }
    if (m_sessions != nullptr) {
        // The direct create seam (§7): the node mints the id; the repo's reply relays as
        // ISessionStore::sessionCreated so the initiating surface adopts it (open + auto-select).
        connect(m_sessions, &SessionRepository::sessionCreated, this,
                &persistence::ISessionStore::sessionCreated);
    }
    rebuildFromSnapshot();
}

void MirrorSessionStore::onNodeSessionCreated(const QString& sessionId, const QString& profileId) {
    emit sessionCreated(sessionId, profileId);
}

void MirrorSessionStore::setOutbox(mirror::Outbox* outbox) {
    if (m_outbox == outbox) {
        return;
    }
    if (m_outbox != nullptr) {
        disconnect(m_outbox, nullptr, this, nullptr);
    }
    m_outbox = outbox;
    if (m_outbox == nullptr) {
        return;
    }
    // §6.5: a session-meta lane rejection reaches the initiating surface through the verb seam's
    // typed failure signal — the SAME metaUpdateFailed relay the GUI toast + TUI notification
    // already bind. The rejected row additionally stays visible in the outbox lens (durable
    // affordance); the lane pauses until retry/discard.
    connect(m_outbox, &mirror::Outbox::opChanged, this, [this](const QString& opId) {
        const mirror::PendingOp op = m_outbox->op(opId);
        if (op.state != mirror::OpState::Rejected ||
            mirror::verbClass(op.verb) != mirror::VerbClass::SessionMeta) {
            return;
        }
        const qsizetype sep = op.lane.indexOf(QChar(0x1f));
        const QString session = sep >= 0 ? op.lane.mid(sep + 1) : QString();
        emit metaUpdateFailed(session, op.lastError);
    });
}

void MirrorSessionStore::enqueueSessionMeta(const domain::SessionId& id, const QJsonObject& patch,
                                            const QString& display) {
    QJsonObject args = patch;
    args.insert(QStringLiteral("session"), id.toString());
    const QByteArray payload = QJsonDocument(args).toJson(QJsonDocument::Compact);
    // Durable BEFORE any send (§6.6); lane scope = the session (§6.3/§6.4 "per session for
    // session-meta"). The drain nudge is the user's own action (the send-now tap, §6.8's manual
    // path) — offline it simply holds: the gate keeps the op pending until connected.
    m_outbox->enqueue(QStringLiteral("SessionUpdateMeta"), id.toString(), payload, display);
    m_outbox->drain();
}

void MirrorSessionStore::onCommitted() {
    if (m_mirror == nullptr) {
        return;
    }
    // Re-derive only when session/fleet deltas landed above the watermark (§8.1 discipline —
    // chat pages and routing churn must not thrash the roster consumers). G2 (M5): a
    // TranscriptBlock delta also fires changed() so the flipped content() read (now mirror-served)
    // refreshes on an engine transcript write / offline reload — without a session-row rebuild.
    const auto sessionDeltas = m_mirror->journal().since(m_watermark, mirror::EntityKind::Session);
    const auto fleetDeltas = m_mirror->journal().since(m_watermark, mirror::EntityKind::FleetUnit);
    const auto transcriptDeltas =
        m_mirror->journal().since(m_watermark, mirror::EntityKind::TranscriptBlock);
    m_watermark = m_mirror->journal().headRev();
    m_mirror->journal().advanceWatermark(QLatin1String(kConsumer), m_watermark);
    if (sessionDeltas.empty() && fleetDeltas.empty() && transcriptDeltas.empty()) {
        return;
    }
    if (!sessionDeltas.empty() || !fleetDeltas.empty()) {
        rebuildFromSnapshot();
    }
    emit changed();
}

void MirrorSessionStore::rebuildFromSnapshot() {
    m_rows.clear();
    if (m_mirror == nullptr) {
        return;
    }
    for (const mirror::Session& s : m_mirror->snapshot().sessions) {
        m_rows.push_back(projectRow(s));
    }
    // Deterministic projection order: pinned first, then last-activity desc, then id (the legacy
    // cache's ORDER BY updated_at_ms DESC was client-receive-time, arbitrary within a refresh).
    std::ranges::sort(m_rows, [](const domain::Session& a, const domain::Session& b) {
        if (a.isPinned != b.isPinned) {
            return a.isPinned;
        }
        if (a.lastActivityMs != b.lastActivityMs) {
            return a.lastActivityMs > b.lastActivityMs;
        }
        return a.sessionId.toString() < b.sessionId.toString();
    });
    for (int i = 0; i < m_rows.size(); ++i) {
        m_rows[i].id = i; // the demoted local int handle (list index), like the legacy stores
    }
}

bool MirrorSessionStore::matchesScope(const domain::Session& s,
                                      const domain::ListScope& scope) const {
    switch (scope.type) {
    case domain::NodeType::Archived:
        return s.isArchived;
    case domain::NodeType::AllSessions:
        return !s.isArchived;
    case domain::NodeType::Agent:
        // SessionScope::ByProfile: bound to the agent's profile, not archived (node semantics).
        return !s.isArchived && !scope.lensKey.isEmpty() &&
               s.boundProfile.toString() == scope.lensKey;
    case domain::NodeType::ByTransport:
        // B4: membership is NODE-resolved; project the recorded id set, never a re-derivation.
        return !s.isArchived &&
               m_transportSessions.value(scope.lensKey).contains(s.sessionId.toString());
    default:
        // Daemon sessions are flat (no unit tree / tags) — same as the legacy daemon store.
        return false;
    }
}

QList<domain::Session> MirrorSessionStore::sessions(const domain::ListScope& scope) const {
    QList<domain::Session> out;
    for (const domain::Session& s : m_rows) {
        if (matchesScope(s, scope)) {
            out.push_back(s);
        }
    }
    return out;
}

int MirrorSessionStore::sessionCount(const domain::ListScope& scope) const {
    int count = 0;
    for (const domain::Session& s : m_rows) {
        if (matchesScope(s, scope)) {
            ++count;
        }
    }
    return count;
}

QString MirrorSessionStore::title(const domain::SessionId& id) const {
    if (m_mirror == nullptr || id.isEmpty()) {
        return {};
    }
    const mirror::Session* s =
        m_mirror->snapshot().sessions.find(mirror::SessionKey{id.toString()});
    return s != nullptr ? s->title : QString();
}

bool MirrorSessionStore::isPinned(const domain::SessionId& id) const {
    if (m_mirror == nullptr || id.isEmpty()) {
        return false;
    }
    const mirror::Session* s =
        m_mirror->snapshot().sessions.find(mirror::SessionKey{id.toString()});
    return s != nullptr && s->pinned;
}

QList<domain::UnitNode> MirrorSessionStore::unitChildren(const domain::UnitId&) const {
    return {};
}

domain::UnitNode MirrorSessionStore::unit(const domain::UnitId&) const {
    return {};
}

QList<domain::Tag> MirrorSessionStore::tags() const {
    return {};
}

QList<domain::Participant> MirrorSessionStore::participants() const {
    return {};
}

QString MirrorSessionStore::content(const domain::SessionId& id) const {
    // G2 (M5): the facade flip. Transcript content now projects the mirror `w_transcript_blocks`
    // window (fed by the engine via MirrorTranscriptSink), byte-identical to the legacy
    // cache projection (S1-S9 parity green).
    return mirrorContent(id);
}

void MirrorSessionStore::setContent(const domain::SessionId&, const QString&) {
    // The engine owns transcript writes (single writer through the sink); a client-side content
    // write has no wire verb and no mirror write API (§14.2). No-op on both modes — the mock
    // aligns to daemon (the pre-AD mock-only local write was a shape fork; A8 precedent).
}

QString MirrorSessionStore::mirrorContent(const domain::SessionId& id) const {
    if (m_mirror == nullptr || id.isEmpty()) {
        return {};
    }
    // The window items are stored cursor-ordered (by seq), so a straight copy is already in the
    // seq order the projection expects.
    const auto& windows = m_mirror->snapshot().transcripts;
    const mirror::TranscriptBlockScope scope{id.toString()};
    std::vector<mirror::TranscriptBlock> blocks;
    if (windows.count(scope) != 0U) {
        const mirror::Window<mirror::TranscriptBlock>& win = windows[scope];
        blocks.reserve(win.items.size());
        for (const auto& item : win.items) {
            blocks.push_back(item.get());
        }
    }
    return projectTranscriptBlocks(blocks);
}

domain::SessionId MirrorSessionStore::newSession(const domain::UnitId&) {
    // Session lifecycle is node-owned: creation flows through requestNewSession (the async
    // direct SessionCreate seam) + the sessionCreated relay — nothing is client-minted. Both
    // modes answer empty (the mock aligns to daemon; the synchronous local mint was a
    // mock/daemon shape fork — A8 precedent).
    return {};
}

// --- session-meta mutations (AD, §6.4): the durable outbox lane -------------------------------
// The node owns pin/archive/title; each mutation is a SessionUpdateMeta intent enqueued to the
// per-session `session-meta` lane (offline-durable, §6.6; op_id dedup makes replay safe, §10.3).
// The mirror row changes ONLY when the node's authoritative read lands (SessionMetaChanged →
// SessionGet / roster delta) — never an optimistic local write (§14.7). Without an outbox (bare
// test stacks) they are no-ops: there is no legacy fallback path anymore.

void MirrorSessionStore::setArchived(const domain::SessionId& id, bool archived) {
    if (m_outbox != nullptr && !id.isEmpty()) {
        enqueueSessionMeta(
            id, QJsonObject{{QStringLiteral("archived"), archived}},
            QStringLiteral(u"%1 \u2014 %2")
                .arg(archived ? QStringLiteral("archive") : QStringLiteral("restore"), title(id)));
    }
}

void MirrorSessionStore::renameSession(const domain::SessionId& id, const QString& title) {
    if (m_outbox != nullptr && !id.isEmpty()) {
        enqueueSessionMeta(id, QJsonObject{{QStringLiteral("title"), title}},
                           QStringLiteral(u"rename \u2014 %1").arg(title));
    }
}

void MirrorSessionStore::deleteSession(const domain::SessionId&) {
    // No session-delete wire verb exists at v39 — the daemon store always no-opped here; the
    // mock now aligns (the local delete was a shape fork). Archive is the supported removal.
}

void MirrorSessionStore::setPinned(const domain::SessionId& id, bool pinned) {
    if (m_outbox != nullptr && !id.isEmpty()) {
        enqueueSessionMeta(
            id, QJsonObject{{QStringLiteral("pinned"), pinned}},
            QStringLiteral(u"%1 \u2014 %2")
                .arg(pinned ? QStringLiteral("pin") : QStringLiteral("unpin"), title(id)));
    }
}

void MirrorSessionStore::moveSession(const domain::SessionId&, int) {
    // The projection order is node-derived (pinned, last-activity, id) — there is no manual
    // reorder verb. Always a no-op, both modes (daemon parity; the mock local reorder was a
    // shape fork).
}

void MirrorSessionStore::requestNewSession(const QString& profileId) {
    // The DIRECT create seam (§7): daemon → SessionCreate through the repository (the node mints
    // the id; the ctor relays sessionCreated); mock → the composition answers via
    // createRequested → the scenario host scripts the outcome + seeds the row.
    if (m_sessions != nullptr) {
        m_sessions->createSession(profileId);
        return;
    }
    emit createRequested(profileId);
}

domain::UnitId MirrorSessionStore::createUnit(const domain::UnitId&, domain::UnitKind) {
    // Units are node-owned supervision structure; no client-side create verb (daemon parity).
    return {};
}

int MirrorSessionStore::createTag(const QString&, const QString&) {
    // Tags are a client-local sidecar concern (§3.4) with no mirror vertical yet; the daemon
    // store never supported them. No-op, both modes.
    return -1;
}

void MirrorSessionStore::refreshSessionsForProfile(const QString& profileId) {
    if (m_ingestor != nullptr) {
        m_ingestor->refetchSessionsForProfile(profileId);
    }
}

void MirrorSessionStore::refreshArchivedSessions() {
    if (m_ingestor != nullptr) {
        m_ingestor->refetchArchivedSessions();
    }
}

void MirrorSessionStore::refreshSessionsForTransport(const QString& transportId) {
    if (m_ingestor != nullptr) {
        m_ingestor->refetchSessionsForTransport(transportId);
    }
}

} // namespace daemonapp::daemon
