// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "transports/icontacts_service.h"

#include <QJsonObject>

namespace daemonapp::daemon {
class ContactsRepository;
}
namespace mirror {
class Outbox;
}

namespace transports {

// [acct-mgmt] Daemon-backed transport-contacts seam (Phase D, wire v34). Projects the
// ContactsRepository's node data (RosterList + ContactGetProfile + DirectorySearch) into the
// IContactsService rows the Channels Contacts surface renders. The four roster MUTATIONS
// (RosterAdd/Update/Remove/ContactSetAlias) ride the durable per-transport `roster-edit` outbox
// lane (D3, spec 09 §6.4) via setOutbox — offline-durable, replayed on reconnect, §6.5 rejection
// surfacing — while the reads stay direct on the repository. Rebuilds/re-emits on the
// repository's refresh signals; refresh piggybacks the node's ContactsChanged feed event (wired
// in the service graph), which is also how a landed mutation refreshes the rows (never an
// optimistic local write).
//
// Lives under src/core/daemon (not transports/) because it depends on the daemon
// ContactsRepository, which links the transports interface — keeping it here avoids a library cycle
// (cf. DaemonTransportRegistry).
class DaemonContactsService : public IContactsService {
    Q_OBJECT

public:
    explicit DaemonContactsService(daemonapp::daemon::ContactsRepository* repo,
                                   QObject* parent = nullptr);

    // D3 (§6.4): the durable `roster-edit` lane seam. Once set, the four roster mutations
    // (add/update/remove/setAlias) ENQUEUE per-transport ops instead of direct-sending, so an
    // edit made offline survives and replays on reconnect; a lane rejection surfaces through
    // contactOperationFailed (§6.5). Reads (refresh/profile/directory) stay direct.
    void setOutbox(mirror::Outbox* outbox);

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
    // Durable enqueue + drain nudge on the per-transport `roster-edit` lane (§6.4/§6.6).
    void enqueueRosterOp(const QString& verb, const QString& transport, QJsonObject args,
                         const QString& display);

    daemonapp::daemon::ContactsRepository* m_repo = nullptr;
    mirror::Outbox* m_outbox = nullptr;
};

} // namespace transports
