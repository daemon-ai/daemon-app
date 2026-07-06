// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/cached_session_store.h"

#include "daemon/repositories.h"

#include <algorithm>
#include <optional>
#include <QDateTime>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

namespace daemonapp::daemon {
namespace {

// Map the cache's string columns (decoded from the wire by NodeApiCodec) back to the domain enums,
// so the projected domain::Session mirrors SessionInfo field-for-field rather than dropping them.
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

domain::UnitKind unitKindFromString(const QString& s) {
    if (s == QStringLiteral("Host")) {
        return domain::UnitKind::Host;
    }
    if (s == QStringLiteral("Orchestrator")) {
        return domain::UnitKind::Orchestrator;
    }
    return domain::UnitKind::Engine;
}

domain::UnitState unitStateFromString(const QString& s) {
    if (s == QStringLiteral("Running")) {
        return domain::UnitState::Running;
    }
    if (s == QStringLiteral("Finished")) {
        return domain::UnitState::Finished;
    }
    return domain::UnitState::Unknown;
}

// Project a cached fleet-unit row (daemon_fleet_units) onto the domain UnitNode the sidebar
// renders.
domain::UnitNode unitNodeFromRow(const CachedFleetUnitRow& r) {
    domain::UnitNode n;
    n.id = domain::UnitId(r.unitId);
    n.parentId = domain::UnitId(r.parentId);
    n.name = r.name;
    n.kind = unitKindFromString(r.kind);
    n.state = unitStateFromString(r.state);
    n.work = r.work;
    n.profile = domain::ProfileRef(r.profileRef);
    n.session = domain::SessionId(r.sessionId);
    n.role = roleFromString(r.role);
    return n;
}

} // namespace

CachedSessionStore::CachedSessionStore(DaemonCacheStore* cache, SessionRepository* sessions,
                                       QObject* parent)
    : persistence::ISessionStore(parent), m_cache(cache), m_sessions(sessions) {
    if (m_sessions != nullptr) {
        connect(m_sessions, &SessionRepository::sessionsRefreshed, this,
                &CachedSessionStore::reload);
        // Node-authoritative create: relay the repo's SessionCreated up as the store-level signal
        // so the controller/sidebar open + auto-select the node-minted id (never a client-minted
        // one).
        connect(m_sessions, &SessionRepository::sessionCreated, this,
                &CachedSessionStore::sessionCreated);
        // Relay a rejected/failed SessionUpdateMeta up as the store-level failure signal so the
        // surface (GUI toast / TUI notification) shows it - a node-owned pin/archive/title write
        // that the node refused must never look applied.
        connect(m_sessions, &SessionRepository::metaUpdateFailed, this,
                &CachedSessionStore::metaUpdateFailed);
        // B4: record the node-resolved ByTransport membership; reload() re-projects the scoped
        // list from the fresh set.
        connect(m_sessions, &SessionRepository::transportSessionsResolved, this,
                [this](const QString& transportId, const QSet<QString>& ids) {
                    m_transportSessions.insert(transportId, ids);
                    reload();
                });
    }
    reload();
}

void CachedSessionStore::reload() {
    m_snapshot = m_cache != nullptr ? m_cache->sessions() : QList<CachedSessionRow>{};
    // Offline roster preview: one grouped query for the latest message per session, so the list
    // shows snippets (and content search works) without a connection.
    m_snippets =
        m_cache != nullptr ? m_cache->latestTranscriptSnippets() : QHash<QString, QString>{};
    emit changed();
}

bool CachedSessionStore::matchesScope(const CachedSessionRow& row,
                                      const domain::ListScope& scope) const {
    switch (scope.type) {
    case domain::NodeType::Archived:
        return row.archived;
    case domain::NodeType::AllSessions:
        return !row.archived;
    case domain::NodeType::Agent:
        // Fleet membership (daemon-supervision-spec §0): an agent's sessions are those bound to its
        // profile. Mirrors the node's SessionScope::ByProfile filter (bound_profile == p, not
        // archived); `lensKey` carries the profile id.
        return !row.archived && !scope.lensKey.isEmpty() && row.profileRef == scope.lensKey;
    case domain::NodeType::ByTransport:
        // B4: membership is NODE-resolved (SessionScope::ByTransport fetch); the client only
        // projects the reported id set. Before/without a fetch the set is empty -> empty list
        // (honest), not a client-side re-derivation.
        return !row.archived && m_transportSessions.value(scope.lensKey).contains(row.sessionId);
    default:
        // Daemon sessions are flat in the first slice: no unit-tree or tag scoping yet.
        return false;
    }
}

QList<domain::UnitNode> CachedSessionStore::unitChildren(const domain::UnitId& parentId) const {
    // Offline-first fleet sidebar: children of `parentId` (empty == roots) from the cached tree,
    // preserving the pre-order the fleet repository persisted (fleetUnits() is ORDER BY ordinal).
    QList<domain::UnitNode> out;
    if (m_cache == nullptr) {
        return out;
    }
    const QString parent = parentId.toString();
    for (const CachedFleetUnitRow& r : m_cache->fleetUnits()) {
        if (r.parentId == parent) {
            out.push_back(unitNodeFromRow(r));
        }
    }
    return out;
}

domain::UnitNode CachedSessionStore::unit(const domain::UnitId& id) const {
    if (m_cache == nullptr || id.isEmpty()) {
        return {};
    }
    const QString key = id.toString();
    for (const CachedFleetUnitRow& r : m_cache->fleetUnits()) {
        if (r.unitId == key) {
            return unitNodeFromRow(r);
        }
    }
    return {};
}

QList<domain::Tag> CachedSessionStore::tags() const {
    return {};
}

QList<domain::Participant> CachedSessionStore::participants() const {
    // Daemon-backed participant rosters are a later slice; the cache has none yet.
    return {};
}

QList<domain::Session> CachedSessionStore::sessions(const domain::ListScope& scope) const {
    QList<domain::Session> out;
    for (int i = 0; i < m_snapshot.size(); ++i) {
        const CachedSessionRow& row = m_snapshot.at(i);
        if (!matchesScope(row, scope)) {
            continue;
        }
        domain::Session session;
        session.id = i;
        session.sessionId = domain::SessionId(row.sessionId);
        session.title = row.title;
        session.isArchived = row.archived;
        session.isPinned = row.pinned;
        // Project the full SessionInfo metadata the cache carries (previously dropped), so the
        // sidebar/list can show bound profile, lifecycle, role, run-state, and parentage.
        session.boundProfile = domain::ProfileRef(row.profileRef);
        session.state = stateFromString(row.state);
        session.lifecycle = lifecycleFromString(row.lifecycle);
        session.role = roleFromString(row.role);
        if (!row.parentSessionId.isEmpty()) {
            session.parent = domain::SessionId(row.parentSessionId);
        }
        session.lastActivityMs = row.updatedAtMs;
        if (row.updatedAtMs > 0) {
            session.modified = QDateTime::fromMSecsSinceEpoch(row.updatedAtMs);
            session.created = session.modified;
        }
        // Offline preview text (latest cached message); the list model truncates for the snippet.
        session.content = m_snippets.value(row.sessionId);
        out.push_back(session);
    }
    std::ranges::stable_partition(out, [](const domain::Session& s) { return s.isPinned; });
    return out;
}

int CachedSessionStore::sessionCount(const domain::ListScope& scope) const {
    int count = 0;
    for (const CachedSessionRow& row : m_snapshot) {
        if (matchesScope(row, scope)) {
            ++count;
        }
    }
    return count;
}

int CachedSessionStore::indexOfSessionId(const domain::SessionId& id) const {
    if (id.isEmpty()) {
        return -1;
    }
    const QString key = id.toString();
    for (int i = 0; i < m_snapshot.size(); ++i) {
        if (m_snapshot.at(i).sessionId == key) {
            return i;
        }
    }
    return -1;
}

domain::UnitId CachedSessionStore::createUnit(const domain::UnitId&, domain::UnitKind) {
    return {};
}

int CachedSessionStore::createTag(const QString&, const QString&) {
    return -1;
}

// --- SessionId-keyed implementations (canonical; daemon-owned mutations stay inert) ---
QString CachedSessionStore::content(const domain::SessionId& id) const {
    // L3 render-from-cache: project the durable coalesced transcript blocks (schema v2) into the
    // CANONICAL msg-fence transcript markdown (the same ```msg boundary markers the live ingest
    // produces), so a refocus / cold start / roster-driven reload round-trips into the same chat
    // bubbles as the live document — never a lossy "**Role:** text" digest the BlockEditor parses
    // as plain paragraphs.
    if (m_cache == nullptr || id.isEmpty()) {
        return {};
    }
    const QList<CachedTranscriptBlockRow> blocks = m_cache->transcriptBlocks(id.toString());

    // Pre-pass: the `todo` tool's blocks are the composer status stack's feed, not transcript
    // content (mirrors the turn engine's live suppression) — collect its call ids so results
    // skip too. Every other ToolResult is folded into its ToolCall's fence (ok/error status),
    // matching the live document's single patched tool card.
    QSet<QString> todoCalls;
    QHash<QString, const CachedTranscriptBlockRow*> resultByCall;
    for (const CachedTranscriptBlockRow& b : blocks) {
        if (b.kind == QStringLiteral("ToolCall") && b.toolName == QStringLiteral("todo")) {
            todoCalls.insert(b.callId);
        } else if (b.kind == QStringLiteral("ToolResult") && !resultByCall.contains(b.callId)) {
            resultByCall.insert(b.callId, &b);
        }
    }

    QString out;
    // The open ```msg group's role ("" = none). Reasoning/tool rows open (or continue) an
    // assistant group; the turn's final assistant Message row continues such a group (matching
    // the live single-assistant-turn grouping), while any other Message row starts a fresh
    // bubble.
    QString openRole;
    bool openedByAgentBlock = false;
    const auto openMessage = [&out, &openRole](const QString& role, quint64 seq) {
        openRole = role;
        // The marker shape serializeMessageMarker emits ({"id","role"}, sorted keys); ids are
        // seq-derived so a re-projection is byte-stable.
        out += QStringLiteral("```msg\n{\"id\":\"cache-%1\",\"role\":\"%2\"}\n```\n\n")
                   .arg(seq)
                   .arg(role);
    };
    const auto openAssistantFor = [&openMessage, &openRole, &openedByAgentBlock](quint64 seq) {
        if (openRole != QStringLiteral("assistant")) {
            openMessage(QStringLiteral("assistant"), seq);
            openedByAgentBlock = true;
        }
    };
    // A typed agent block's canonical fenced form (```reasoning / ```tool with a compact JSON
    // metadata body) — the exact shape the live ingest serializes, so the reload re-parses into
    // the same cards.
    const auto appendFence = [&out](const QString& info, const QJsonObject& meta) {
        out +=
            QStringLiteral("```%1\n%2\n```\n\n")
                .arg(info, QString::fromUtf8(QJsonDocument(meta).toJson(QJsonDocument::Compact)));
    };
    for (const CachedTranscriptBlockRow& b : blocks) {
        if (b.kind == QStringLiteral("Message")) {
            const QString role =
                (b.role.isEmpty() ? QStringLiteral("Assistant") : b.role).toLower();
            const bool continuesAgentTurn =
                openedByAgentBlock && role == QStringLiteral("assistant") && openRole == role;
            if (!continuesAgentTurn) {
                openMessage(role, b.seq);
            }
            openedByAgentBlock = false;
            out += b.text;
            out += QStringLiteral("\n\n");
        } else if (b.kind == QStringLiteral("Reasoning")) {
            openAssistantFor(b.seq);
            appendFence(QStringLiteral("reasoning"),
                        QJsonObject{{QStringLiteral("status"), QStringLiteral("complete")},
                                    {QStringLiteral("body"), b.text}});
        } else if (b.kind == QStringLiteral("ToolCall")) {
            if (todoCalls.contains(b.callId)) {
                continue;
            }
            openAssistantFor(b.seq);
            const CachedTranscriptBlockRow* result = resultByCall.value(b.callId, nullptr);
            const QString status = result == nullptr ? QStringLiteral("running")
                                   : result->ok      ? QStringLiteral("ok")
                                                     : QStringLiteral("error");
            QJsonObject tool{{QStringLiteral("callId"), b.callId},
                             {QStringLiteral("name"), b.toolName},
                             {QStringLiteral("argsSummary"), b.argsSummary},
                             {QStringLiteral("status"), status}};
            // D1: fold the persisted rich result into the fence RAW (full content in `summary`,
            // typed JSON detail as UTF-8 text) - the reload's buildToolView projects it into the
            // flat renderer keys, exactly like the live path.
            if (result != nullptr) {
                if (!result->summary.isEmpty()) {
                    tool.insert(QStringLiteral("summary"), result->summary);
                }
                if (!result->detailKind.isEmpty()) {
                    tool.insert(QStringLiteral("detailKind"), result->detailKind);
                    tool.insert(QStringLiteral("detailBody"),
                                QString::fromUtf8(result->detailBody));
                }
            }
            appendFence(QStringLiteral("tool"), tool);
        }
        // ToolResult rows carry no standalone rendering: folded into their call's fence above
        // (todo results are dropped with their call).
    }
    return out.trimmed();
}

QString CachedSessionStore::title(const domain::SessionId& id) const {
    const int i = indexOfSessionId(id);
    return i >= 0 ? m_snapshot.at(i).title : QString();
}

bool CachedSessionStore::isPinned(const domain::SessionId& id) const {
    const int i = indexOfSessionId(id);
    return i >= 0 && m_snapshot.at(i).pinned;
}

domain::SessionId CachedSessionStore::newSession(const domain::UnitId&) {
    // Session lifecycle is daemon-owned; creation flows through Submit, not the cache adapter.
    return {};
}

void CachedSessionStore::setContent(const domain::SessionId&, const QString&) {}

// Pin / archive / title are node-owned domain state. We DO NOT write the cache and reload (that
// would fork the node's truth and make a rejected change look applied); we send a SessionUpdateMeta
// intent and let the node's authoritative response drive the roster refetch (SessionRepository ->
// SessionMetaChanged / Ok refetch -> sessionsRefreshed -> reload -> changed()). A rejected write
// surfaces via metaUpdateFailed (relayed to the UI), never a silent local mutation.
void CachedSessionStore::setArchived(const domain::SessionId& id, bool archived) {
    if (m_sessions != nullptr && !id.isEmpty()) {
        m_sessions->updateSessionMeta(id.toString(), std::nullopt, archived, std::nullopt);
    }
}

void CachedSessionStore::renameSession(const domain::SessionId& id, const QString& title) {
    if (m_sessions != nullptr && !id.isEmpty()) {
        m_sessions->updateSessionMeta(id.toString(), std::nullopt, std::nullopt, title);
    }
}

void CachedSessionStore::deleteSession(const domain::SessionId&) {}

void CachedSessionStore::setPinned(const domain::SessionId& id, bool pinned) {
    if (m_sessions != nullptr && !id.isEmpty()) {
        m_sessions->updateSessionMeta(id.toString(), pinned, std::nullopt, std::nullopt);
    }
}

void CachedSessionStore::moveSession(const domain::SessionId&, int) {}

void CachedSessionStore::refreshSessionsForProfile(const QString& profileId) {
    if (m_sessions != nullptr && !profileId.isEmpty()) {
        m_sessions->refreshSessionsByProfile(profileId);
    }
}

void CachedSessionStore::refreshArchivedSessions() {
    // F6: the archived scope is fetched on demand (TopLevel excludes it); the repo merges the
    // rows additively and sessionsRefreshed() drives reload() -> changed().
    if (m_sessions != nullptr) {
        m_sessions->refreshArchivedSessions();
    }
}

void CachedSessionStore::refreshSessionsForTransport(const QString& transportId) {
    // B4: the node resolves ByTransport membership; the resolved id set backs matchesScope.
    if (m_sessions != nullptr && !transportId.isEmpty()) {
        m_sessions->refreshSessionsByTransport(transportId);
    }
}

void CachedSessionStore::requestNewSession(const QString& profileId) {
    // Daemon-owned lifecycle: the node CREATES the session and mints the id. The repo's
    // sessionCreated (relayed as ISessionStore::sessionCreated) carries the node-provided id.
    if (m_sessions != nullptr) {
        m_sessions->createSession(profileId);
    }
}

} // namespace daemonapp::daemon
