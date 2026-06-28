#include "daemon/daemon_fs_service.h"

#include "daemon/daemon_cache_store.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"

#include <QDateTime>
#include <QTimer>
#include <QVariantMap>
#include <utility>

namespace fs {
namespace {

using daemonapp::daemon::NodeApiCodec;

QString parentDir(const QString& path) {
    const qsizetype slash = path.lastIndexOf(QLatin1Char('/'));
    return slash < 0 ? QString() : path.left(slash);
}

} // namespace

DaemonFsService::DaemonFsService(daemonapp::daemon::NodeApiClient* client,
                                 daemonapp::daemon::DaemonCacheStore* cache, QObject* parent)
    : IFsService(parent), m_client(client), m_cache(cache) {
    registerFsMetatypes();
    if (m_client != nullptr) {
        connect(m_client, &daemonapp::daemon::NodeApiClient::responseReady, this,
                &DaemonFsService::onResponse);
        connect(m_client, &daemonapp::daemon::NodeApiClient::failed, this,
                &DaemonFsService::onFailure);
    }
}

QString DaemonFsService::track(const Pending& ctx) {
    const QString id = QStringLiteral("fs/") + QString::number(m_nextId++);
    m_pending.insert(id, ctx);
    return id;
}

QString DaemonFsService::watchKey(const QString& rootId, const QString& dir) {
    return rootId + QLatin1Char('\x1f') + dir;
}

void DaemonFsService::listRoots() {
    if (m_client == nullptr) {
        return;
    }
    m_client->sendRequest(NodeApiCodec::encodeFsRootsRequest(), track({Op::Roots, {}, {}, {}}));
}

void DaemonFsService::open(const QString& rootId, const QString& dir) {
    if (m_client == nullptr) {
        return;
    }
    m_client->sendRequest(NodeApiCodec::encodeFsListRequest(rootId, dir),
                          track({Op::List, rootId, dir, {}}));
}

void DaemonFsService::stat(const QString& rootId, const QString& path) {
    if (m_client == nullptr) {
        return;
    }
    // FsStat shares the FsList encode shape on the wire; reuse the list decoder's entry struct.
    m_client->sendRequest(NodeApiCodec::encodeFsListRequest(rootId, path),
                          track({Op::Stat, rootId, path, {}}));
}

void DaemonFsService::read(const QString& rootId, const QString& path) {
    if (m_client == nullptr) {
        return;
    }
    m_client->sendRequest(NodeApiCodec::encodeFsReadRequest(rootId, path),
                          track({Op::Read, rootId, path, {}}));
}

void DaemonFsService::write(const QString& rootId, const QString& path, const QByteArray& bytes,
                            const QString& baseRevision, bool force) {
    if (m_client == nullptr) {
        return;
    }
    m_client->sendRequest(
        NodeApiCodec::encodeFsWriteRequest(rootId, path, bytes, baseRevision, force),
        track({Op::Write, rootId, path, {}}));
}

void DaemonFsService::search(const QString& rootId, const QString& query, const QVariantMap& opts) {
    if (m_client == nullptr) {
        return;
    }
    const bool regex = opts.value(QStringLiteral("regex")).toBool();
    const bool caseSensitive = opts.value(QStringLiteral("caseSensitive")).toBool();
    const quint32 maxResults = opts.value(QStringLiteral("maxResults")).toUInt();
    m_client->sendRequest(
        NodeApiCodec::encodeFsSearchRequest(rootId, query, regex, caseSensitive, maxResults),
        track({Op::Search, rootId, {}, query}));
}

void DaemonFsService::watch(const QString& rootId, const QString& dir) {
    const QString key = watchKey(rootId, dir);
    if (m_watches.contains(key)) {
        return;
    }
    WatchEntry entry;
    entry.rootId = rootId;
    entry.dir = dir;
    entry.timer = new QTimer(this);
    entry.timer->setInterval(kWatchPollMs);
    connect(entry.timer, &QTimer::timeout, this, [this, key] { pollWatch(key); });
    m_watches.insert(key, entry);
    entry.timer->start();
    pollWatch(key); // prime immediately (the daemon's first poll just snapshots, no events)
}

void DaemonFsService::unwatch(const QString& rootId, const QString& dir) {
    const QString key = watchKey(rootId, dir);
    auto it = m_watches.find(key);
    if (it == m_watches.end()) {
        return;
    }
    if (it->timer != nullptr) {
        it->timer->stop();
        it->timer->deleteLater();
    }
    m_watches.erase(it);
}

void DaemonFsService::pollWatch(const QString& key) {
    auto it = m_watches.constFind(key);
    if (it == m_watches.constEnd() || m_client == nullptr) {
        return;
    }
    m_client->sendRequest(NodeApiCodec::encodeFsWatchPollRequest(it->rootId, it->dir, it->afterSeq),
                          track({Op::WatchPoll, it->rootId, key, {}}));
}

void DaemonFsService::cacheEntries(const QString& rootId, const QList<FsEntry>& entries) {
    if (m_cache == nullptr) {
        return;
    }
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (const FsEntry& e : entries) {
        daemonapp::daemon::CachedFsEntryRow row;
        row.rootId = rootId;
        row.path = e.path;
        row.kind = e.isDir ? QStringLiteral("dir")
                           : (e.symlink ? QStringLiteral("symlink") : QStringLiteral("file"));
        row.size = e.size;
        row.mtimeMs = e.mtimeMs;
        row.updatedAtMs = now;
        m_cache->upsertFsEntry(row);
    }
}

void DaemonFsService::onResponse(const QString& correlationId, const QByteArray& responseCbor) {
    auto it = m_pending.find(correlationId);
    if (it == m_pending.end()) {
        return;
    }
    // Move out before erase (which invalidates the iterator); the value is needed below.
    const Pending ctx = std::move(it.value());
    m_pending.erase(it);

    switch (ctx.op) {
    case Op::Roots: {
        QList<daemonapp::daemon::DecodedFsRoot> decoded;
        if (!NodeApiCodec::decodeFsRoots(responseCbor, &decoded)) {
            return;
        }
        QList<FsRoot> roots;
        for (const auto& r : decoded) {
            FsRoot root;
            root.id = r.id;
            root.label = r.label;
            root.unitId = r.kind == QStringLiteral("session") ? r.id.mid(8) : QString();
            roots.append(root);
        }
        emit rootsChanged(roots);
        break;
    }
    case Op::List: {
        QList<daemonapp::daemon::DecodedFsEntry> decoded;
        if (!NodeApiCodec::decodeFsList(responseCbor, &decoded)) {
            emit error(ctx.rootId, ctx.path, QStringLiteral("Failed to decode FsList"));
            return;
        }
        QList<FsEntry> entries;
        for (const auto& e : decoded) {
            FsEntry entry;
            entry.name = e.name;
            entry.path = e.path;
            entry.isDir = e.kind == QStringLiteral("dir");
            entry.symlink = e.kind == QStringLiteral("symlink");
            entry.ignored = e.ignored;
            entry.size = static_cast<qint64>(e.size);
            entry.mtimeMs = static_cast<qint64>(e.mtimeMs);
            entries.append(entry);
        }
        cacheEntries(ctx.rootId, entries);
        emit listed(ctx.rootId, ctx.path, entries);
        break;
    }
    case Op::Stat: {
        QList<daemonapp::daemon::DecodedFsEntry> decoded;
        const bool ok = NodeApiCodec::decodeFsList(responseCbor, &decoded) && !decoded.isEmpty();
        FsEntry entry;
        if (ok) {
            const auto& e = decoded.first();
            entry.name = e.name;
            entry.path = e.path;
            entry.isDir = e.kind == QStringLiteral("dir");
            entry.symlink = e.kind == QStringLiteral("symlink");
            entry.ignored = e.ignored;
            entry.size = static_cast<qint64>(e.size);
            entry.mtimeMs = static_cast<qint64>(e.mtimeMs);
        }
        emit statResult(ctx.rootId, ctx.path, entry, ok);
        break;
    }
    case Op::Read: {
        daemonapp::daemon::DecodedFsContent content;
        if (!NodeApiCodec::decodeFsRead(responseCbor, &content)) {
            emit error(ctx.rootId, ctx.path, QStringLiteral("Failed to decode FsRead"));
            return;
        }
        const bool binary = content.bytes.contains('\0');
        emit fileRead(ctx.rootId, ctx.path, content.bytes, content.revision, binary,
                      content.truncated);
        break;
    }
    case Op::Write: {
        QString revision;
        if (NodeApiCodec::decodeFsWrite(responseCbor, &revision)) {
            emit writeResult(ctx.rootId, ctx.path, true, revision, QString());
        } else {
            // A decode miss here is typically an ApiError (e.g. a stale-revision Conflict).
            daemonapp::daemon::DecodedApiError err;
            const QString msg = NodeApiCodec::decodeError(responseCbor, &err)
                                    ? err.message
                                    : QStringLiteral("Write failed");
            emit writeResult(ctx.rootId, ctx.path, false, QString(), msg);
        }
        break;
    }
    case Op::Search: {
        daemonapp::daemon::DecodedFsSearchPage page;
        if (!NodeApiCodec::decodeFsSearch(responseCbor, &page)) {
            emit error(ctx.rootId, QString(), QStringLiteral("Failed to decode FsSearch"));
            return;
        }
        QList<FsSearchHit> hits;
        for (const auto& h : page.hits) {
            FsSearchHit hit;
            hit.path = h.path;
            hit.line = static_cast<int>(h.line);
            hit.column = static_cast<int>(h.col);
            hit.preview = h.preview;
            hits.append(hit);
        }
        emit searchResults(ctx.rootId, ctx.query, hits, !page.hasMore);
        break;
    }
    case Op::WatchPoll: {
        daemonapp::daemon::DecodedFsWatchPage page;
        if (!NodeApiCodec::decodeFsWatch(responseCbor, &page)) {
            return;
        }
        auto entry = m_watches.find(ctx.path); // ctx.path holds the watch key
        if (entry == m_watches.end()) {
            return; // unwatched while in flight
        }
        // `reset` (ring aged out) or any events => the dir changed; consumers re-list to reconcile.
        if (page.reset || !page.events.isEmpty()) {
            emit changed(entry->rootId, entry->dir);
        }
        entry->afterSeq = page.headSeq;
        break;
    }
    }
}

void DaemonFsService::onFailure(const QString& correlationId, const QString& message) {
    auto it = m_pending.find(correlationId);
    if (it == m_pending.end()) {
        return;
    }
    const Pending ctx = std::move(it.value());
    m_pending.erase(it);
    if (ctx.op == Op::WatchPoll || ctx.op == Op::Roots) {
        return; // transient; the next poll / refresh retries
    }
    if (ctx.op == Op::List && m_cache != nullptr) {
        // Offline fallback: render the last-known children of this dir from the cache.
        QList<FsEntry> entries;
        for (const auto& row : m_cache->fsEntries(ctx.rootId)) {
            if (parentDir(row.path) != ctx.path) {
                continue;
            }
            FsEntry e;
            e.path = row.path;
            e.name = row.path.mid(row.path.lastIndexOf(QLatin1Char('/')) + 1);
            e.isDir = row.kind == QStringLiteral("dir");
            e.symlink = row.kind == QStringLiteral("symlink");
            e.size = static_cast<qint64>(row.size);
            e.mtimeMs = row.mtimeMs;
            entries.append(e);
        }
        if (!entries.isEmpty()) {
            emit listed(ctx.rootId, ctx.path, entries);
            return;
        }
    }
    emit error(ctx.rootId, ctx.path, message);
}

} // namespace fs
