// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "tab_model.h"

#include <algorithm>

TabModel::TabModel(QObject* parent) : QAbstractListModel(parent) {}

void TabModel::setCurrentIndex(int index) {
    if (index < 0 || index >= m_tabs.size() || index == m_currentIndex) {
        return;
    }
    setCurrentInternal(index);
}

int TabModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_tabs.size());
}

QVariant TabModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_tabs.size()) {
        return {};
    }
    const Tab& tab = m_tabs.at(index.row());
    switch (role) {
    case TabIdRole:
        return tab.id;
    case KindRole:
        return tab.kind;
    case TitleRole:
    case Qt::DisplayRole:
        return tab.title;
    case SessionIdRole:
        return tab.sessionId;
    case ClosableRole:
        return tab.closable;
    case CurrentRole:
        return index.row() == m_currentIndex;
    case PreviewRole:
        return tab.preview;
    case FilePathRole:
        return tab.path;
    case FileRootRole:
        return tab.rootId;
    case DirtyRole:
        return tab.dirty;
    case ProfileRole:
        return tab.profile;
    default:
        return {};
    }
}

QHash<int, QByteArray> TabModel::roleNames() const {
    return {
        {TabIdRole, "tabId"},         {KindRole, "kind"},         {TitleRole, "title"},
        {SessionIdRole, "sessionId"}, {ClosableRole, "closable"}, {CurrentRole, "current"},
        {PreviewRole, "preview"},     {FilePathRole, "filePath"}, {FileRootRole, "fileRoot"},
        {DirtyRole, "dirty"},         {ProfileRole, "profile"},
    };
}

int TabModel::openTranscript(const QString& sessionId, const QString& title) {
    const int existing = findTranscriptRow(sessionId);
    if (existing >= 0) {
        // Reuse the open tab; refresh its title if the caller supplies a new one,
        // and pin it (a deliberate open promotes a preview tab to permanent).
        QVector<int> roles;
        if (!title.isEmpty() && m_tabs.at(existing).title != title) {
            m_tabs[existing].title = title;
            roles << TitleRole << Qt::DisplayRole;
        }
        if (m_tabs.at(existing).preview) {
            m_tabs[existing].preview = false;
            roles << PreviewRole;
        }
        if (!roles.isEmpty()) {
            const QModelIndex idx = index(existing, 0);
            emit dataChanged(idx, idx, roles);
        }
        activate(existing);
        return m_tabs.at(existing).id;
    }

    const int row = static_cast<int>(m_tabs.size());
    beginInsertRows({}, row, row);
    Tab tab;
    tab.id = m_nextId++;
    tab.kind = Transcript;
    tab.title = title.isEmpty() ? tr("Session") : title;
    tab.sessionId = sessionId;
    tab.closable = true;
    tab.preview = false;
    m_tabs.append(tab);
    endInsertRows();
    emit countChanged();

    setCurrentInternal(row);
    return tab.id;
}

int TabModel::openTranscriptPinned(const QString& sessionId, const QString& title) {
    return openTranscript(sessionId, title);
}

