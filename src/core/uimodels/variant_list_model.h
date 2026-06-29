// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QString>
#include <QVariantMap>

namespace uimodels {

// A generic list model over QVariantMap rows, shared by the mock-backed manager
// surfaces (models hub, accounts, profiles, fleet, cron, ...). Each row is
// exposed whole under the single `entry` role, so QML delegates read
// `entry.<field>` and the same model serves every list without bespoke roles.
// Rows carry an `id` field used for upsert/remove so live updates (download
// progress, status changes) mutate in place and emit dataChanged.
class VariantListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role { EntryRole = Qt::UserRole + 1 };

    using QAbstractListModel::QAbstractListModel;

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override {
        return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
    }

    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
            return {};
        }
        if (role == EntryRole) {
            return m_rows.at(index.row());
        }
        return {};
    }

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override {
        return {{EntryRole, "entry"}};
    }

    [[nodiscard]] int count() const { return static_cast<int>(m_rows.size()); }

    [[nodiscard]] Q_INVOKABLE QVariantMap at(int row) const {
        return (row >= 0 && row < m_rows.size()) ? m_rows.at(row) : QVariantMap{};
    }

    // Replace all rows (used for search results / scope changes).
    void setRows(const QList<QVariantMap>& rows) {
        beginResetModel();
        m_rows = rows;
        endResetModel();
        emit countChanged();
    }

    [[nodiscard]] const QList<QVariantMap>& rows() const { return m_rows; }

    // Insert or update a row matched by its "id" field. Returns the row index.
    int upsert(const QVariantMap& row) {
        const QVariant id = row.value(QStringLiteral("id"));
        for (int i = 0; i < m_rows.size(); ++i) {
            if (m_rows.at(i).value(QStringLiteral("id")) == id) {
                m_rows[i] = row;
                const QModelIndex idx = index(i);
                emit dataChanged(idx, idx, {EntryRole});
                return i;
            }
        }
        const int at = static_cast<int>(m_rows.size());
        beginInsertRows({}, at, at);
        m_rows.append(row);
        endInsertRows();
        emit countChanged();
        return at;
    }

    void removeById(const QVariant& id) {
        for (int i = 0; i < m_rows.size(); ++i) {
            if (m_rows.at(i).value(QStringLiteral("id")) == id) {
                beginRemoveRows({}, i, i);
                m_rows.removeAt(i);
                endRemoveRows();
                emit countChanged();
                return;
            }
        }
    }

    [[nodiscard]] int indexOfId(const QVariant& id) const {
        for (int i = 0; i < m_rows.size(); ++i) {
            if (m_rows.at(i).value(QStringLiteral("id")) == id) {
                return i;
            }
        }
        return -1;
    }

signals:
    void countChanged();

private:
    QList<QVariantMap> m_rows;
};

} // namespace uimodels
