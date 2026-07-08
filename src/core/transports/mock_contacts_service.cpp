// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "transports/mock_contacts_service.h"

namespace transports {

namespace {

// Matches MockTransportRegistry's canned Matrix account so the Contacts section renders against a
// real-looking transport offline.
constexpr auto kMockTransport = "matrix/@you:matrix.org";

QVariantMap contactRow(const QString& id, const QString& displayName, const QString& presence,
                       const QString& permission) {
    QVariantMap m;
    m.insert(QStringLiteral("id"), id);
    m.insert(QStringLiteral("displayName"), displayName);
    m.insert(QStringLiteral("presence"), presence);
    m.insert(QStringLiteral("permission"), permission);
    return m;
}

} // namespace

MockContactsService::MockContactsService(QObject* parent) : IContactsService(parent) {
    m_contacts[QLatin1String(kMockTransport)] = QVariantList{
        contactRow(QStringLiteral("@alice:matrix.org"), QStringLiteral("Alice"),
                   QStringLiteral("available"), QStringLiteral("allow")),
        contactRow(QStringLiteral("@bob:matrix.org"), QStringLiteral("Bob"), QStringLiteral("idle"),
                   QStringLiteral("allow")),
        // No display name -> the UI falls back to the id (exercises that path).
        contactRow(QStringLiteral("@carol:matrix.org"), QString(), QStringLiteral("offline"),
                   QStringLiteral("allow")),
    };
}

QVariantList MockContactsService::contacts(const QString& transport) const {
    return m_contacts.value(transport);
}

void MockContactsService::refreshContacts(const QString& transport) {
    emit contactsChanged(transport, m_contacts.value(transport));
}

void MockContactsService::addContact(const QString& transport, const QString& contactId) {
    if (contactId.isEmpty()) {
        return;
    }
    QVariantList& rows = m_contacts[transport];
    for (const QVariant& v : rows) {
        if (v.toMap().value(QStringLiteral("id")).toString() == contactId) {
            return; // already present
        }
    }
    rows.append(
        contactRow(contactId, QString(), QStringLiteral("unknown"), QStringLiteral("allow")));
    emit contactsChanged(transport, rows);
}

void MockContactsService::updateContact(const QString& transport, const QVariantMap& contact) {
    const QString id = contact.value(QStringLiteral("id")).toString();
    if (id.isEmpty()) {
        return;
    }
    QVariantList& rows = m_contacts[transport];
    for (QVariant& v : rows) {
        QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("id")).toString() == id) {
            for (auto it = contact.constBegin(); it != contact.constEnd(); ++it) {
                m.insert(it.key(), it.value());
            }
            v = m;
            emit contactsChanged(transport, rows);
            return;
        }
    }
}

void MockContactsService::removeContact(const QString& transport, const QString& contactId) {
    QVariantList& rows = m_contacts[transport];
    for (qsizetype i = 0; i < rows.size(); ++i) {
        if (rows.at(i).toMap().value(QStringLiteral("id")).toString() == contactId) {
            rows.removeAt(i);
            emit contactsChanged(transport, rows);
            return;
        }
    }
}

void MockContactsService::setAlias(const QString& transport, const QString& contactId,
                                   const QString& alias) {
    // The node reflects a set alias into the contact's display name (contact-info carries no
    // separate alias field); mirror that so the "alias overlay" renders. Empty clears it.
    QVariantList& rows = m_contacts[transport];
    for (QVariant& v : rows) {
        QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("id")).toString() == contactId) {
            m.insert(QStringLiteral("displayName"), alias);
            v = m;
            emit contactsChanged(transport, rows);
            return;
        }
    }
}

void MockContactsService::getProfile(const QString& transport, const QString& contactId) {
    QString name = contactId;
    for (const QVariant& v : m_contacts.value(transport)) {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("id")).toString() == contactId) {
            const QString dn = m.value(QStringLiteral("displayName")).toString();
            if (!dn.isEmpty()) {
                name = dn;
            }
            break;
        }
    }
    emit profileReady(transport, contactId,
                      QStringLiteral("%1\n%2\nPresence handled by the node.").arg(name, contactId));
}

void MockContactsService::searchDirectory(const QString& transport, const QString& query) {
    // A tiny canned directory (people NOT in the roster). Filter by the query substring so the
    // people-picker's search path is exercised.
    const QVariantList people{
        contactRow(QStringLiteral("@dave:matrix.org"), QStringLiteral("Dave"),
                   QStringLiteral("available"), QStringLiteral("unset")),
        contactRow(QStringLiteral("@erin:matrix.org"), QStringLiteral("Erin"),
                   QStringLiteral("away"), QStringLiteral("unset")),
    };
    if (query.trimmed().isEmpty()) {
        emit directoryResults(transport, people);
        return;
    }
    QVariantList hits;
    for (const QVariant& v : people) {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("id")).toString().contains(query, Qt::CaseInsensitive) ||
            m.value(QStringLiteral("displayName"))
                .toString()
                .contains(query, Qt::CaseInsensitive)) {
            hits.append(m);
        }
    }
    emit directoryResults(transport, hits);
}

} // namespace transports
