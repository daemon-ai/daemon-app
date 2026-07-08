// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "transports/icontacts_service.h"

#include <QHash>

namespace transports {

// [acct-mgmt] Inert dev stand-in for IContactsService: carries a canned per-transport contact
// roster (consistent with MockTransportRegistry's canned Matrix account) so the Contacts UI can be
// built + exercised offline (UI-first). The mutations mutate this in-memory state and emit the same
// change signals the daemon service does; getProfile/searchDirectory return canned data. Replaced
// by DaemonContactsService (decoding the node's roster ops) whenever a daemon connects.
class MockContactsService : public IContactsService {
    Q_OBJECT

public:
    explicit MockContactsService(QObject* parent = nullptr);

    [[nodiscard]] QVariantList contacts(const QString& transport) const override;
    void refreshContacts(const QString& transport) override;
    void addContact(const QString& transport, const QString& contactId) override;
    void updateContact(const QString& transport, const QVariantMap& contact) override;
    void removeContact(const QString& transport, const QString& contactId) override;
    void setAlias(const QString& transport, const QString& contactId,
                  const QString& alias) override;
    void getProfile(const QString& transport, const QString& contactId) override;
    void searchDirectory(const QString& transport, const QString& query) override;

private:
    // transport -> ordered contact rows ({id, displayName, presence, permission, alias?}).
    QHash<QString, QVariantList> m_contacts;
};

} // namespace transports
