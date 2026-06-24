#include "daemon/repositories.h"

#include <QDateTime>

namespace daemonapp::daemon {

RepositoryBase::RepositoryBase(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent)
    : QObject(parent)
    , m_client(client)
    , m_cache(cache)
{
}

bool SessionRepository::upsertCachedSession(const CachedSessionRow& row)
{
    return cache() != nullptr && cache()->upsertSession(row);
}

QList<CachedSessionRow> SessionRepository::cachedSessions() const
{
    return cache() != nullptr ? cache()->sessions() : QList<CachedSessionRow>{};
}

bool SessionRepository::appendCachedLog(const CachedLogRow& row)
{
    return cache() != nullptr && cache()->appendSessionLog(row);
}

QList<CachedLogRow> SessionRepository::cachedLog(const QString& sessionId, quint64 afterSeq,
                                                 int limit) const
{
    return cache() != nullptr ? cache()->sessionLog(sessionId, afterSeq, limit)
                              : QList<CachedLogRow>{};
}

bool SessionRepository::setLogCursor(const QString& sessionId, quint64 seq)
{
    return cache() != nullptr
        && cache()->setCursor(QStringLiteral("session-log/%1").arg(sessionId),
                              QString::number(seq), QDateTime::currentMSecsSinceEpoch());
}

quint64 SessionRepository::logCursor(const QString& sessionId) const
{
    return cache() != nullptr
        ? cache()->cursor(QStringLiteral("session-log/%1").arg(sessionId)).toULongLong()
        : 0;
}

void SessionRepository::refreshSessions(const QByteArray& sessionsQueryRequestCbor)
{
    if (client() != nullptr) {
        client()->sendRequest(sessionsQueryRequestCbor, QStringLiteral("sessions-query"));
    }
}

void SessionRepository::subscribe(const QString& sessionId, const QByteArray& subscribeRequestCbor)
{
    if (client() != nullptr) {
        client()->sendRequest(subscribeRequestCbor, QStringLiteral("subscribe/%1").arg(sessionId));
    }
}

bool FsRepository::upsertCachedEntry(const CachedFsEntryRow& row)
{
    return cache() != nullptr && cache()->upsertFsEntry(row);
}

QList<CachedFsEntryRow> FsRepository::cachedEntries(const QString& rootId) const
{
    return cache() != nullptr ? cache()->fsEntries(rootId) : QList<CachedFsEntryRow>{};
}

bool ApprovalRepository::upsertCachedApproval(const CachedApprovalRow& row)
{
    return cache() != nullptr && cache()->upsertApproval(row);
}

QList<CachedApprovalRow> ApprovalRepository::cachedApprovals() const
{
    return cache() != nullptr ? cache()->approvals() : QList<CachedApprovalRow>{};
}

void ApprovalRepository::respond(const QByteArray& respondRequestCbor)
{
    if (client() != nullptr) {
        client()->sendRequest(respondRequestCbor, QStringLiteral("respond-approval"));
    }
}

} // namespace daemonapp::daemon
