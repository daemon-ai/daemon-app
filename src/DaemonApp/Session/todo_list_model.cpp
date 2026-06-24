#include "todo_list_model.h"

#include <QVariantMap>

TodoListModel::TodoListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int TodoListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_items.size());
}

QVariant TodoListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return {};
    }
    const Item& item = m_items.at(index.row());
    switch (role) {
    case TextRole:
        return item.text;
    case DoneRole:
        return item.done;
    default:
        return {};
    }
}

QHash<int, QByteArray> TodoListModel::roleNames() const
{
    return {
        { TextRole, QByteArrayLiteral("text") },
        { DoneRole, QByteArrayLiteral("done") },
    };
}

void TodoListModel::setTodos(const QVariantList& items)
{
    beginResetModel();
    m_items.clear();
    m_items.reserve(items.size());
    for (const QVariant& v : items) {
        const QVariantMap map = v.toMap();
        m_items.push_back(Item{ map.value(QStringLiteral("text")).toString(),
                                map.value(QStringLiteral("done")).toBool() });
    }
    endResetModel();
    emit countChanged();
}

void TodoListModel::clear()
{
    if (m_items.isEmpty()) {
        return;
    }
    beginResetModel();
    m_items.clear();
    endResetModel();
    emit countChanged();
}

QString TodoListModel::textAt(int index) const
{
    if (index < 0 || index >= m_items.size()) {
        return {};
    }
    return m_items.at(index).text;
}

bool TodoListModel::doneAt(int index) const
{
    if (index < 0 || index >= m_items.size()) {
        return false;
    }
    return m_items.at(index).done;
}
