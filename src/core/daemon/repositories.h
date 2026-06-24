#pragma once

#include "daemon/daemon_cache_store.h"
#include "daemon/node_api_client.h"

#include <QObject>
#include <QString>

namespace daemonapp::daemon {

class RepositoryBase : public QObject {
public:
    RepositoryBase(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

protected:
    [[nodiscard]] NodeApiClient* client() const { return m_client; }
    [[nodiscard]] DaemonCacheStore* cache() const { return m_cache; }

private:
    NodeApiClient* m_client = nullptr;
    DaemonCacheStore* m_cache = nullptr;
};

class SessionRepository : public RepositoryBase {
public:
    using RepositoryBase::RepositoryBase;

    bool upsertCachedSession(const CachedSessionRow& row);
    [[nodiscard]] QList<CachedSessionRow> cachedSessions() const;
    bool appendCachedLog(const CachedLogRow& row);
    [[nodiscard]] QList<CachedLogRow> cachedLog(const QString& sessionId, quint64 afterSeq = 0,
                                                int limit = 0) const;
    bool setLogCursor(const QString& sessionId, quint64 seq);
    [[nodiscard]] quint64 logCursor(const QString& sessionId) const;

    void refreshSessions(const QByteArray& sessionsQueryRequestCbor);
    void subscribe(const QString& sessionId, const QByteArray& subscribeRequestCbor);
};

class ProfileRepository : public RepositoryBase {
public:
    using RepositoryBase::RepositoryBase;
};

class ModelRepository : public RepositoryBase {
public:
    using RepositoryBase::RepositoryBase;
};

class FsRepository : public RepositoryBase {
public:
    using RepositoryBase::RepositoryBase;

    bool upsertCachedEntry(const CachedFsEntryRow& row);
    [[nodiscard]] QList<CachedFsEntryRow> cachedEntries(const QString& rootId) const;
};

class ApprovalRepository : public RepositoryBase {
public:
    using RepositoryBase::RepositoryBase;

    bool upsertCachedApproval(const CachedApprovalRow& row);
    [[nodiscard]] QList<CachedApprovalRow> cachedApprovals() const;
    void respond(const QByteArray& respondRequestCbor);
};

class CheckpointRepository : public RepositoryBase {
public:
    using RepositoryBase::RepositoryBase;
};

} // namespace daemonapp::daemon
