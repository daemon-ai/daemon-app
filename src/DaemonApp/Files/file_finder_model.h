// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "fs/fs_dtos.h"

#include <QAbstractListModel>
#include <QList>
#include <QQueue>
#include <QSet>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace fs {
class IFsService;
}

namespace files {

// Ctrl+P fuzzy file finder. Builds an incremental file index by walking the
// node through the IFsService seam (open() per directory, with file-count and
// time-ish caps), then ranks candidates against the query with an end-anchored
// fuzzy match (ported from Lite XL's system.c f_fuzzy_match, MIT) merged with a
// most-recently-used list. No local filesystem access lives here.
class FileFinderModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* service READ service WRITE setService NOTIFY serviceChanged)
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(bool indexing READ indexing NOTIFY indexingChanged)
    Q_PROPERTY(int fileCount READ fileCount NOTIFY indexingChanged)

public:
    enum Role {
        PathRole = Qt::UserRole + 1,
        NameRole,
        RootIdRole,
    };
    Q_ENUM(Role)

    explicit FileFinderModel(QObject* parent = nullptr);
    ~FileFinderModel() override;

    [[nodiscard]] QObject* service() const;
    void setService(QObject* service);
    [[nodiscard]] QString query() const { return m_query; }
    void setQuery(const QString& query);
    [[nodiscard]] bool indexing() const { return m_indexing; }
    [[nodiscard]] int fileCount() const { return static_cast<int>(m_index.size()); }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // (Re)build the file index from the connected node's roots.
    Q_INVOKABLE void rebuildIndex();
    // Record a chosen path so it ranks first on the next empty/þartial query.
    Q_INVOKABLE void noteRecent(const QString& rootId, const QString& path);

    // End-anchored fuzzy score; returns false when `pattern` is not fully
    // consumed. Higher score = better. Exposed for reuse/testing.
    static bool fuzzyScore(const QString& text, const QString& pattern, int* score);

signals:
    void serviceChanged();
    void queryChanged();
    void indexingChanged();

private slots:
    void onListed(const QString& rootId, const QString& dir, const QList<fs::FsEntry>& entries);
    void onRootsChanged(const QList<fs::FsRoot>& roots);
    void onError(const QString& rootId, const QString& path, const QString& message);

private:
    struct File {
        QString rootId;
        QString path;
        QString name;
    };

    void enqueueDir(const QString& rootId, const QString& dir);
    void pump();
    void rerank();

    fs::IFsService* m_service = nullptr;
    QString m_query;
    bool m_indexing = false;
    QList<File> m_index;                        // all known files
    QList<int> m_results;                       // indices into m_index, ranked
    QQueue<QPair<QString, QString>> m_dirQueue; // (rootId, dir) to walk
    QSet<QString> m_pendingDirs;                // request keys this model owns
    QSet<QString> m_seenKeys;                   // indexed file keys for this rebuild
    int m_inFlight = 0;
    int m_filesSeen = 0;
    QStringList m_recents; // "rootId\u001fpath", most recent first
};

// Project-wide content search results (server-side search via IFsService). A
// flat list of hits the SearchResults view renders; clicking a hit opens the
// file at the line.
class SearchResultsModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* service READ service WRITE setService NOTIFY serviceChanged)
    Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)
    Q_PROPERTY(QString query READ query NOTIFY queryChanged)

public:
    enum Role {
        PathRole = Qt::UserRole + 1,
        LineRole,
        ColumnRole,
        PreviewRole,
        RootIdRole,
    };
    Q_ENUM(Role)

    explicit SearchResultsModel(QObject* parent = nullptr);
    ~SearchResultsModel() override;

    [[nodiscard]] QObject* service() const;
    void setService(QObject* service);
    [[nodiscard]] bool searching() const { return m_searching; }
    [[nodiscard]] QString query() const { return m_query; }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Run a project search under `rootId` (opts: regex, caseSensitive, maxResults).
    Q_INVOKABLE void search(const QString& rootId, const QString& query,
                            const QVariantMap& opts = {});

signals:
    void serviceChanged();
    void searchingChanged();
    void queryChanged();

private slots:
    void onSearchResults(const QString& rootId, const QString& query,
                         const QList<fs::FsSearchHit>& hits, bool complete);

private:
    fs::IFsService* m_service = nullptr;
    bool m_searching = false;
    QString m_query;
    QString m_rootId;
    QList<fs::FsSearchHit> m_hits;
};

} // namespace files
