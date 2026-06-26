#pragma once

#include "fs/fs_dtos.h"

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariantMap>

namespace fs {

// Transport-agnostic filesystem seam (sibling of IConnectionService): the GUI
// reads and writes workspace files through the connected node, never off the
// local disk in production. Every op is async + signal-based to suit a
// latency-bound remote transport. Paths are (rootId, root-relative POSIX);
// `revision` strings are opaque (staleness guard for write). The dev
// LocalDiskFsService backs the UI now; a daemon adapter replaces it later by
// decoding the wire once. UI never sees the codec, and the wire shape
// (daemon-fs-surface-spec) is free to change without touching this seam.
class IFsService : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    ~IFsService() override = default;

    // The browsable roots this node exposes (default workspace root + any
    // opened unit sandboxes). Result via rootsChanged.
    Q_INVOKABLE virtual void listRoots() = 0;
    // One directory's children (root-relative dir, "" = root). Result via listed.
    Q_INVOKABLE virtual void open(const QString& rootId, const QString& dir) = 0;
    // One entry's metadata. Result via statResult.
    Q_INVOKABLE virtual void stat(const QString& rootId, const QString& path) = 0;
    // Read a file's bytes + content revision. Result via fileRead.
    Q_INVOKABLE virtual void read(const QString& rootId, const QString& path) = 0;
    // Write with optimistic concurrency: a stale `baseRevision` is rejected
    // (empty base = unconditional / new file). Result via writeResult.
    Q_INVOKABLE virtual void write(const QString& rootId, const QString& path,
                                   const QByteArray& bytes, const QString& baseRevision,
                                   bool force = false) = 0;
    // Server-side project search (content; opts may carry "regex","caseSensitive",
    // "maxResults"). Result via searchResults (possibly several, last is complete).
    Q_INVOKABLE virtual void search(const QString& rootId, const QString& query,
                                    const QVariantMap& opts) = 0;
    // Subscribe / unsubscribe to change events for an opened directory.
    Q_INVOKABLE virtual void watch(const QString& rootId, const QString& dir) = 0;
    Q_INVOKABLE virtual void unwatch(const QString& rootId, const QString& dir) = 0;

signals:
    void rootsChanged(const QList<fs::FsRoot>& roots);
    void listed(const QString& rootId, const QString& dir, const QList<fs::FsEntry>& entries);
    void statResult(const QString& rootId, const QString& path, const fs::FsEntry& entry, bool ok);
    void fileRead(const QString& rootId, const QString& path, const QByteArray& bytes,
                  const QString& revision, bool binary, bool truncated);
    void writeResult(const QString& rootId, const QString& path, bool ok, const QString& revision,
                     const QString& error);
    void searchResults(const QString& rootId, const QString& query,
                       const QList<fs::FsSearchHit>& hits, bool complete);
    // The directory changed externally; consumers invalidate that subtree / reload.
    void changed(const QString& rootId, const QString& dir);
    void error(const QString& rootId, const QString& path, const QString& message);
};

// Register the DTO list metatypes used by the signals above (safe to call more
// than once). Lets queued connections and QML marshal FsEntry/FsRoot lists.
void registerFsMetatypes();

} // namespace fs
