#include "daemon/repositories.h"

#include "daemon/node_api_codec.h"

#include <QDateTime>

namespace daemonapp::daemon {

RepositoryBase::RepositoryBase(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent)
    : QObject(parent), m_client(client), m_cache(cache) {}

SessionRepository::SessionRepository(NodeApiClient* client, DaemonCacheStore* cache,
                                     QObject* parent)
    : RepositoryBase(client, cache, parent) {
    if (this->client() != nullptr) {
        connect(this->client(), &NodeApiClient::responseReady, this,
                &SessionRepository::handleResponse);
        connect(this->client(), &NodeApiClient::failed, this, &SessionRepository::handleFailure);
    }
}

bool SessionRepository::upsertCachedSession(const CachedSessionRow& row) {
    return cache() != nullptr && cache()->upsertSession(row);
}

QList<CachedSessionRow> SessionRepository::cachedSessions() const {
    return cache() != nullptr ? cache()->sessions() : QList<CachedSessionRow>{};
}

bool SessionRepository::appendCachedLog(const CachedLogRow& row) {
    return cache() != nullptr && cache()->appendSessionLog(row);
}

QList<CachedLogRow> SessionRepository::cachedLog(const QString& sessionId, quint64 afterSeq,
                                                 int limit) const {
    return cache() != nullptr ? cache()->sessionLog(sessionId, afterSeq, limit)
                              : QList<CachedLogRow>{};
}

bool SessionRepository::setLogCursor(const QString& sessionId, quint64 seq) {
    return cache() != nullptr &&
           cache()->setCursor(QStringLiteral("session-log/%1").arg(sessionId), QString::number(seq),
                              QDateTime::currentMSecsSinceEpoch());
}

quint64 SessionRepository::logCursor(const QString& sessionId) const {
    return cache() != nullptr
               ? cache()->cursor(QStringLiteral("session-log/%1").arg(sessionId)).toULongLong()
               : 0;
}

void SessionRepository::refreshSessions() {
    if (client() == nullptr) {
        emit refreshFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeSessionsQueryRequest(),
                          QLatin1String(kSessionsCorrelation));
}

void SessionRepository::subscribe(const QString& sessionId) {
    if (client() == nullptr) {
        emit refreshFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    // Resume from the persisted cursor so each poll only pulls new entries.
    constexpr quint32 kMaxEntries = 256;
    client()->sendRequest(
        NodeApiCodec::encodeSubscribeRequest(sessionId, logCursor(sessionId), kMaxEntries),
        subscribeCorrelation(sessionId));
}

QString SessionRepository::subscribeCorrelation(const QString& sessionId) {
    return QLatin1String(kSubscribePrefix) + sessionId;
}

void SessionRepository::handleResponse(const QString& correlationId,
                                       const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kSessionsCorrelation)) {
        QList<CachedSessionRow> rows;
        QString nextCursor;
        if (!NodeApiCodec::decodeSessionPage(responseCbor, &rows, &nextCursor)) {
            emit refreshFailed(QStringLiteral("Failed to decode SessionPage response"));
            return;
        }
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        for (CachedSessionRow& row : rows) {
            row.updatedAtMs = now;
            upsertCachedSession(row);
        }
        if (!nextCursor.isEmpty() && cache() != nullptr) {
            cache()->setCursor(QStringLiteral("sessions-query/cursor"), nextCursor, now);
        }
        emit sessionsRefreshed();
        return;
    }

    if (correlationId.startsWith(QLatin1String(kSubscribePrefix))) {
        const QString sessionId = correlationId.mid(int(qstrlen(kSubscribePrefix)));
        QList<CachedLogRow> rows;
        quint64 nextSeq = 0;
        if (!NodeApiCodec::decodeLogPage(responseCbor, sessionId, &rows, &nextSeq)) {
            emit refreshFailed(QStringLiteral("Failed to decode LogPage response"));
            return;
        }
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        for (CachedLogRow& row : rows) {
            row.updatedAtMs = now;
            appendCachedLog(row);
        }
        if (nextSeq > 0) {
            setLogCursor(sessionId, nextSeq);
        }
        emit logUpdated(sessionId);
        return;
    }
}

void SessionRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kSessionsCorrelation) ||
        correlationId.startsWith(QLatin1String(kSubscribePrefix))) {
        emit refreshFailed(message);
    }
}

bool ProfileRepository::upsertCachedProfile(const CachedProfileRow& row) {
    return cache() != nullptr && cache()->upsertProfile(row);
}

QList<CachedProfileRow> ProfileRepository::cachedProfiles() const {
    return cache() != nullptr ? cache()->profiles() : QList<CachedProfileRow>{};
}

bool FsRepository::upsertCachedEntry(const CachedFsEntryRow& row) {
    return cache() != nullptr && cache()->upsertFsEntry(row);
}

QList<CachedFsEntryRow> FsRepository::cachedEntries(const QString& rootId) const {
    return cache() != nullptr ? cache()->fsEntries(rootId) : QList<CachedFsEntryRow>{};
}

bool ApprovalRepository::upsertCachedApproval(const CachedApprovalRow& row) {
    return cache() != nullptr && cache()->upsertApproval(row);
}

QList<CachedApprovalRow> ApprovalRepository::cachedApprovals() const {
    return cache() != nullptr ? cache()->approvals() : QList<CachedApprovalRow>{};
}

} // namespace daemonapp::daemon
