// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "fs/fs_dtos.h"

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace fs {
class IFsService;
}

namespace files {

class FsExplorerModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* service READ service WRITE setService NOTIFY serviceChanged)
    Q_PROPERTY(bool showIgnored READ showIgnored WRITE setShowIgnored NOTIFY showIgnoredChanged)
    Q_PROPERTY(bool anyExpanded READ anyExpanded NOTIFY treeChanged)

public:
    enum Role {
        LabelRole = Qt::UserRole + 1,
        PathRole,
        RootIdRole,
        DepthRole,
        IsDirRole,
        IsRootRole,
        IgnoredRole,
        HasChildrenRole,
        ExpandedRole,
        CurrentRole,
        LoadingRole,
        ErrorRole,
    };
    Q_ENUM(Role)

    explicit FsExplorerModel(QObject* parent = nullptr);

    [[nodiscard]] QObject* service() const;
    void setService(QObject* service);
    [[nodiscard]] bool showIgnored() const { return m_showIgnored; }
    void setShowIgnored(bool show);
    [[nodiscard]] bool anyExpanded() const { return !m_expanded.isEmpty(); }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void activate(int row);
    Q_INVOKABLE void toggleExpand(int row);
    Q_INVOKABLE void expandAll();
    Q_INVOKABLE void collapseAll();
    Q_INVOKABLE int currentRow() const;
    Q_INVOKABLE QString rootIdAt(int row) const;
    Q_INVOKABLE QString pathAt(int row) const;
    Q_INVOKABLE bool isDirAt(int row) const;

signals:
    void serviceChanged();
    void showIgnoredChanged();
    void treeChanged();
    void fileActivated(const QString& rootId, const QString& path);
    void directoryActivated(const QString& rootId, const QString& path);

private:
    struct Row {
        QString rootId;
        QString path;
        QString label;
        int depth = 0;
        bool isDir = false;
        bool isRoot = false;
        bool ignored = false;
        bool hasChildren = false;
        bool expanded = false;
        bool loading = false;
        QString error;
    };

    [[nodiscard]] static QString key(const QString& rootId, const QString& path);
    [[nodiscard]] int rowForKey(const QString& rowKey) const;
    [[nodiscard]] int subtreeEnd(int row) const;
    void rebuildFromRoots();
    void requestChildren(int row);
    void removeChildren(int row);
    void insertChildren(int row, const QList<fs::FsEntry>& entries);
    void setCurrentKey(const QString& rowKey);

    void onRootsChanged(const QList<fs::FsRoot>& roots);
    void onListed(const QString& rootId, const QString& dir, const QList<fs::FsEntry>& entries);
    void onChanged(const QString& rootId, const QString& dir);
    void onError(const QString& rootId, const QString& path, const QString& message);

    fs::IFsService* m_service = nullptr;
    bool m_showIgnored = false;
    // Expand each root's first level once, on the initial roots load, so the tree opens to its
    // top-level entries instead of a bare collapsed root. One-shot so a later user collapseAll (or
    // a roster refresh) is not overridden.
    bool m_autoExpandedRoots = false;
    QList<fs::FsRoot> m_roots;
    QList<Row> m_rows;
    QHash<QString, QList<fs::FsEntry>> m_cache;
    QSet<QString> m_expanded;
    QSet<QString> m_pending;
    QString m_currentKey;
};

} // namespace files
