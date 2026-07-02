// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/cached_session_store.h"

#include "daemon/repositories.h"

#include <algorithm>
#include <QDateTime>

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

bool CachedSessionStore::matchesScope(const CachedSessionRow& row, const domain::ListScope& scope) {
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
    // markdown the transcript view renders, so a refocus / cold start shows the conversation from
    // disk while the turn engine fetches only a short delta past the persisted watermark.
    if (m_cache == nullptr || id.isEmpty()) {
        return {};
    }
    QString out;
    const QList<CachedTranscriptBlockRow> blocks = m_cache->transcriptBlocks(id.toString());
    for (const CachedTranscriptBlockRow& b : blocks) {
        if (b.kind == QStringLiteral("Message")) {
            const QString who = b.role.isEmpty() ? QStringLiteral("Assistant") : b.role;
            out += QStringLiteral("**%1:** %2\n\n").arg(who, b.text);
        } else if (b.kind == QStringLiteral("ToolCall")) {
            out += QStringLiteral("`%1(%2)`\n\n").arg(b.toolName, b.argsSummary);
        } else if (b.kind == QStringLiteral("ToolResult")) {
            const QString prefix = b.ok ? QString() : QStringLiteral("[failed] ");
            out += QStringLiteral("> %1%2\n\n").arg(prefix, b.summary);
        }
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

void CachedSessionStore::setArchived(const domain::SessionId& id, bool archived) {
    const int i = indexOfSessionId(id);
    if (i < 0 || m_cache == nullptr) {
        return;
    }
    CachedSessionRow row = m_snapshot.at(i);
    row.archived = archived;
    row.updatedAtMs = QDateTime::currentMSecsSinceEpoch();
    if (m_cache->upsertSession(row)) {
        reload();
    }
}

void CachedSessionStore::renameSession(const domain::SessionId&, const QString&) {}

void CachedSessionStore::deleteSession(const domain::SessionId&) {}

void CachedSessionStore::setPinned(const domain::SessionId& id, bool pinned) {
    const int i = indexOfSessionId(id);
    if (i < 0 || m_cache == nullptr) {
        return;
    }
    CachedSessionRow row = m_snapshot.at(i);
    row.pinned = pinned;
    row.updatedAtMs = QDateTime::currentMSecsSinceEpoch();
    if (m_cache->upsertSession(row)) {
        reload();
    }
}

void CachedSessionStore::moveSession(const domain::SessionId&, int) {}

void CachedSessionStore::refreshSessionsForProfile(const QString& profileId) {
    if (m_sessions != nullptr && !profileId.isEmpty()) {
        m_sessions->refreshSessionsByProfile(profileId);
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
