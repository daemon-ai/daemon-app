#include "composer_queue_model.h"

ComposerQueueModel::ComposerQueueModel(QObject* parent) : QAbstractListModel(parent) {}

int ComposerQueueModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_entries.size());
}

QVariant ComposerQueueModel::data(const QModelIndex& index, int role) const {
    if (index.row() < 0 || index.row() >= m_entries.size()) {
        return {};
    }
    const Entry& e = m_entries.at(index.row());
    switch (role) {
    case TextRole:
        return e.text;
    case RefsRole:
        return e.refs;
    default:
        return {};
    }
}

QHash<int, QByteArray> ComposerQueueModel::roleNames() const {
    return {
        {TextRole, "text"},
        {RefsRole, "refs"},
    };
}

void ComposerQueueModel::append(const QString& text, const QString& refs) {
    const int row = static_cast<int>(m_entries.size());
    beginInsertRows({}, row, row);
    m_entries.push_back({text, refs});
    endInsertRows();
    emit countChanged();
}

void ComposerQueueModel::setEntry(int index, const QString& text, const QString& refs) {
    if (index < 0 || index >= m_entries.size()) {
        return;
    }
    m_entries[index] = {text, refs};
    const QModelIndex idx = this->index(index);
    emit dataChanged(idx, idx, {TextRole, RefsRole});
}

void ComposerQueueModel::insertAt(int index, const QString& text, const QString& refs) {
    if (index < 0 || index > m_entries.size()) {
        return;
    }
    beginInsertRows({}, index, index);
    m_entries.insert(index, {text, refs});
    endInsertRows();
    emit countChanged();
}

void ComposerQueueModel::removeAt(int index) {
    if (index < 0 || index >= m_entries.size()) {
        return;
    }
    beginRemoveRows({}, index, index);
    m_entries.removeAt(index);
    endRemoveRows();
    emit countChanged();
}

void ComposerQueueModel::clear() {
    if (m_entries.isEmpty()) {
        return;
    }
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}

QString ComposerQueueModel::textAt(int index) const {
    return (index >= 0 && index < m_entries.size()) ? m_entries.at(index).text : QString();
}

QString ComposerQueueModel::refsAt(int index) const {
    return (index >= 0 && index < m_entries.size()) ? m_entries.at(index).refs : QString();
}

void ComposerQueueModel::setEntries(const QList<Entry>& entries) {
    beginResetModel();
    m_entries = entries;
    endResetModel();
    emit countChanged();
}