int TabModel::previewTranscript(const QString& sessionId, const QString& title) {
    // Already open anywhere: just activate it (do not change its pinned state).
    const int existing = findTranscriptRow(sessionId);
    if (existing >= 0) {
        activate(existing);
        return m_tabs.at(existing).id;
    }

    // Reuse the single preview slot, reassigning its session in place.
    const int previewRow = findPreviewRow();
    if (previewRow >= 0) {
        Tab& tab = m_tabs[previewRow];
        const bool kindChanged = tab.kind != Transcript;
        tab.kind = Transcript;
        tab.sessionId = sessionId;
        tab.rootId.clear();
        tab.path.clear();
        tab.dirty = false;
        tab.title = title.isEmpty() ? tr("Session") : title;
        const QModelIndex idx = index(previewRow, 0);
        emit dataChanged(idx, idx,
                         {KindRole, TitleRole, Qt::DisplayRole, SessionIdRole, FilePathRole,
                          FileRootRole, DirtyRole});
        if (kindChanged)
            emit tabKindChanged(tab.id);
        emit tabSessionChanged(tab.id, sessionId);
        // Make sure it is the active tab (it may not have been).
        if (m_currentIndex != previewRow) {
            setCurrentInternal(previewRow);
        } else {
            emit currentTabChanged(tab.id);
        }
        return tab.id;
    }

    // No preview slot yet: append a fresh transient tab.
    const int row = static_cast<int>(m_tabs.size());
    beginInsertRows({}, row, row);
    Tab tab;
    tab.id = m_nextId++;
    tab.kind = Transcript;
    tab.title = title.isEmpty() ? tr("Session") : title;
    tab.sessionId = sessionId;
    tab.closable = true;
    tab.preview = true;
    m_tabs.append(tab);
    endInsertRows();
    emit countChanged();

    setCurrentInternal(row);
    return tab.id;
}

void TabModel::pinTab(int index) {
    if (index < 0 || index >= m_tabs.size() || !m_tabs.at(index).preview) {
        return;
    }
    m_tabs[index].preview = false;
    const QModelIndex idx = this->index(index, 0);
    emit dataChanged(idx, idx, {PreviewRole});
}

void TabModel::pinTabById(int tabId) {
    pinTab(indexOfTabId(tabId));
}

void TabModel::pinCurrent() {
    pinTab(m_currentIndex);
}

int TabModel::openPage(int kind, const QString& title) {
    const int existing = findPageRow(kind);
    if (existing >= 0) {
        activate(existing);
        return m_tabs.at(existing).id;
    }

    const int row = static_cast<int>(m_tabs.size());
    beginInsertRows({}, row, row);
    Tab tab;
    tab.id = m_nextId++;
    tab.kind = kind;
    tab.title = title.isEmpty() ? tr("Page") : title;
    tab.sessionId.clear();
    tab.closable = true;
    m_tabs.append(tab);
    endInsertRows();
    emit countChanged();

    setCurrentInternal(row);
    return tab.id;
}

int TabModel::openAgentTab(int kind, const QString& profile, const QString& title) {
    const int existing = findAgentRow(kind, profile);
    if (existing >= 0) {
        if (!title.isEmpty() && m_tabs.at(existing).title != title) {
            m_tabs[existing].title = title;
            const QModelIndex idx = index(existing, 0);
            emit dataChanged(idx, idx, {TitleRole, Qt::DisplayRole});
        }
        activate(existing);
        return m_tabs.at(existing).id;
    }

    const int row = static_cast<int>(m_tabs.size());
    beginInsertRows({}, row, row);
    Tab tab;
    tab.id = m_nextId++;
    tab.kind = kind;
    tab.title = title.isEmpty() ? tr("Agent") : title;
    tab.sessionId.clear();
    tab.closable = true;
    tab.profile = profile;
    m_tabs.append(tab);
    endInsertRows();
    emit countChanged();

    setCurrentInternal(row);
    return tab.id;
}

QString TabModel::agentRefAt(int index) const {
    return (index >= 0 && index < m_tabs.size()) ? m_tabs.at(index).profile : QString();
}

