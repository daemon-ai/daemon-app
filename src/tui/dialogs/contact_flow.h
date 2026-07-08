// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

// [acct-mgmt] The TUI transport-contacts flows ('a'/'f' on an account/contacts-group row, Enter/
// 'a' on a contact row): the terminal analog of the GUI Contacts section over the SAME
// IContactsService seam (the conv-create DM reuses ITransportRegistry, no new wire op). Node-first:
// add is a typed-id prompt (RosterAdd), find chains a query prompt (DirectorySearch) into a results
// palette → per-person Add/DM, alias is a text prompt (ContactSetAlias; empty clears), and profile
// shows the node-rendered ContactProfile string read-only. Esc aborts any step without touching the
// seam. Lives in its own dialogs/ TU (the room_flow precedent).

#include <QObject>
#include <QString>
#include <QVariantList>
#include <Tui/ZWidget.h>

class PaletteDialog;
namespace transports {
class IContactsService;
class ITransportRegistry;
} // namespace transports

class ContactFlow : public QObject {
    Q_OBJECT

public:
    ContactFlow(transports::IContactsService* contacts, transports::ITransportRegistry* registry,
                Tui::ZWidget* host);

    // Account / contacts-group row entry points.
    void startAdd(const QString& transport);
    void startFind(const QString& transport);
    // Contact row entry points.
    void startAlias(const QString& transport, const QString& contactId, const QString& currentName);
    void openProfile(const QString& transport, const QString& contactId);

signals:
    // A seam mutation was driven; the host re-renders the Channels page.
    void changed();
    // The flow left its dialogs (committed or canceled); the host restores page focus.
    void finished();

private:
    void showDirectoryPalette();
    void showPersonActions(const QVariantMap& person);
    // Per-verb gating for the flow's transport (GUI canRosterOp/canConversationOp parity): resolves
    // the transport to its family's AdapterInfo; a concrete ops map is authoritative, a missing one
    // means the verb is unavailable (contacts have no legacy coarse fallback).
    [[nodiscard]] bool canRosterOp(const QString& verb) const;
    [[nodiscard]] bool canConversationOp(const QString& verb) const;

    transports::IContactsService* m_contacts = nullptr;
    transports::ITransportRegistry* m_registry = nullptr;
    Tui::ZWidget* m_host = nullptr;

    QString m_transport;
    QString m_contactId;
    // The last directory results (for the palette + action lookups).
    QVariantList m_directory;
    // Guards the directoryResults/profileReady handlers so a stale reply does not re-open a dialog
    // after the flow moved on (only the matching start* arms them).
    bool m_findPending = false;
    bool m_profilePending = false;

    PaletteDialog* m_directoryPick = nullptr;
    PaletteDialog* m_actionPick = nullptr;
};
