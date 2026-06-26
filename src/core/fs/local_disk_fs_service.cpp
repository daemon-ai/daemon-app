#include "fs/local_disk_fs_service.h"

#include <algorithm>
#include <functional>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTimer>

namespace fs {
namespace {

constexpr qint64 kMaxReadBytes = 5 * 1024 * 1024; // skip huge files in the viewer
constexpr int kMaxSearchFiles = 5000;             // dev-impl walk caps
constexpr int kMaxSearchHits = 1000;
constexpr qint64 kMaxSearchFileBytes = 2 * 1024 * 1024;

// Natural, directories-first ordering (port of Lite XL's path_compare semantics).
bool entryLess(const FsEntry& a, const FsEntry& b) {
    if (a.isDir != b.isDir)
        return a.isDir; // directories first
    const QString& x = a.name;
    const QString& y = b.name;
    int i = 0;
    int j = 0;
    while (i < x.size() && j < y.size()) {
        const QChar cx = x[i];
        const QChar cy = y[j];
        if (cx.isDigit() && cy.isDigit()) {
            // Compare runs of digits numerically (shorter run is smaller).
            int si = i;
            int sj = j;
            while (i < x.size() && x[i].isDigit())
                ++i;
            while (j < y.size() && y[j].isDigit())
                ++j;
            const QStringView nx = QStringView(x).mid(si, i - si);
            const QStringView ny = QStringView(y).mid(sj, j - sj);
            if (nx.size() != ny.size())
                return nx.size() < ny.size();
            const int c = nx.compare(ny);
            if (c != 0)
                return c < 0;
        } else {
            const QChar lx = cx.toLower();
            const QChar ly = cy.toLower();
            if (lx != ly)
                return lx < ly;
            ++i;
            ++j;
        }
    }
    return (x.size() - i) < (y.size() - j);
}

bool looksBinary(const QByteArray& head) {
    const int n = static_cast<int>(qMin<qsizetype>(head.size(), 8000));
    for (int i = 0; i < n; ++i) {
        if (head.at(i) == '\0')
            return true;
    }
    return false;
}

QString expandHome(QString path) {
    path = path.trimmed();
    if (path == QStringLiteral("~")) {
        return QDir::homePath();
    }
    if (path.startsWith(QStringLiteral("~/"))) {
        return QDir::homePath() + path.mid(1);
    }
    return path;
}

} // namespace

LocalDiskFsService::LocalDiskFsService(const QString& rootPath, const QString& label,
                                       QObject* parent)
    : IFsService(parent), m_watcher(new QFileSystemWatcher(this)) {
    registerFsMetatypes();

    QString base = expandHome(rootPath);
    if (base.isEmpty() || !QFileInfo(base).isDir())
        base = QDir::homePath();
    base = QFileInfo(base).canonicalFilePath();
    if (base.isEmpty())
        base = QDir(rootPath).absolutePath();

    Root root;
    root.id = QStringLiteral("workspace");
    root.label = label.isEmpty() ? QFileInfo(base).fileName() : label;
    if (root.label.isEmpty())
        root.label = base;
    root.base = base;
    m_primaryRootId = root.id;
    m_roots.insert(root.id, root);

    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString& path) {
        QString rootId;
        QString rel;
        if (relativize(path, rootId, rel))
            emit changed(rootId, rel);
    });
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString& path) {
        QString rootId;
        QString rel;
        if (relativize(QFileInfo(path).absolutePath(), rootId, rel))
            emit changed(rootId, rel);
    });
}

LocalDiskFsService::~LocalDiskFsService() = default;

QString LocalDiskFsService::resolve(const QString& rootId, const QString& path) const {
    const auto it = m_roots.constFind(rootId);
    if (it == m_roots.constEnd())
        return {};
    const QString abs = QDir::cleanPath(it->base + QLatin1Char('/') + path);

    // Containment is based on canonical paths for existing targets, and on the
    // canonical parent for new files. This matches the daemon-side contain(...)
    // boundary better than a string-prefix check over cleaned paths: symlinks
    // inside the served root must not grant access outside it.
    QString contained;
    const QFileInfo fi(abs);
    if (fi.exists()) {
        contained = fi.canonicalFilePath();
    } else {
        const QFileInfo parentInfo(fi.absolutePath());
        const QString parent = parentInfo.canonicalFilePath();
        if (parent.isEmpty())
            return {};
        contained = QDir::cleanPath(parent + QLatin1Char('/') + fi.fileName());
    }

    if (contained != it->base && !contained.startsWith(it->base + QLatin1Char('/')))
        return {};
    return contained;
}

