// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once
// mirror::PersonListModel — the M2 persons read lens (spec 09 §8.3). TableModel<Person> with a
// curated role map reading the generated Person Q_GADGET; the single canonical shape replacing the
// legacy person row map (07§6, contact/person 4→1). Alias-sorted presentation. GUI-free C++.

#include "mirror/generated/entities_gen.h"
#include "mirror/table_model.h"

#include <QByteArray>
#include <QHash>
#include <QVariant>

namespace mirror {

class PersonListModel : public TableModel<Person> {
    Q_OBJECT

public:
    enum Roles : int {
        PersonIdRole = Qt::UserRole + 100,
        AliasRole,
        AvatarBlobRole,
        EndpointCountRole,
    };

    explicit PersonListModel(Store& store, QObject* parent = nullptr);

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

protected:
    [[nodiscard]] QVariant roleData(const Person& p, int role) const override;
    [[nodiscard]] bool lessThan(const Person& a, const Person& b) const override;
};

} // namespace mirror
