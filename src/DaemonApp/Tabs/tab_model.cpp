#include "tab_model.h"

TabModel::TabModel(QObject* parent) : QAbstractListModel(parent) {}

void TabModel::setCurrentIndex(int index)
{
    if (index < 0 || index >= m_tabs.size() || index == m_currentIndex) {
        return;
    }
    setCurrentInternal(index);
}

int TabModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_tabs.size());
}

QVariant TabModel::data(const QModelIndex& index, int role) const
{
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
    case ConversationIdRole:
        return tab.conversationId;
    case ClosableRole:
        return tab.closable;
    case CurrentRole:
        return index.row() == m_currentIndex;
    default:
        return {};
    }
}

QHash<int, QByteArray> TabModel::roleNames() const
{
    return {
        { TabIdRole, "tabId" },
        { KindRole, "kind" },
        { TitleRole, "title" },
        { ConversationIdRole, "conversationId" },
        { ClosableRole, "closable" },
        { CurrentRole, "current" },
    };
}

int TabModel::openTranscript(int conversationId, const QString& title)
{
    const int existing = findTranscriptRow(conversationId);
    if (existing >= 0) {
        // Reuse the open tab; refresh its title if the caller supplies a new one.
        if (!title.isEmpty() && m_tabs.at(existing).title != title) {
            m_tabs[existing].title = title;
            const QModelIndex idx = index(existing, 0);
            emit dataChanged(idx, idx, { TitleRole, Qt::DisplayRole });
        }
        activate(existing);
        return m_tabs.at(existing).id;
    }

    const int row = static_cast<int>(m_tabs.size());
    beginInsertRows({}, row, row);
    Tab tab;
    tab.id = m_nextId++;
    tab.kind = Transcript;
    tab.title = title.isEmpty() ? QStringLiteral("Conversation") : title;
    tab.conversationId = conversationId;
    tab.closable = true;
    m_tabs.append(tab);
    endInsertRows();
    emit countChanged();

    setCurrentInternal(row);
    return tab.id;
}

int TabModel::openPage(int kind, const QString& title)
{
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
    tab.title = title.isEmpty() ? QStringLiteral("Page") : title;
    tab.conversationId = -1;
    tab.closable = true;
    m_tabs.append(tab);
    endInsertRows();
    emit countChanged();

    setCurrentInternal(row);
    return tab.id;
}

void TabModel::closeTab(int index)
{
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
    if (next > last) {
        next = last;
    }
    if (next < 0) {
        next = 0;
    }
    // Force a refresh: even when the numeric index is unchanged the tab under it
    // is now a different one, so re-emit and repaint the CurrentRole.
    m_currentIndex = -1;
    setCurrentInternal(next);
}

void TabModel::closeTabById(int tabId)
{
    closeTab(indexOfTabId(tabId));
}

void TabModel::activate(int index)
{
    if (index < 0 || index >= m_tabs.size() || index == m_currentIndex) {
        return;
    }
    setCurrentInternal(index);
}

void TabModel::cycle(int delta)
{
    if (m_tabs.isEmpty() || delta == 0) {
        return;
    }
    const int n = static_cast<int>(m_tabs.size());
    int next = ((m_currentIndex + delta) % n + n) % n;
    if (next != m_currentIndex) {
        setCurrentInternal(next);
    }
}

void TabModel::moveTab(int from, int to)
{
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

void TabModel::setTitle(int index, const QString& title)
{
    if (index < 0 || index >= m_tabs.size() || m_tabs.at(index).title == title) {
        return;
    }
    m_tabs[index].title = title;
    const QModelIndex idx = this->index(index, 0);
    emit dataChanged(idx, idx, { TitleRole, Qt::DisplayRole });
}

int TabModel::tabIdAt(int index) const
{
    if (index < 0 || index >= m_tabs.size()) {
        return -1;
    }
    return m_tabs.at(index).id;
}

int TabModel::kindAt(int index) const
{
    if (index < 0 || index >= m_tabs.size()) {
        return -1;
    }
    return m_tabs.at(index).kind;
}

int TabModel::conversationIdAt(int index) const
{
    if (index < 0 || index >= m_tabs.size()) {
        return -1;
    }
    return m_tabs.at(index).conversationId;
}

QString TabModel::titleAt(int index) const
{
    if (index < 0 || index >= m_tabs.size()) {
        return {};
    }
    return m_tabs.at(index).title;
}

int TabModel::indexOfTabId(int tabId) const
{
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs.at(i).id == tabId) {
            return i;
        }
    }
    return -1;
}

void TabModel::setCurrentInternal(int index)
{
    m_currentIndex = index;
    emit currentIndexChanged();
    emit currentTabChanged(m_tabs.at(index).id);
    emitCurrentChanged();
}

void TabModel::emitCurrentChanged()
{
    if (m_tabs.isEmpty()) {
        return;
    }
    emit dataChanged(index(0, 0), index(static_cast<int>(m_tabs.size()) - 1, 0), { CurrentRole });
}

int TabModel::findTranscriptRow(int conversationId) const
{
    if (conversationId < 0) {
        return -1;
    }
    for (int i = 0; i < m_tabs.size(); ++i) {
        const Tab& tab = m_tabs.at(i);
        if (tab.kind == Transcript && tab.conversationId == conversationId) {
            return i;
        }
    }
    return -1;
}

int TabModel::findPageRow(int kind) const
{
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs.at(i).kind == kind) {
            return i;
        }
    }
    return -1;
}
