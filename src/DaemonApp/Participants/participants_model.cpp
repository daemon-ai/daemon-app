// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "participants_model.h"

#include "persistence/isession_store.h"

namespace participants {

ParticipantsModel::ParticipantsModel(QObject* parent) : QAbstractListModel(parent) {
    rebuild();
}

QObject* ParticipantsModel::store() const {
    return m_store;
}

void ParticipantsModel::setStore(QObject* store) {
    auto* typed = qobject_cast<persistence::ISessionStore*>(store);
    if (typed == m_store) {
        return;
    }
    if (m_store != nullptr) {
        disconnect(m_store, nullptr, this, nullptr);
    }
    m_store = typed;
    if (m_store != nullptr) {
        connect(m_store, &persistence::ISessionStore::changed, this, &ParticipantsModel::rebuild);
    }
    emit storeChanged();
    rebuild();
}

void ParticipantsModel::rebuild() {
    beginResetModel();
    m_rows.clear();

    Row header;
    header.label = tr("Participants");
    header.separator = true;
    header.hasChildren = true;
    m_rows.push_back(header);

    if (m_expanded && m_store != nullptr) {
        for (const domain::Participant& p : m_store->participants()) {
            Row r;
            r.label = p.name;
            r.color = p.color;
            r.presence = p.presence;
            r.isAgent = p.isAgent;
            m_rows.push_back(r);
        }
    }
    endResetModel();
}

int ParticipantsModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_rows.size());
}

QVariant ParticipantsModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }
    const Row& r = m_rows.at(index.row());
    switch (role) {
    case LabelRole:
        return r.label;
    case IsSeparatorRole:
        return r.separator;
    case HasChildrenRole:
        return r.hasChildren;
    case ExpandedRole:
        return m_expanded;
    case ColorRole:
        return r.color;
    case PresenceRole:
        return r.presence;
    case IsAgentRole:
        return r.isAgent;
    default:
        return {};
    }
}

QHash<int, QByteArray> ParticipantsModel::roleNames() const {
    return {
        {LabelRole, "label"},
        {IsSeparatorRole, "isSeparator"},
        {HasChildrenRole, "hasChildren"},
        {ExpandedRole, "expanded"},
        {ColorRole, "color"},
        {PresenceRole, "presence"},
        {IsAgentRole, "isAgent"},
    };
}

void ParticipantsModel::toggleExpand(int row) {
    if (row < 0 || row >= m_rows.size() || !m_rows.at(row).separator) {
        return;
    }
    m_expanded = !m_expanded;
    rebuild();
}

} // namespace participants