int TabModel::previewFile(const QString& rootId, const QString& path, const QString& title) {
    const QString label = title.isEmpty() ? path.section(QLatin1Char('/'), -1) : title;

    const int existing = findFileRow(rootId, path);
    if (existing >= 0) {
        activate(existing);
        return m_tabs.at(existing).id;
    }

    // Reuse the single preview slot (it may currently hold a transcript preview;
    // the page Loader keys on KindRole, so it reloads as a FilePage in place).
    const int previewRow = findPreviewRow();
    if (previewRow >= 0) {
        Tab& tab = m_tabs[previewRow];
        const bool kindChanged = tab.kind != File;
        tab.kind = File;
        tab.sessionId.clear();
        tab.rootId = rootId;
        tab.path = path;
        tab.title = label;
        tab.dirty = false;
        const QModelIndex idx = index(previewRow, 0);
        emit dataChanged(idx, idx,
                         {KindRole, TitleRole, Qt::DisplayRole, SessionIdRole, FilePathRole,
                          FileRootRole, DirtyRole});
        if (kindChanged)
            emit tabKindChanged(tab.id);
        emit tabFileChanged(tab.id, rootId, path);
        if (m_currentIndex != previewRow)
            setCurrentInternal(previewRow);
        else
            emit currentTabChanged(tab.id);
        return tab.id;
    }

    const int row = static_cast<int>(m_tabs.size());
    beginInsertRows({}, row, row);
    Tab tab;
    tab.id = m_nextId++;
    tab.kind = File;
    tab.title = label;
    tab.rootId = rootId;
    tab.path = path;
    tab.preview = true;
    m_tabs.append(tab);
    endInsertRows();
    emit countChanged();
    setCurrentInternal(row);
    return tab.id;
}

int TabModel::openFilePinned(const QString& rootId, const QString& path, const QString& title) {
    const int existing = findFileRow(rootId, path);
    if (existing >= 0) {
        if (m_tabs.at(existing).preview) {
            m_tabs[existing].preview = false;
            const QModelIndex idx = index(existing, 0);
            emit dataChanged(idx, idx, {PreviewRole});
        }
        activate(existing);
        return m_tabs.at(existing).id;
    }
    const int id = previewFile(rootId, path, title);
    pinTabById(id);
    return id;
}

void TabModel::setDirtyById(int tabId, bool dirty) {
    const int row = indexOfTabId(tabId);
    if (row < 0 || m_tabs.at(row).dirty == dirty)
        return;
    m_tabs[row].dirty = dirty;
    const QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {DirtyRole});
}

QString TabModel::filePathAt(int index) const {
    return (index >= 0 && index < m_tabs.size()) ? m_tabs.at(index).path : QString();
}

QString TabModel::fileRootAt(int index) const {
    return (index >= 0 && index < m_tabs.size()) ? m_tabs.at(index).rootId : QString();
}

void TabModel::closeTab(int index) {
    if (index < 0 || index >= m_tabs.size() || !m_tabs.at(index).closable) {
        return;
    }
    const int closedId = m_tabs.at(index).id;

    beginRemoveRows({}, index, index);
    m_tabs.removeAt(index);
    endRemoveRows();
    emit countChanged();
    emit tabClosed(closedId);

    // Re-anchor the active tab. Removing a row before the active one shifts it
    // left by one; removing the active row keeps the same slot (the right-hand
    // neighbour slides in), clamped to the new last row. The selection signal
    // always fires because the underlying active tab identity changed or the
    // index moved.
    if (m_tabs.isEmpty()) {
        m_currentIndex = -1;
        emit currentIndexChanged();
        emit currentTabChanged(-1);
        return;
    }
    const int last = static_cast<int>(m_tabs.size()) - 1;
    int next = m_currentIndex;
    if (index < m_currentIndex) {
        next = m_currentIndex - 1;
    }
    next = std::min(next, last);
    next = std::max(next, 0);
    // Force a refresh: even when the numeric index is unchanged the tab under it
    // is now a different one, so re-emit and repaint the CurrentRole.
    m_currentIndex = -1;
    setCurrentInternal(next);
}

void TabModel::closeTabById(int tabId) {
    closeTab(indexOfTabId(tabId));
}

void TabModel::activate(int index) {
    if (index < 0 || index >= m_tabs.size() || index == m_currentIndex) {
        return;
    }
    setCurrentInternal(index);
}

void TabModel::cycle(int delta) {
    if (m_tabs.isEmpty() || delta == 0) {
        return;
    }
    const int n = static_cast<int>(m_tabs.size());
    int next = ((m_currentIndex + delta) % n + n) % n;
    if (next != m_currentIndex) {
        setCurrentInternal(next);
    }
}

