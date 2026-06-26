#include "daemon/cached_session_store.h"

#include "daemon/repositories.h"

#include <algorithm>
#include <QDateTime>

namespace daemonapp::daemon {

CachedSessionStore::CachedSessionStore(DaemonCacheStore* cache, SessionRepository* sessions,
                                       QObject* parent)
    : persistence::ISessionStore(parent), m_cache(cache), m_sessions(sessions) {
    if (m_sessions != nullptr) {
        connect(m_sessions, &SessionRepository::sessionsRefreshed, this,
                &CachedSessionStore::reload);
    }
    reload();
}

void CachedSessionStore::reload() {
    m_snapshot = m_cache != nullptr ? m_cache->sessions() : QList<CachedSessionRow>{};
    emit changed();
}

bool CachedSessionStore::matchesScope(const CachedSessionRow& row, const domain::ListScope& scope) {
    switch (scope.type) {
    case domain::NodeType::Archived:
        return row.archived;
    case domain::NodeType::AllSessions:
        return !row.archived;
    default:
        // Daemon sessions are flat in the first slice: no unit-tree or tag scoping yet.
        return false;
    }
}

QList<domain::UnitNode> CachedSessionStore::unitChildren(const domain::UnitId&) const {
    return {};
}

domain::UnitNode CachedSessionStore::unit(const domain::UnitId&) const {
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
        if (row.updatedAtMs > 0) {
            session.modified = QDateTime::fromMSecsSinceEpoch(row.updatedAtMs);
            session.created = session.modified;
        }
        out.push_back(session);
    }
    std::stable_partition(out.begin(), out.end(),
                          [](const domain::Session& s) { return s.isPinned; });
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
QString CachedSessionStore::content(const domain::SessionId&) const {
    // Structured session-log projection is a later slice; the transcript is not cached as markdown.
    return {};
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

} // namespace daemonapp::daemon
