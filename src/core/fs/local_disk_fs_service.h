#pragma once

#include "fs/ifs_service.h"

#include <QHash>
#include <QString>

QT_BEGIN_NAMESPACE
class QFileSystemWatcher;
QT_END_NAMESPACE

namespace fs {

// DEV-ONLY IFsService implementation that serves a single root from the local
// disk (QDir + QFileSystemWatcher). It exists so the file tree / finder /
// editor can be built and exercised against real files before the daemon FS
// surface lands; it is NOT the production source of truth (production reads the
// node via a daemon adapter). Results are posted on the event loop to mimic the
// async shape of a real transport.
class LocalDiskFsService : public IFsService {
    Q_OBJECT

public:
    // `rootPath` is the absolute base directory served as the single root
    // (defaults to the user's home if empty/invalid). `label` is its display name.
    explicit LocalDiskFsService(const QString& rootPath, const QString& label = QString(),
                                QObject* parent = nullptr);
    ~LocalDiskFsService() override;

    void listRoots() override;
    void open(const QString& rootId, const QString& dir) override;
    void stat(const QString& rootId, const QString& path) override;
    void read(const QString& rootId, const QString& path) override;
    void write(const QString& rootId, const QString& path, const QByteArray& bytes,
               const QString& baseRevision) override;
    void search(const QString& rootId, const QString& query, const QVariantMap& opts) override;
    void watch(const QString& rootId, const QString& dir) override;
    void unwatch(const QString& rootId, const QString& dir) override;

private:
    struct Root {
        QString id;
        QString label;
        QString base; // absolute, normalized, no trailing separator
    };

    // Resolve (rootId, root-relative path) to a contained absolute path. Returns
    // empty on unknown root or an escape attempt (".." past the root).
    [[nodiscard]] QString resolve(const QString& rootId, const QString& path) const;
    // Inverse: absolute path -> root-relative ("" when it is the root itself);
    // sets `rootId`. Returns false when not under any served root.
    [[nodiscard]] bool relativize(const QString& absPath, QString& rootId, QString& rel) const;
    [[nodiscard]] static bool isIgnored(const QString& name);
    [[nodiscard]] static QString revisionFor(const QString& absPath);

    QHash<QString, Root> m_roots; // id -> Root (a single entry in the dev impl)
    QString m_primaryRootId;
    QFileSystemWatcher* m_watcher = nullptr;
    // Active watch absolute dirs -> (rootId, rel dir), to map watcher signals back.
    QHash<QString, QPair<QString, QString>> m_watched;
};

} // namespace fs
