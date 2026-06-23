#include "subagent_model.h"

#include <QVariantMap>

SubagentModel::SubagentModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int SubagentModel::runningCount() const
{
    int n = 0;
    for (const Item& item : m_items) {
        if (item.status == QStringLiteral("running")) {
            ++n;
        }
    }
    return n;
}

int SubagentModel::failedCount() const
{
    int n = 0;
    for (const Item& item : m_items) {
        if (item.status == QStringLiteral("error")) {
            ++n;
        }
    }
    return n;
}

int SubagentModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_items.size());
}

QVariant SubagentModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return {};
    }
    const Item& item = m_items.at(index.row());
    switch (role) {
    case IdRole:
        return item.id;
    case TitleRole:
        return item.title;
    case StatusRole:
        return item.status;
    case DetailRole:
        return item.detail;
    default:
        return {};
    }
}

QHash<int, QByteArray> SubagentModel::roleNames() const
{
    return {
        { IdRole, "subId" },
        { TitleRole, "title" },
        { StatusRole, "status" },
        { DetailRole, "detail" },
    };
}

void SubagentModel::applyEvents(const QVariantList& events)
{
    bool changed = false;
    for (const QVariant& v : events) {
        const QVariantMap event = v.toMap();
        if (event.value(QStringLiteral("type")).toString() != QStringLiteral("subagent")) {
            continue;
        }
        const QString id = event.value(QStringLiteral("id")).toString();
        if (id.isEmpty()) {
            continue;
        }
        const QString title = event.value(QStringLiteral("title")).toString();
        const QString status = event.value(QStringLiteral("status")).toString();
        const QString detail = event.value(QStringLiteral("detail")).toString();

        const auto it = m_indexById.constFind(id);
        if (it == m_indexById.constEnd()) {
            const int row = static_cast<int>(m_items.size());
            beginInsertRows({}, row, row);
            m_items.push_back({ id, title, status, detail });
            m_indexById.insert(id, row);
            endInsertRows();
            changed = true;
        } else {
            const int row = it.value();
            Item& item = m_items[row];
            if (!title.isEmpty()) {
                item.title = title;
            }
            if (!status.isEmpty() && status != item.status) {
                item.status = status;
                changed = true;
            }
            item.detail = detail;
            const QModelIndex idx = index(row);
            emit dataChanged(idx, idx, { TitleRole, StatusRole, DetailRole });
        }
    }
    if (changed) {
        emit countChanged();
    }
}

void SubagentModel::clear()
{
    if (m_items.isEmpty()) {
        return;
    }
    beginResetModel();
    m_items.clear();
    m_indexById.clear();
    endResetModel();
    emit countChanged();
}

QString SubagentModel::titleAt(int index) const
{
    if (index < 0 || index >= m_items.size()) {
        return {};
    }
    return m_items.at(index).title;
}

QString SubagentModel::statusAt(int index) const
{
    if (index < 0 || index >= m_items.size()) {
        return {};
    }
    return m_items.at(index).status;
}

QString SubagentModel::detailAt(int index) const
{
    if (index < 0 || index >= m_items.size()) {
        return {};
    }
    return m_items.at(index).detail;
}
