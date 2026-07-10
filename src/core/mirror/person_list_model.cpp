// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "mirror/person_list_model.h"

namespace mirror {

PersonListModel::PersonListModel(Store& store, QObject* parent)
    : TableModel<Person>(store, parent) {
    reprime(); // re-derive order with this leaf's lessThan (silent, construction-time)
}

QHash<int, QByteArray> PersonListModel::roleNames() const {
    QHash<int, QByteArray> names;
    names.insert(StableIdRole, QByteArrayLiteral("stableId"));
    names.insert(PersonIdRole, QByteArrayLiteral("personId"));
    names.insert(AliasRole, QByteArrayLiteral("alias"));
    names.insert(AvatarBlobRole, QByteArrayLiteral("avatarBlob"));
    names.insert(EndpointCountRole, QByteArrayLiteral("endpointCount"));
    return names;
}

QVariant PersonListModel::roleData(const Person& p, int role) const {
    switch (role) {
    case PersonIdRole:
        return p.id;
    case AliasRole:
        return p.alias;
    case AvatarBlobRole:
        return p.avatar_blob;
    case EndpointCountRole:
        return p.endpoint_count;
    default:
        return {};
    }
}

bool PersonListModel::lessThan(const Person& a, const Person& b) const {
    const int byAlias = a.alias.compare(b.alias, Qt::CaseInsensitive);
    if (byAlias != 0) {
        return byAlias < 0;
    }
    return a.id < b.id;
}

} // namespace mirror