void TabModel::moveTab(int from, int to) {
    if (from < 0 || from >= m_tabs.size() || to < 0 || to >= m_tabs.size() || from == to) {
        return;
    }
    const int activeId = (m_currentIndex >= 0) ? m_tabs.at(m_currentIndex).id : -1;

    // beginMoveRows wants the destination row in pre-move coordinates: when
    // moving down, that is to + 1.
    const int destination = to > from ? to + 1 : to;
    beginMoveRows({}, from, from, {}, destination);
    m_tabs.move(from, to);
    endMoveRows();

    // The active tab keeps its identity; recompute its row so currentIndex still
    // points at the same tab (no currentTabChanged - the active tab is the same).
    if (activeId >= 0) {
        const int row = indexOfTabId(activeId);
        if (row != m_currentIndex) {
            m_currentIndex = row;
            emit currentIndexChanged();
            emitCurrentChanged();
        }
    }
}

void TabModel::setTitle(int index, const QString& title) {
    if (index < 0 || index >= m_tabs.size() || m_tabs.at(index).title == title) {
        return;
    }
    m_tabs[index].title = title;
    const QModelIndex idx = this->index(index, 0);
    emit dataChanged(idx, idx, {TitleRole, Qt::DisplayRole});
}

int TabModel::tabIdAt(int index) const {
    if (index < 0 || index >= m_tabs.size()) {
        return -1;
    }
    return m_tabs.at(index).id;
}

int TabModel::kindAt(int index) const {
    if (index < 0 || index >= m_tabs.size()) {
        return -1;
    }
    return m_tabs.at(index).kind;
}

QString TabModel::sessionIdAt(int index) const {
    if (index < 0 || index >= m_tabs.size()) {
        return {};
    }
    return m_tabs.at(index).sessionId;
}

QString TabModel::titleAt(int index) const {
    if (index < 0 || index >= m_tabs.size()) {
        return {};
    }
    return m_tabs.at(index).title;
}

bool TabModel::isPreviewAt(int index) const {
    if (index < 0 || index >= m_tabs.size()) {
        return false;
    }
    return m_tabs.at(index).preview;
}

int TabModel::indexOfTabId(int tabId) const {
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs.at(i).id == tabId) {
            return i;
        }
    }
    return -1;
}

void TabModel::setCurrentInternal(int index) {
    m_currentIndex = index;
    emit currentIndexChanged();
    emit currentTabChanged(m_tabs.at(index).id);
    emitCurrentChanged();
}

void TabModel::emitCurrentChanged() {
    if (m_tabs.isEmpty()) {
        return;
    }
    emit dataChanged(index(0, 0), index(static_cast<int>(m_tabs.size()) - 1, 0), {CurrentRole});
}

int TabModel::findTranscriptRow(const QString& sessionId) const {
    if (sessionId.isEmpty()) {
        return -1;
    }
    for (int i = 0; i < m_tabs.size(); ++i) {
        const Tab& tab = m_tabs.at(i);
        if (tab.kind == Transcript && tab.sessionId == sessionId) {
            return i;
        }
    }
    return -1;
}

int TabModel::findPageRow(int kind) const {
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs.at(i).kind == kind) {
            return i;
        }
    }
    return -1;
}

int TabModel::findPreviewRow() const {
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs.at(i).preview && !m_tabs.at(i).dirty) {
            return i;
        }
    }
    return -1;
}

int TabModel::findFileRow(const QString& rootId, const QString& path) const {
    for (int i = 0; i < m_tabs.size(); ++i) {
        const Tab& tab = m_tabs.at(i);
        if (tab.kind == File && tab.rootId == rootId && tab.path == path) {
            return i;
        }
    }
    return -1;
}

int TabModel::findAgentRow(int kind, const QString& profile) const {
    for (int i = 0; i < m_tabs.size(); ++i) {
        const Tab& tab = m_tabs.at(i);
        if (tab.kind == kind && tab.profile == profile) {
            return i;
        }
    }
    return -1;
}
