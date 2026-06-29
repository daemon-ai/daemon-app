// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "fs/ifs_service.h"

#include <QByteArray>
#include <QHash>
#include <QString>

class QTimer;

namespace daemonapp::daemon {
class NodeApiClient;
class DaemonCacheStore;
} // namespace daemonapp::daemon

namespace fs {

// Daemon-backed IFsService (Phase 4): serves the file explorer/finder/editor/search over the
// NodeApi `fs_*` ops through the shared NodeApiClient + NodeApiCodec, instead of the local disk -
// the correct path for a remote/embedded daemon (whose workspace the client cannot reach locally).
// Each async op is correlated by a unique id; the response decodes and emits the matching
// IFsService signal. `watch()` runs an FsWatchPoll cursor loop and, on a `reset` page (the ring
// aged the cursor out), forces a `changed()` so consumers re-list and reconcile - the lossless
// watch contract. The last-listed tree is mirrored into DaemonCacheStore (daemon_fs_entries) as an
// offline fallback.
class DaemonFsService : public IFsService {
    Q_OBJECT

public:
    DaemonFsService(daemonapp::daemon::NodeApiClient* client,
                    daemonapp::daemon::DaemonCacheStore* cache, QObject* parent = nullptr);

    void listRoots() override;
    void open(const QString& rootId, const QString& dir) override;
    void stat(const QString& rootId, const QString& path) override;
    void read(const QString& rootId, const QString& path) override;
    void write(const QString& rootId, const QString& path, const QByteArray& bytes,
               const QString& baseRevision, bool force) override;
    void search(const QString& rootId, const QString& query, const QVariantMap& opts) override;
    void watch(const QString& rootId, const QString& dir) override;
    void unwatch(const QString& rootId, const QString& dir) override;

private:
    enum class Op { Roots, List, Stat, Read, Write, Search, WatchPoll };
    struct Pending {
        Op op = Op::Roots;
        QString rootId;
        QString path; // file path (read/stat/write) or dir (list) or watch key (watch)
        QString query;
    };
    struct WatchEntry {
        QString rootId;
        QString dir;
        quint64 afterSeq = 0;
        QTimer* timer = nullptr;
    };

    void onResponse(const QString& correlationId, const QByteArray& responseCbor);
    void onFailure(const QString& correlationId, const QString& message);
    QString track(const Pending& ctx); // register + return a fresh correlation id
    void pollWatch(const QString& key);
    [[nodiscard]] static QString watchKey(const QString& rootId, const QString& dir);
    void cacheEntries(const QString& rootId, const QList<FsEntry>& entries);

    daemonapp::daemon::NodeApiClient* m_client = nullptr;
    daemonapp::daemon::DaemonCacheStore* m_cache = nullptr;
    quint64 m_nextId = 1;
    QHash<QString, Pending> m_pending;
    QHash<QString, WatchEntry> m_watches; // key -> cursor + poll timer

    static constexpr int kWatchPollMs = 1500;
};

} // namespace fs
