// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace transports {

// [acct-mgmt] Transport-contacts (roster) seam (Phase D, wire v34): the client-side view of one
// account's contact roster + the node's contact verbs. A sibling of ITransportRegistry (kept
// cohesive — this does NOT grow that seam): the Channels surface renders per-account contact rows
// and drives add/update/remove/setAlias/getProfile/searchDirectory through here, while
// ITransportRegistry stays the adapter/instance/room surface. The node decides — every verb is a
// thin intent over the wire; the app renders the node's answer and never re-derives roster state.
//
// DISTINCT from the GUI-sessions roster (fleet::ISessionRoster / RosterChanged): those are the
// session inbox. These are transport contacts (a Matrix/XMPP-style buddy list). A daemon adapter
// (DaemonContactsService) decodes RosterList/Add/Update/Remove + ContactGetProfile/SetAlias +
// DirectorySearch and emits these rows; the mock returns canned per-family data so the surface can
// be built + exercised offline. The `id`-shaped rows are display-only maps (never the codec).
class IContactsService : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    ~IContactsService() override = default;

    // The last-known contacts for a transport (RosterList). Each entry is a map:
    //   id (QString), displayName (QString), presence (QString), permission (QString).
    // `contacts()` returns the cached set; `refreshContacts()` triggers a live enumeration and
    // fires contactsChanged when it lands. Default no-ops so non-daemon seams need not implement.
    [[nodiscard]] Q_INVOKABLE virtual QVariantList contacts(const QString& transport) const {
        Q_UNUSED(transport)
        return {};
    }
    Q_INVOKABLE virtual void refreshContacts(const QString& transport) { Q_UNUSED(transport) }

    // Roster mutations (RosterAdd/Update/Remove). add/remove name the target id; update carries a
    // contact map ({id, displayName?, presence?, permission?}). On success the node emits
    // ContactsChanged and the service re-refreshes, so contactsChanged fires with the new set.
    Q_INVOKABLE virtual void addContact(const QString& transport, const QString& contactId) {
        Q_UNUSED(transport)
        Q_UNUSED(contactId)
    }
    Q_INVOKABLE virtual void updateContact(const QString& transport, const QVariantMap& contact) {
        Q_UNUSED(transport)
        Q_UNUSED(contact)
    }
    Q_INVOKABLE virtual void removeContact(const QString& transport, const QString& contactId) {
        Q_UNUSED(transport)
        Q_UNUSED(contactId)
    }

    // Set/clear a contact's local alias (ContactSetAlias; empty `alias` clears it), fetch a
    // node-rendered profile string (ContactGetProfile -> profileReady), search the adapter's people
    // directory (DirectorySearch -> directoryResults). Default no-ops.
    Q_INVOKABLE virtual void setAlias(const QString& transport, const QString& contactId,
                                      const QString& alias) {
        Q_UNUSED(transport)
        Q_UNUSED(contactId)
        Q_UNUSED(alias)
    }
    Q_INVOKABLE virtual void getProfile(const QString& transport, const QString& contactId) {
        Q_UNUSED(transport)
        Q_UNUSED(contactId)
    }
    Q_INVOKABLE virtual void searchDirectory(const QString& transport, const QString& query) {
        Q_UNUSED(transport)
        Q_UNUSED(query)
    }

signals:
    // The roster for `transport` changed (a refresh landed / a mutation re-refreshed / the node's
    // ContactsChanged feed event fired). `contacts` is the fresh row list (same shape as
    // contacts()).
    void contactsChanged(const QString& transport, const QVariantList& contacts);
    // A ContactGetProfile resolved: `profile` is the node-rendered string (rendered verbatim).
    void profileReady(const QString& transport, const QString& contactId, const QString& profile);
    // A DirectorySearch resolved: `contacts` is the people-picker result list (same row shape).
    void directoryResults(const QString& transport, const QVariantList& contacts);
    // A contact/roster operation failed on the node (surfaced as a toast / TUI notice).
    void contactOperationFailed(const QString& message);
};

} // namespace transports