bool LocalDiskFsService::relativize(const QString& absPath, QString& rootId, QString& rel) const {
    const QString abs = QDir::cleanPath(absPath);
    for (auto it = m_roots.constBegin(); it != m_roots.constEnd(); ++it) {
        if (abs == it->base) {
            rootId = it->id;
            rel = QString();
            return true;
        }
        if (abs.startsWith(it->base + QLatin1Char('/'))) {
            rootId = it->id;
            rel = abs.mid(it->base.size() + 1);
            return true;
        }
    }
    return false;
}

bool LocalDiskFsService::isIgnored(const QString& name) {
    static const QStringList kIgnored = {
        QStringLiteral(".git"),         QStringLiteral(".hg"),     QStringLiteral(".svn"),
        QStringLiteral("node_modules"), QStringLiteral("target"),  QStringLiteral(".cache"),
        QStringLiteral("__pycache__"),  QStringLiteral(".direnv"), QStringLiteral(".venv"),
    };
    return kIgnored.contains(name);
}

QString LocalDiskFsService::revisionFor(const QString& absPath) {
    const QFileInfo fi(absPath);
    if (!fi.exists())
        return {};
    return QStringLiteral("%1:%2").arg(fi.lastModified().toMSecsSinceEpoch()).arg(fi.size());
}

void LocalDiskFsService::listRoots() {
    QList<FsRoot> roots;
    for (auto it = m_roots.constBegin(); it != m_roots.constEnd(); ++it)
        roots.push_back(FsRoot{it->id, it->label, QString()});
    QMetaObject::invokeMethod(
        this, [this, roots] { emit rootsChanged(roots); }, Qt::QueuedConnection);
}

void LocalDiskFsService::open(const QString& rootId, const QString& dir) {
    const QString abs = resolve(rootId, dir);
    if (abs.isEmpty()) {
        QMetaObject::invokeMethod(
            this, [this, rootId, dir] { emit error(rootId, dir, QStringLiteral("invalid path")); },
            Qt::QueuedConnection);
        return;
    }

    QList<FsEntry> entries;
    const QDir d(abs);
    const QFileInfoList infos =
        d.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden, QDir::NoSort);
    for (const QFileInfo& fi : infos) {
        FsEntry e;
        e.name = fi.fileName();
        e.path = dir.isEmpty() ? e.name : (dir + QLatin1Char('/') + e.name);
        e.isDir = fi.isDir();
        e.symlink = fi.isSymLink();
        e.ignored = isIgnored(e.name) || e.name.startsWith(QLatin1Char('.'));
        e.size = fi.size();
        e.mtimeMs = fi.lastModified().toMSecsSinceEpoch();
        entries.push_back(e);
    }
    std::ranges::sort(entries, entryLess);

    QMetaObject::invokeMethod(
        this, [this, rootId, dir, entries] { emit listed(rootId, dir, entries); },
        Qt::QueuedConnection);
}

void LocalDiskFsService::stat(const QString& rootId, const QString& path) {
    const QString abs = resolve(rootId, path);
    const QFileInfo fi(abs);
    FsEntry e;
    bool ok = !abs.isEmpty() && fi.exists();
    if (ok) {
        e.name = fi.fileName();
        e.path = path;
        e.isDir = fi.isDir();
        e.symlink = fi.isSymLink();
        e.ignored = isIgnored(e.name) || e.name.startsWith(QLatin1Char('.'));
        e.size = fi.size();
        e.mtimeMs = fi.lastModified().toMSecsSinceEpoch();
    }
    QMetaObject::invokeMethod(
        this, [this, rootId, path, e, ok] { emit statResult(rootId, path, e, ok); },
        Qt::QueuedConnection);
}

void LocalDiskFsService::read(const QString& rootId, const QString& path) {
    const QString abs = resolve(rootId, path);
    if (abs.isEmpty()) {
        QMetaObject::invokeMethod(
            this,
            [this, rootId, path] { emit error(rootId, path, QStringLiteral("invalid path")); },
            Qt::QueuedConnection);
        return;
    }
    QFile f(abs);
    if (!f.open(QIODevice::ReadOnly)) {
        QMetaObject::invokeMethod(
            this, [this, rootId, path] { emit error(rootId, path, QStringLiteral("cannot open")); },
            Qt::QueuedConnection);
        return;
    }
    const bool truncated = f.size() > kMaxReadBytes;
    const QByteArray bytes = f.read(kMaxReadBytes);
    f.close();
    const bool binary = looksBinary(bytes);
    const QString rev = revisionFor(abs);
    QMetaObject::invokeMethod(
        this,
        [this, rootId, path, bytes, rev, binary, truncated] {
            emit fileRead(rootId, path, bytes, rev, binary, truncated);
        },
        Qt::QueuedConnection);
}

