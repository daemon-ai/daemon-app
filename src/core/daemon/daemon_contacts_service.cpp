// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_contacts_service.h"

#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"
#include "outbox.h"
#include "verb_class.h"

#include <QJsonDocument>
#include <QJsonObject>

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
    if (m_outbox == outbox) {
        return;
    }
    if (m_outbox != nullptr) {
        disconnect(m_outbox, nullptr, this, nullptr);
    }
    m_outbox = outbox;
    if (m_outbox == nullptr) {
        return;
    }
    // §6.5: a roster-edit lane rejection reaches the initiating surface through the SAME
    // contactOperationFailed relay the GUI toast + TUI notice already bind. The rejected row
    // additionally stays visible in the outbox lens (durable affordance); the lane pauses until
    // retry/discard.
    connect(m_outbox, &mirror::Outbox::opChanged, this, [this](const QString& opId) {
        const mirror::PendingOp op = m_outbox->op(opId);
        if (op.state == mirror::OpState::Rejected &&
            mirror::verbClass(op.verb) == mirror::VerbClass::RosterEdit) {
            emit contactOperationFailed(op.lastError);
        }
    });
}

void DaemonContactsService::enqueueRosterOp(const QString& verb, const QString& transport,
                                            QJsonObject args, const QString& display) {
    args.insert(QStringLiteral("transport"), transport);
    const QByteArray payload = QJsonDocument(args).toJson(QJsonDocument::Compact);
    // Durable BEFORE any send (§6.6); lane scope = the transport (§6.4 "per transport for
    // roster edits" — cross-transport edits are independent). The drain nudge is the user's own
    // action; offline it simply holds: the gate keeps the op pending until connected, and it
    // replays on reconnect (§6.8).
    m_outbox->enqueue(verb, transport, payload, display);
    m_outbox->drain();
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

// --- roster mutations (D3, §6.4): the durable `roster-edit` outbox lane -----------------------
// The node owns the roster; each mutation is an intent enqueued to the per-transport lane
// (offline-durable, §6.6; the op_id rides the wire so replay is dedup-safe, §10.3). The roster
// rows change ONLY when the node's authoritative read lands (ContactsChanged → RosterList
// refetch) — never an optimistic local write (§14.7, no fake rows). Without an outbox (bare test
// stacks) they are no-ops: there is no legacy direct-send fallback anymore (the
// MirrorSessionStore precedent).

void DaemonContactsService::addContact(const QString& transport, const QString& contactId) {
    if (m_outbox == nullptr || transport.isEmpty() || contactId.isEmpty()) {
        return;
    }
    enqueueRosterOp(QStringLiteral("RosterAdd"), transport,
                    QJsonObject{{QStringLiteral("id"), contactId}},
                    QStringLiteral(u"add \u2014 %1").arg(contactId));
}

void DaemonContactsService::updateContact(const QString& transport, const QVariantMap& contact) {
    const QString contactId = contact.value(QStringLiteral("id")).toString();
    if (m_outbox == nullptr || transport.isEmpty() || contactId.isEmpty()) {
        return;
    }
    enqueueRosterOp(
        QStringLiteral("RosterUpdate"), transport,
        QJsonObject{
            {QStringLiteral("id"), contactId},
            {QStringLiteral("displayName"),
             contact.value(QStringLiteral("displayName")).toString()},
            {QStringLiteral("presence"), contact.value(QStringLiteral("presence")).toString()},
            {QStringLiteral("permission"), contact.value(QStringLiteral("permission")).toString()}},
        QStringLiteral(u"update \u2014 %1").arg(contactId));
}

void DaemonContactsService::removeContact(const QString& transport, const QString& contactId) {
    if (m_outbox == nullptr || transport.isEmpty() || contactId.isEmpty()) {
        return;
    }
    enqueueRosterOp(QStringLiteral("RosterRemove"), transport,
                    QJsonObject{{QStringLiteral("id"), contactId}},
                    QStringLiteral(u"remove \u2014 %1").arg(contactId));
}

void DaemonContactsService::setAlias(const QString& transport, const QString& contactId,
                                     const QString& alias) {
    if (m_outbox == nullptr || transport.isEmpty() || contactId.isEmpty()) {
        return;
    }
    // Empty alias clears (hasAlias=false ⇒ the wire optional is omitted node-side), the same
    // convention the direct repo seam used.
    enqueueRosterOp(QStringLiteral("ContactSetAlias"), transport,
                    QJsonObject{{QStringLiteral("id"), contactId},
                                {QStringLiteral("hasAlias"), !alias.isEmpty()},
                                {QStringLiteral("alias"), alias}},
                    QStringLiteral(u"alias \u2014 %1").arg(contactId));
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
