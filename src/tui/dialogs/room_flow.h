// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

// [acct-mgmt] The TUI room-lifecycle + member-management flows ('g'/'n' on an account row,
// Enter/'i'/'l'/'x' on a room row): the terminal analog of the GUI JoinRoomDialog / NewRoomDialog /
// member palette over the SAME ITransportRegistry seam. Node-first: the join/create forms are
// fetched from the node (ConvJoinDetails / ConvCreateDetails) and rendered as chained
// TextPromptDialog steps (name -> masked password if supported -> nickname if supported -> one
// prompt per extras field); the member palette lists ConvGet members and offers Invite/Kick/Ban/
// Role over confirm dialogs. Esc aborts any step without touching the seam. Lives in its own
// dialogs/ TU (the add_account_flow precedent) so the room workstream owns one focused file.

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <Tui/ZWidget.h>

class PaletteDialog;
namespace transports {
class ITransportRegistry;
}

class RoomFlow : public QObject {
    Q_OBJECT

public:
    RoomFlow(transports::ITransportRegistry* registry, Tui::ZWidget* host);

    // Two-phase form entry points ('g'/'n' on an account row): fetch the node's form, then chain
    // the prompts. joinRoom/createRoom are sent once every step is collected.
    void startJoin(const QString& transport);
    void startCreate(const QString& transport);
    // Member management ('i'/Enter on a room row): a single invite prompt, or the members palette.
    void startInvite(const QString& transport, const QString& conversation);
    void openMembers(const QString& transport, const QString& conversation);

signals:
    // A seam mutation was driven; the host re-renders the Channels page.
    void changed();
    // The flow left its dialogs (committed or canceled); the host restores page focus.
    void finished();

private:
    // Chained-prompt driver for the join form: consumes m_pending steps in order, then joins.
    void runNextJoinStep();
    void runNextCreateStep();
    // Members palette -> per-member action palette -> confirm-if-destructive.
    void showMembersPalette();
    void showMemberActions(const QVariantMap& member);
    void showRolePalette(const QVariantMap& member);

    // [acct-mgmt] Per-verb membership gating for the action palette (wire v33): resolves the
    // flow's transport to its family's AdapterInfo; a concrete membershipOps map is authoritative
    // for `verb` (invite|remove|ban|setRole), a missing one (v32 node / null) falls back to the
    // legacy coarse `rooms` capability — GUI canMembershipOp parity.
    [[nodiscard]] bool canMembershipOp(const QString& verb) const;

    transports::ITransportRegistry* m_registry = nullptr;
    Tui::ZWidget* m_host = nullptr;

    // In-flight flow context (only one flow runs at a time; the seam signals are matched on it).
    QString m_transport;
    QString m_conversation;

    // Join: the collected form + the queue of remaining {kind,key,label,masked} prompt steps.
    QVariantMap m_joinForm;
    QVariantList m_joinSteps;
    // Create: the collected extras + the queue of remaining extras-field steps.
    QVariantMap m_createExtras;
    QVariantList m_createSteps;

    // The last-loaded member list (for the palette + action lookups).
    QVariantList m_members;
    // Guards the membersChanged handler so a post-action ConvGet refresh does not re-pop the
    // palette after the flow has moved on (only openMembers arms it).
    bool m_membersPending = false;

    PaletteDialog* m_membersPick = nullptr;
    PaletteDialog* m_actionPick = nullptr;
    PaletteDialog* m_rolePick = nullptr;
};