void LocalDiskFsService::write(const QString& rootId, const QString& path, const QByteArray& bytes,
                               const QString& baseRevision, bool force) {
    const QString abs = resolve(rootId, path);
    if (abs.isEmpty()) {
        QMetaObject::invokeMethod(
            this,
            [this, rootId, path] {
                emit writeResult(rootId, path, false, QString(), QStringLiteral("invalid path"));
            },
            Qt::QueuedConnection);
        return;
    }
    // Optimistic concurrency: reject if the on-disk revision moved out from
    // under a non-empty base (empty base = unconditional / new file).
    if (!force && !baseRevision.isEmpty()) {
        const QString current = revisionFor(abs);
        if (!current.isEmpty() && current != baseRevision) {
            QMetaObject::invokeMethod(
                this,
                [this, rootId, path] {
                    emit writeResult(rootId, path, false, QString(),
                                     QStringLiteral("stale: file changed on disk"));
                },
                Qt::QueuedConnection);
            return;
        }
    }
    QSaveFile f(abs);
    if (!f.open(QIODevice::WriteOnly) || f.write(bytes) != bytes.size() || !f.commit()) {
        QMetaObject::invokeMethod(
            this,
            [this, rootId, path] {
                emit writeResult(rootId, path, false, QString(), QStringLiteral("write failed"));
            },
            Qt::QueuedConnection);
        return;
    }
    const QString rev = revisionFor(abs);
    QMetaObject::invokeMethod(
        this, [this, rootId, path, rev] { emit writeResult(rootId, path, true, rev, QString()); },
        Qt::QueuedConnection);
}

void LocalDiskFsService::search(const QString& rootId, const QString& query,
                                const QVariantMap& opts) {
    const QString abs = resolve(rootId, QString());
    QList<FsSearchHit> hits;
    if (abs.isEmpty() || query.isEmpty()) {
        QMetaObject::invokeMethod(
            this, [this, rootId, query, hits] { emit searchResults(rootId, query, hits, true); },
            Qt::QueuedConnection);
        return;
    }
    const bool regex = opts.value(QStringLiteral("regex"), false).toBool();
    const bool caseSensitive = opts.value(QStringLiteral("caseSensitive"), false).toBool();
    const int maxResults = opts.value(QStringLiteral("maxResults"), kMaxSearchHits).toInt();
    QRegularExpression re;
    if (regex) {
        re.setPattern(query);
        if (!caseSensitive)
            re.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        if (!re.isValid()) {
            QMetaObject::invokeMethod(
                this,
                [this, rootId, query, hits] { emit searchResults(rootId, query, hits, true); },
                Qt::QueuedConnection);
            return;
        }
    }
    const Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    int filesScanned = 0;
    QDirIterator it(abs, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (it.hasNext() && filesScanned < kMaxSearchFiles && hits.size() < maxResults) {
        const QString file = it.next();
        const QFileInfo fi = it.fileInfo();
        // Skip ignored ancestors and oversized files.
        bool skip = fi.size() > kMaxSearchFileBytes;
        for (const QString& part :
             fi.absolutePath().mid(abs.size()).split(QLatin1Char('/'), Qt::SkipEmptyParts)) {
            if (isIgnored(part)) {
                skip = true;
                break;
            }
        }
        if (skip)
            continue;
        ++filesScanned;
        QFile f(file);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        const QByteArray head = f.peek(4096);
        if (looksBinary(head)) {
            f.close();
            continue;
        }
        QString rootRel;
        {
            QString rid;
            if (!relativize(file, rid, rootRel))
                rootRel = file;
        }
        int lineNo = 0;
        while (!f.atEnd() && hits.size() < maxResults) {
            ++lineNo;
            const QString line = QString::fromUtf8(f.readLine()).trimmed();
            int col = -1;
            if (regex) {
                const QRegularExpressionMatch m = re.match(line);
                if (m.hasMatch())
                    col = static_cast<int>(m.capturedStart(0));
            } else {
                col = static_cast<int>(line.indexOf(query, 0, cs));
            }
            if (col >= 0)
                hits.push_back(FsSearchHit{rootRel, lineNo, col + 1, line});
        }
        f.close();
    }
    QMetaObject::invokeMethod(
        this, [this, rootId, query, hits] { emit searchResults(rootId, query, hits, true); },
        Qt::QueuedConnection);
}

void LocalDiskFsService::watch(const QString& rootId, const QString& dir) {
    const QString abs = resolve(rootId, dir);
    if (abs.isEmpty() || m_watched.contains(abs))
        return;
    if (m_watcher->addPath(abs))
        m_watched.insert(abs, qMakePair(rootId, dir));
}

void LocalDiskFsService::unwatch(const QString& rootId, const QString& dir) {
    const QString abs = resolve(rootId, dir);
    if (abs.isEmpty())
        return;
    if (m_watched.remove(abs) > 0)
        m_watcher->removePath(abs);
}

} // namespace fs
