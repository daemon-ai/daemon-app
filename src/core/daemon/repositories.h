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

// Sessions are the first live daemon slice: refreshSessions() issues a SessionsQuery via the
// NodeApiClient, the response is decoded by NodeApiCodec, and the resulting rows are written into
// the DaemonCacheStore. UI/adapters read the cache, never the wire.
class SessionRepository : public RepositoryBase {
    Q_OBJECT

public:
    SessionRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

    bool upsertCachedSession(const CachedSessionRow& row);
    [[nodiscard]] QList<CachedSessionRow> cachedSessions() const;
    bool appendCachedLog(const CachedLogRow& row);
    [[nodiscard]] QList<CachedLogRow> cachedLog(const QString& sessionId, quint64 afterSeq = 0,
                                                int limit = 0) const;
    bool setLogCursor(const QString& sessionId, quint64 seq);
    [[nodiscard]] quint64 logCursor(const QString& sessionId) const;

    // Issue a SessionsQuery; on success the cache is updated and sessionsRefreshed() fires.
    void refreshSessions();

signals:
    void sessionsRefreshed();
    void refreshFailed(const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    static constexpr auto kSessionsCorrelation = "repo/sessions-query";
};

class ProfileRepository : public RepositoryBase {
public:
    using RepositoryBase::RepositoryBase;

    bool upsertCachedProfile(const CachedProfileRow& row);
    [[nodiscard]] QList<CachedProfileRow> cachedProfiles() const;
};

// Not part of the first daemon slice: kept as a cache/NodeApi-aware stub until its
// daemon-api codec subset (model catalog) is generated.
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
};

// Not part of the first daemon slice: kept as a cache/NodeApi-aware stub until checkpoint
// timelines are modeled in the daemon-api codec subset.
class CheckpointRepository : public RepositoryBase {
public:
    using RepositoryBase::RepositoryBase;
};

} // namespace daemonapp::daemon
