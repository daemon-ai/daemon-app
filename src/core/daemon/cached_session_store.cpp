#include "daemon/cached_session_store.h"

#include "daemon/repositories.h"

#include <QDateTime>

#include <algorithm>

namespace daemonapp::daemon {

CachedSessionStore::CachedSessionStore(DaemonCacheStore* cache, SessionRepository* sessions,
                                       QObject* parent)
    : persistence::ISessionStore(parent)
    , m_cache(cache)
    , m_sessions(sessions)
{
    if (m_sessions != nullptr) {
        connect(m_sessions, &SessionRepository::sessionsRefreshed, this,
                &CachedSessionStore::reload);
    }
    reload();
}

void CachedSessionStore::reload()
{
    m_snapshot = m_cache != nullptr ? m_cache->sessions() : QList<CachedSessionRow>{};
    emit changed();
}

bool CachedSessionStore::matchesScope(const CachedSessionRow& row, const domain::ListScope& scope)
{
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

QList<domain::UnitNode> CachedSessionStore::unitChildren(const domain::UnitId&) const
{
    return {};
}

domain::UnitNode CachedSessionStore::unit(const domain::UnitId&) const
{
    return {};
}

QList<domain::Tag> CachedSessionStore::tags() const
{
    return {};
}

QList<domain::Session> CachedSessionStore::sessions(const domain::ListScope& scope) const
{
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

int CachedSessionStore::sessionCount(const domain::ListScope& scope) const
{
    int count = 0;
    for (const CachedSessionRow& row : m_snapshot) {
        if (matchesScope(row, scope)) {
            ++count;
        }
    }
    return count;
}

QString CachedSessionStore::content(int sessionId) const
{
    // Structured session-log projection is a later slice; the transcript is not cached as markdown.
    Q_UNUSED(sessionId)
    return {};
}

QString CachedSessionStore::title(int sessionId) const
{
    return sessionId >= 0 && sessionId < m_snapshot.size() ? m_snapshot.at(sessionId).title
                                                           : QString();
}

int CachedSessionStore::createSession(const domain::UnitId&)
{
    // Session lifecycle is daemon-owned; creation flows through Submit, not the cache adapter.
    return -1;
}

domain::UnitId CachedSessionStore::createUnit(const domain::UnitId&, domain::UnitKind)
{
    return {};
}

int CachedSessionStore::createTag(const QString&, const QString&)
{
    return -1;
}

void CachedSessionStore::setContent(int, const QString&)
{
}

void CachedSessionStore::setArchived(int sessionId, bool archived)
{
    if (sessionId < 0 || sessionId >= m_snapshot.size() || m_cache == nullptr) {
        return;
    }
    CachedSessionRow row = m_snapshot.at(sessionId);
    row.archived = archived;
    row.updatedAtMs = QDateTime::currentMSecsSinceEpoch();
    if (m_cache->upsertSession(row)) {
        reload();
    }
}

void CachedSessionStore::renameSession(int, const QString&)
{
}

void CachedSessionStore::deleteSession(int)
{
}

void CachedSessionStore::setPinned(int sessionId, bool pinned)
{
    if (sessionId < 0 || sessionId >= m_snapshot.size() || m_cache == nullptr) {
        return;
    }
    CachedSessionRow row = m_snapshot.at(sessionId);
    row.pinned = pinned;
    row.updatedAtMs = QDateTime::currentMSecsSinceEpoch();
    if (m_cache->upsertSession(row)) {
        reload();
    }
}

bool CachedSessionStore::isPinned(int sessionId) const
{
    return sessionId >= 0 && sessionId < m_snapshot.size() && m_snapshot.at(sessionId).pinned;
}

void CachedSessionStore::moveSession(int, int)
{
}

} // namespace daemonapp::daemon
