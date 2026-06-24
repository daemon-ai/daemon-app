#include "fs_explorer_model.h"

#include "fs/ifs_service.h"

namespace files {

FsExplorerModel::FsExplorerModel(QObject* parent)
    : QAbstractListModel(parent)
{
    fs::registerFsMetatypes();
}

QObject* FsExplorerModel::service() const { return m_service; }

void FsExplorerModel::setService(QObject* service)
{
    auto* svc = qobject_cast<fs::IFsService*>(service);
    if (svc == m_service)
        return;
    if (m_service)
        m_service->disconnect(this);
    m_service = svc;
    emit serviceChanged();
    beginResetModel();
    m_roots.clear();
    m_rows.clear();
    m_cache.clear();
    m_expanded.clear();
    m_pending.clear();
    m_currentKey.clear();
    endResetModel();
    emit treeChanged();
    if (m_service) {
        connect(m_service, &fs::IFsService::rootsChanged, this, &FsExplorerModel::onRootsChanged);
        connect(m_service, &fs::IFsService::listed, this, &FsExplorerModel::onListed);
        connect(m_service, &fs::IFsService::changed, this, &FsExplorerModel::onChanged);
        connect(m_service, &fs::IFsService::error, this, &FsExplorerModel::onError);
        m_service->listRoots();
    }
}

void FsExplorerModel::setShowIgnored(bool show)
{
    if (m_showIgnored == show)
        return;
    m_showIgnored = show;
    m_cache.clear();
    emit showIgnoredChanged();
    rebuildFromRoots();
}

int FsExplorerModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant FsExplorerModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const Row& r = m_rows.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case LabelRole:
        return r.label;
    case PathRole:
        return r.path;
    case RootIdRole:
        return r.rootId;
    case DepthRole:
        return r.depth;
    case IsDirRole:
        return r.isDir;
    case IsRootRole:
        return r.isRoot;
    case IgnoredRole:
        return r.ignored;
    case HasChildrenRole:
        return r.hasChildren;
    case ExpandedRole:
        return r.expanded;
    case CurrentRole:
        return key(r.rootId, r.path) == m_currentKey;
    case LoadingRole:
        return r.loading;
    case ErrorRole:
        return r.error;
    default:
        return {};
    }
}

QHash<int, QByteArray> FsExplorerModel::roleNames() const
{
    return {
        { LabelRole, "label" },       { PathRole, "path" },
        { RootIdRole, "rootId" },     { DepthRole, "depth" },
        { IsDirRole, "isDir" },       { IsRootRole, "isRoot" },
        { IgnoredRole, "ignored" },   { HasChildrenRole, "hasChildren" },
        { ExpandedRole, "expanded" }, { CurrentRole, "current" },
        { LoadingRole, "loading" },   { ErrorRole, "error" },
    };
}

QString FsExplorerModel::key(const QString& rootId, const QString& path)
{
    return rootId + QChar(0x1f) + path;
}

int FsExplorerModel::rowForKey(const QString& rowKey) const
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (key(m_rows.at(i).rootId, m_rows.at(i).path) == rowKey)
            return i;
    }
    return -1;
}

int FsExplorerModel::subtreeEnd(int row) const
{
    if (row < 0 || row >= m_rows.size())
        return row;
    const int depth = m_rows.at(row).depth;
    int end = row + 1;
    while (end < m_rows.size() && m_rows.at(end).depth > depth)
        ++end;
    return end;
}

void FsExplorerModel::setCurrentKey(const QString& rowKey)
{
    if (m_currentKey == rowKey)
        return;
    const int old = rowForKey(m_currentKey);
    m_currentKey = rowKey;
    const int now = rowForKey(m_currentKey);
    if (old >= 0)
        emit dataChanged(index(old), index(old), { CurrentRole });
    if (now >= 0)
        emit dataChanged(index(now), index(now), { CurrentRole });
}

void FsExplorerModel::activate(int row)
{
    if (row < 0 || row >= m_rows.size())
        return;
    const Row& r = m_rows.at(row);
    setCurrentKey(key(r.rootId, r.path));
    if (r.isDir) {
        emit directoryActivated(r.rootId, r.path);
        toggleExpand(row);
    } else {
        emit fileActivated(r.rootId, r.path);
    }
}

void FsExplorerModel::toggleExpand(int row)
{
    if (row < 0 || row >= m_rows.size() || !m_rows.at(row).isDir)
        return;
    const QString rowKey = key(m_rows.at(row).rootId, m_rows.at(row).path);
    if (m_expanded.contains(rowKey)) {
        m_expanded.remove(rowKey);
        m_rows[row].expanded = false;
        removeChildren(row);
        emit dataChanged(index(row), index(row), { ExpandedRole });
        emit treeChanged();
        return;
    }
    m_expanded.insert(rowKey);
    m_rows[row].expanded = true;
    emit dataChanged(index(row), index(row), { ExpandedRole });
    if (m_cache.contains(rowKey))
        insertChildren(row, m_cache.value(rowKey));
    else
        requestChildren(row);
    emit treeChanged();
}

