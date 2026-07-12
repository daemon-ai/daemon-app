// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once
// mirror::ContactListModel — the M2 contacts (transport roster) read lens (spec 09 §8.3).
// TableModel<Contact> filtered to one transport, curated role map off the generated Contact
// Q_GADGET — the single canonical shape retiring the 4 contact/person layouts (07§6). Sorted by
// display name. GUI-free C++.

#include "mirror/generated/entities_gen.h"
#include "mirror/table_model.h"

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QVariant>
#include <vector>

namespace mirror {

class ContactListModel : public TableModel<Contact> {
    Q_OBJECT

public:
    enum Roles : int {
        TransportRole = Qt::UserRole + 100,
        ContactIdRole,
        DisplayNameRole,
        PermissionRole,
        PresenceRole,
    };

    // `transport` empty = all transports; otherwise only that transport's roster rows.
    explicit ContactListModel(Store& store, QString transport = QString(),
                              QObject* parent = nullptr);

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

protected:
    [[nodiscard]] std::vector<Contact> presentRows(const MirrorModel& m) const override;
    [[nodiscard]] QVariant roleData(const Contact& c, int role) const override;
    [[nodiscard]] bool lessThan(const Contact& a, const Contact& b) const override;

private:
    QString m_transport;
};

} // namespace mirror
