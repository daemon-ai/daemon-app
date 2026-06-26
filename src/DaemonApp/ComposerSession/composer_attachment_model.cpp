#include "composer_attachment_model.h"

#include <QStringList>

ComposerAttachmentModel::ComposerAttachmentModel(QObject* parent) : QAbstractListModel(parent) {}

int ComposerAttachmentModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant ComposerAttachmentModel::data(const QModelIndex& index, int role) const {
    if (index.row() < 0 || index.row() >= m_items.size()) {
        return {};
    }
    const Attachment& a = m_items.at(index.row());
    switch (role) {
    case NameRole:
        return a.name;
    case KindRole:
        return a.kind;
    default:
        return {};
    }
}

QHash<int, QByteArray> ComposerAttachmentModel::roleNames() const {
    return {
        {NameRole, "name"},
        {KindRole, "kind"},
    };
}

void ComposerAttachmentModel::append(const QString& name, const QString& kind) {
    const int row = static_cast<int>(m_items.size());
    beginInsertRows({}, row, row);
    m_items.push_back({name, kind});
    endInsertRows();
    emit countChanged();
}

void ComposerAttachmentModel::removeAt(int index) {
    if (index < 0 || index >= m_items.size()) {
        return;
    }
    beginRemoveRows({}, index, index);
    m_items.removeAt(index);
    endRemoveRows();
    emit countChanged();
}

void ComposerAttachmentModel::clear() {
    if (m_items.isEmpty()) {
        return;
    }
    beginResetModel();
    m_items.clear();
    endResetModel();
    emit countChanged();
}

QString ComposerAttachmentModel::buildRefs() const {
    QStringList refs;
    refs.reserve(static_cast<int>(m_items.size()));
    for (const Attachment& a : m_items) {
        QString prefix;
        if (a.kind == QStringLiteral("image")) {
            prefix = QStringLiteral("@image:");
        } else if (a.kind == QStringLiteral("folder")) {
            prefix = QStringLiteral("@folder:");
        } else if (a.kind == QStringLiteral("url")) {
            prefix = QStringLiteral("@url:");
        } else {
            prefix = QStringLiteral("@file:");
        }
        refs.push_back(prefix + a.name);
    }
    return refs.join(QLatin1Char(' '));
}