void FsExplorerModel::requestChildren(int row)
{
    if (!m_service || row < 0 || row >= m_rows.size())
        return;
    Row& r = m_rows[row];
    const QString rowKey = key(r.rootId, r.path);
    if (m_pending.contains(rowKey))
        return;
    m_pending.insert(rowKey);
    r.loading = true;
    r.error.clear();
    emit dataChanged(index(row), index(row), { LoadingRole, ErrorRole });
    m_service->open(r.rootId, r.path);
}

void FsExplorerModel::removeChildren(int row)
{
    const int end = subtreeEnd(row);
    if (end <= row + 1)
        return;
    beginRemoveRows({}, row + 1, end - 1);
    m_rows.erase(m_rows.begin() + row + 1, m_rows.begin() + end);
    endRemoveRows();
}

void FsExplorerModel::insertChildren(int row, const QList<fs::FsEntry>& entries)
{
    if (row < 0 || row >= m_rows.size())
        return;
    removeChildren(row);
    QList<Row> children;
    children.reserve(entries.size());
    for (const fs::FsEntry& e : entries) {
        if (e.ignored && !m_showIgnored)
            continue;
        Row child;
        child.rootId = m_rows.at(row).rootId;
        child.path = e.path;
        child.label = e.name;
        child.depth = m_rows.at(row).depth + 1;
        child.isDir = e.isDir;
        child.ignored = e.ignored;
        child.hasChildren = e.isDir;
        child.expanded = m_expanded.contains(key(child.rootId, child.path));
        children.push_back(child);
    }
    if (children.isEmpty())
        return;
    beginInsertRows({}, row + 1, row + children.size());
    for (int i = 0; i < children.size(); ++i)
        m_rows.insert(row + 1 + i, children.at(i));
    endInsertRows();
}

void FsExplorerModel::expandAll()
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows.at(i).isDir && !m_rows.at(i).expanded)
            toggleExpand(i);
    }
}

void FsExplorerModel::collapseAll()
{
    beginResetModel();
    m_expanded.clear();
    for (Row& r : m_rows)
        r.expanded = false;
    rebuildFromRoots();
    endResetModel();
    emit treeChanged();
}

int FsExplorerModel::currentRow() const { return rowForKey(m_currentKey); }
QString FsExplorerModel::rootIdAt(int row) const { return row >= 0 && row < m_rows.size() ? m_rows.at(row).rootId : QString(); }
QString FsExplorerModel::pathAt(int row) const { return row >= 0 && row < m_rows.size() ? m_rows.at(row).path : QString(); }
bool FsExplorerModel::isDirAt(int row) const { return row >= 0 && row < m_rows.size() && m_rows.at(row).isDir; }

void FsExplorerModel::rebuildFromRoots()
{
    beginResetModel();
    m_rows.clear();
    for (const fs::FsRoot& root : std::as_const(m_roots)) {
        Row r;
        r.rootId = root.id;
        r.label = root.label;
        r.isDir = true;
        r.isRoot = true;
        r.hasChildren = true;
        r.expanded = m_expanded.contains(key(r.rootId, r.path));
        m_rows.push_back(r);
    }
    endResetModel();
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows.at(i).expanded)
            requestChildren(i);
    }
    emit treeChanged();
}

void FsExplorerModel::onRootsChanged(const QList<fs::FsRoot>& roots)
{
    m_roots = roots;
    rebuildFromRoots();
}

void FsExplorerModel::onListed(const QString& rootId, const QString& dir,
                               const QList<fs::FsEntry>& entries)
{
    const QString rowKey = key(rootId, dir);
    if (!m_pending.remove(rowKey) && !m_expanded.contains(rowKey))
        return;
    m_cache.insert(rowKey, entries);
    const int row = rowForKey(rowKey);
    if (row < 0)
        return;
    m_rows[row].loading = false;
    m_rows[row].error.clear();
    emit dataChanged(index(row), index(row), { LoadingRole, ErrorRole });
    if (m_rows.at(row).expanded)
        insertChildren(row, entries);
}

void FsExplorerModel::onChanged(const QString& rootId, const QString& dir)
{
    const QString rowKey = key(rootId, dir);
    m_cache.remove(rowKey);
    const int row = rowForKey(rowKey);
    if (row >= 0 && m_rows.at(row).expanded)
        requestChildren(row);
}

void FsExplorerModel::onError(const QString& rootId, const QString& path, const QString& message)
{
    const QString rowKey = key(rootId, path);
    m_pending.remove(rowKey);
    const int row = rowForKey(rowKey);
    if (row < 0)
        return;
    m_rows[row].loading = false;
    m_rows[row].error = message;
    emit dataChanged(index(row), index(row), { LoadingRole, ErrorRole });
}

} // namespace files
