// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_contacts_service.h"

#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"

namespace transports {

namespace {

// Project one DecodedContact into the display-only row map the surfaces render.
QVariantMap contactToVariant(const daemonapp::daemon::DecodedContact& c) {
    QVariantMap row;
    row[QStringLiteral("id")] = c.id;
    row[QStringLiteral("displayName")] = c.displayName;
    row[QStringLiteral("presence")] = c.presence;
    row[QStringLiteral("permission")] = c.permission;
    return row;
}

QVariantList contactsToVariant(const QList<daemonapp::daemon::DecodedContact>& contacts) {
    QVariantList out;
    out.reserve(contacts.size());
    for (const daemonapp::daemon::DecodedContact& c : contacts) {
        out.append(contactToVariant(c));
    }
    return out;
}

} // namespace

DaemonContactsService::DaemonContactsService(daemonapp::daemon::ContactsRepository* repo,
                                             QObject* parent)
    : IContactsService(parent), m_repo(repo) {
    if (m_repo == nullptr) {
        return;
    }
    // Re-project the roster on a refresh (carrying the fresh row list), forward the directory
    // results + profile string, and surface op errors — the same seam both surfaces bind.
    connect(m_repo, &daemonapp::daemon::ContactsRepository::contactsRefreshed, this,
            [this](const QString& transport) {
                emit contactsChanged(transport, contactsToVariant(m_repo->contacts(transport)));
            });
    connect(m_repo, &daemonapp::daemon::ContactsRepository::directoryReady, this,
            [this](const QString& transport) {
                emit directoryResults(transport, contactsToVariant(m_repo->directory(transport)));
            });
    connect(m_repo, &daemonapp::daemon::ContactsRepository::profileReady, this,
            &IContactsService::profileReady);
    connect(m_repo, &daemonapp::daemon::ContactsRepository::operationFailed, this,
            &IContactsService::contactOperationFailed);
}

void DaemonContactsService::setOutbox(mirror::Outbox* outbox) {
    // RED scaffolding (D3): stores the lane seam; the enqueue routing lands in the green step.
    m_outbox = outbox;
}

QVariantList DaemonContactsService::contacts(const QString& transport) const {
    if (m_repo == nullptr) {
        return {};
    }
    return contactsToVariant(m_repo->contacts(transport));
}

void DaemonContactsService::refreshContacts(const QString& transport) {
    if (m_repo != nullptr) {
        m_repo->refreshContacts(transport);
    }
}

void DaemonContactsService::addContact(const QString& transport, const QString& contactId) {
    if (m_repo != nullptr) {
        m_repo->addContact(transport, contactId);
    }
}

void DaemonContactsService::updateContact(const QString& transport, const QVariantMap& contact) {
    if (m_repo == nullptr) {
        return;
    }
    daemonapp::daemon::DecodedContact c;
    c.id = contact.value(QStringLiteral("id")).toString();
    c.displayName = contact.value(QStringLiteral("displayName")).toString();
    c.presence = contact.value(QStringLiteral("presence")).toString();
    c.permission = contact.value(QStringLiteral("permission")).toString();
    m_repo->updateContact(transport, c);
}

void DaemonContactsService::removeContact(const QString& transport, const QString& contactId) {
    if (m_repo != nullptr) {
        m_repo->removeContact(transport, contactId);
    }
}

void DaemonContactsService::setAlias(const QString& transport, const QString& contactId,
                                     const QString& alias) {
    if (m_repo != nullptr) {
        m_repo->setAlias(transport, contactId, alias);
    }
}

void DaemonContactsService::getProfile(const QString& transport, const QString& contactId) {
    if (m_repo != nullptr) {
        m_repo->getProfile(transport, contactId);
    }
}

void DaemonContactsService::searchDirectory(const QString& transport, const QString& query) {
    if (m_repo != nullptr) {
        m_repo->searchDirectory(transport, query);
    }
}

} // namespace transports
