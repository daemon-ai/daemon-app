// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/mirror_session_store.h"

#include "daemon/transcript_projection.h"
#include "mirror/ingestor.h"
#include "mirror/store.h"

#include <algorithm>
#include <QDateTime>
#include <QHash>
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
                                       persistence::ISessionStore* legacy, QObject* parent)
    : persistence::ISessionStore(parent), m_mirror(store), m_ingestor(ingestor), m_legacy(legacy) {
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
    if (m_legacy != nullptr) {
        // Dual-write window: the delegated reads (transcript content) refresh through the legacy
        // signal; the wire round-trips (create / meta rejection) surface through its relays.
        connect(m_legacy, &persistence::ISessionStore::changed, this,
                &persistence::ISessionStore::changed);
        connect(m_legacy, &persistence::ISessionStore::sessionCreated, this,
                &persistence::ISessionStore::sessionCreated);
        connect(m_legacy, &persistence::ISessionStore::metaUpdateFailed, this,
                &persistence::ISessionStore::metaUpdateFailed);
    }
    rebuildFromSnapshot();
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
    // window (fed by the engine dual-write via MirrorTranscriptSink), byte-identical to the legacy
    // cache projection (S1-S9 parity green). The legacy delegation is retired here.
    return mirrorContent(id);
}

void MirrorSessionStore::setContent(const domain::SessionId& id, const QString& markdown) {
    if (m_legacy != nullptr) {
        m_legacy->setContent(id, markdown);
    }
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

domain::SessionId MirrorSessionStore::newSession(const domain::UnitId& unitId) {
    return m_legacy != nullptr ? m_legacy->newSession(unitId) : domain::SessionId();
}

void MirrorSessionStore::setArchived(const domain::SessionId& id, bool archived) {
    if (m_legacy != nullptr) {
        m_legacy->setArchived(id, archived);
    }
}

void MirrorSessionStore::renameSession(const domain::SessionId& id, const QString& title) {
    if (m_legacy != nullptr) {
        m_legacy->renameSession(id, title);
    }
}

void MirrorSessionStore::deleteSession(const domain::SessionId& id) {
    if (m_legacy != nullptr) {
        m_legacy->deleteSession(id);
    }
}

void MirrorSessionStore::setPinned(const domain::SessionId& id, bool pinned) {
    if (m_legacy != nullptr) {
        m_legacy->setPinned(id, pinned);
    }
}

void MirrorSessionStore::moveSession(const domain::SessionId& id, int delta) {
    if (m_legacy != nullptr) {
        m_legacy->moveSession(id, delta);
    }
}

void MirrorSessionStore::requestNewSession(const QString& profileId) {
    if (m_legacy != nullptr) {
        m_legacy->requestNewSession(profileId);
    }
}

domain::UnitId MirrorSessionStore::createUnit(const domain::UnitId& parentId,
                                              domain::UnitKind kind) {
    return m_legacy != nullptr ? m_legacy->createUnit(parentId, kind) : domain::UnitId();
}

int MirrorSessionStore::createTag(const QString& name, const QString& color) {
    return m_legacy != nullptr ? m_legacy->createTag(name, color) : -1;
}

void MirrorSessionStore::refreshSessionsForProfile(const QString& profileId) {
    // Dual-write: the legacy repo refreshes its cache (unported consumers), the ingestor fetches
    // the same scoped subset into the mirror (ported consumers). Both land additively.
    if (m_legacy != nullptr) {
        m_legacy->refreshSessionsForProfile(profileId);
    }
    if (m_ingestor != nullptr) {
        m_ingestor->refetchSessionsForProfile(profileId);
    }
}

void MirrorSessionStore::refreshArchivedSessions() {
    if (m_legacy != nullptr) {
        m_legacy->refreshArchivedSessions();
    }
    if (m_ingestor != nullptr) {
        m_ingestor->refetchArchivedSessions();
    }
}

void MirrorSessionStore::refreshSessionsForTransport(const QString& transportId) {
    if (m_legacy != nullptr) {
        m_legacy->refreshSessionsForTransport(transportId);
    }
    if (m_ingestor != nullptr) {
        m_ingestor->refetchSessionsForTransport(transportId);
    }
}

} // namespace daemonapp::daemon
