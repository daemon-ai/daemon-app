// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "mirror/contact_list_model.h"

namespace mirror {

ContactListModel::ContactListModel(Store& store, QString transport, QObject* parent)
    : TableModel<Contact>(store, parent), m_transport(std::move(transport)) {
    // Re-derive with the transport filter + display-name sort now set (silent, construction-time).
    reprime();
}

std::vector<Contact> ContactListModel::presentRows(const MirrorModel& m) const {
    std::vector<Contact> out;
    for (const Contact& c : m.contacts) {
        if (m_transport.isEmpty() || c.transport == m_transport) {
            out.push_back(c);
        }
    }
    return out;
}

QHash<int, QByteArray> ContactListModel::roleNames() const {
    QHash<int, QByteArray> names;
    names.insert(StableIdRole, QByteArrayLiteral("stableId"));
    names.insert(TransportRole, QByteArrayLiteral("transport"));
    names.insert(ContactIdRole, QByteArrayLiteral("contactId"));
    names.insert(DisplayNameRole, QByteArrayLiteral("displayName"));
    names.insert(PermissionRole, QByteArrayLiteral("permission"));
    names.insert(PresenceRole, QByteArrayLiteral("presence"));
    return names;
}

QVariant ContactListModel::roleData(const Contact& c, int role) const {
    switch (role) {
    case TransportRole:
        return c.transport;
    case ContactIdRole:
        return c.id;
    case DisplayNameRole:
        return c.display_name;
    case PermissionRole:
        return c.permission;
    case PresenceRole:
        return c.presence_primitive;
    default:
        return {};
    }
}

bool ContactListModel::lessThan(const Contact& a, const Contact& b) const {
    const int byName = a.display_name.compare(b.display_name, Qt::CaseInsensitive);
    if (byName != 0) {
        return byName < 0;
    }
    return a.id < b.id;
}

} // namespace mirror
