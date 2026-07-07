// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Channels / Events-IO account manager (story 04, read surface: EIO-1/3/8/9). Daemon-backed +
// offline-first: the adapter picker (Transports.availableAdapters), the configured accounts with
// Pidgin-style status dots (Presence), and each account's live rooms (Transports.conversations via
// ConvList). Read-only this slice - connecting (EIO-2) and disconnecting (EIO-7) are deferred.
Item {
    id: root

    // Re-read the Q_INVOKABLE lists on the seams' change signals (they are not properties).
    property var adapters: Transports.availableAdapters()
    property var accounts: Transports.instances()

    Connections {
        target: Transports
        function onAdaptersChanged() { root.adapters = Transports.availableAdapters(); }
        function onInstancesChanged() { root.accounts = Transports.instances(); }
    }

    function dotColor(state) {
        if (state === "connected")
            return Theme.accent;
        // [waveB:app-v30] D1: connecting AND disconnecting are transient; both render amber. The
        // node reports "connecting" after a drop (reconnecting…) — never predicted client-side.
        if (state === "connecting" || state === "disconnecting")
            return Theme.warning;
        if (state === "error")
            return Theme.danger;
        return Theme.iconMuted; // offline / unknown
    }

    // [waveB:app-v30] D1: friendly copy for the node's coarse disconnect-reason token. The node's
    // `message` (rendered verbatim, not here) is authoritative; this only labels the token.
    function reasonLabel(token) {
        switch (token) {
        case "user_requested": return qsTr("Disconnected by request");
        case "network_error": return qsTr("Network error");
        case "authentication_failed": return qsTr("Authentication failed");
        case "replaced_by_other_client": return qsTr("Replaced by another client");
        case "invalid_settings": return qsTr("Invalid settings");
        case "certificate_error": return qsTr("Certificate error");
        case "other": return qsTr("Disconnected");
        default: return "";
        }
    }

    // [wave2:app-channels-liveness] B1: true when the node advertises an interactive-auth
    // provider for this adapter family (so the "Connect" button only lights up when a sign-in
    // flow actually exists — the app never fakes a capability the node did not report).
    function hasAuthProvider(family) {
        if (typeof AuthFlow === "undefined" || !AuthFlow)
            return false;
        var rows = AuthFlow.providers();
        for (var i = 0; i < rows.length; ++i)
            if (rows[i].family === family)
                return true;
        return false;
    }

    // The shared interactive-auth sheet (B1): the "Connect" button opens it pre-narrowed to the
    // adapter's family. On success the shared service graph refetches instances + that account's
    // rooms, so the new account appears here (and its live status lights up via TransportChanged).
    AuthFlowSheet { id: authSheet }

    // Pin a room to an agent/session (B4/B6): the same RouteDialog the routing manager uses,
    // driven by a page-local controller over the shared DaemonNet.
    RoutingManagerController {
        id: channelsRouting
        daemonNet: typeof DaemonNet !== "undefined" ? DaemonNet : null
    }
    RouteDialog {
        id: pinDialog
        controller: channelsRouting
    }

    // [waveB:app-v30] D1: remove-account confirm. Sends TransportRemove (wire v30): the node
    // sequences the full teardown (disconnect + conversation close + routing unbind + credential
    // drop + config drop) — the client sends ONE intent and renders the reported outcome. This is
    // distinct from the AccountsPage credential-only lever; we do NOT chain a credential remove.
    Kit.Dialog {
        id: removeAccountDialog
        property string transport: ""
        property string accountLabel: ""
        title: qsTr("Remove account?")
        acceptText: qsTr("Remove account")
        destructive: true
        onAccepted: {
            if (transport.length > 0)
                Transports.remove(transport);
            transport = "";
        }
        onRejected: transport = ""

        contentItem: ColumnLayout {
            spacing: 6
            Text {
                Layout.fillWidth: true
                Layout.maximumWidth: 340
                text: qsTr("Removes the account “%1” from the node.").arg(removeAccountDialog.accountLabel)
                font.family: FontIcons.display; font.pixelSize: 13; color: Theme.text
                wrapMode: Text.WordWrap
            }
            Text {
                Layout.fillWidth: true
                Layout.maximumWidth: 340
                text: qsTr("The node disconnects the transport, closes its conversations, unbinds its routes, and drops the stored credential. This cannot be undone.")
                font.family: FontIcons.display; font.pixelSize: 12; color: Theme.textMuted
                wrapMode: Text.WordWrap
            }
        }

        function openFor(transport, accountLabel) {
            removeAccountDialog.transport = transport;
            removeAccountDialog.accountLabel = accountLabel;
            removeAccountDialog.open();
        }
    }

    PageHeader {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr("Channels")
        icon: FontIcons.fa_comments
    }

    QQC.ScrollView {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: root.width - 32
            spacing: 14

            // --- Connected accounts (EIO-3 list + EIO-9 status dots) -----------
            SectionLabel { text: qsTr("Accounts") }

            Text {
                visible: root.accounts.length === 0
                Layout.fillWidth: true
                text: qsTr("No channels connected.")
                wrapMode: Text.Wrap
                font.family: FontIcons.display; font.pixelSize: 12; color: Theme.textMuted
            }

            // [wave2:app-channels-liveness] B2: honest, behavior-level note. Room membership is the
            // node's (it accepts invites per its own configuration); newly-joined rooms surface here
            // on refresh. We do NOT display the node's auto-accept policy value — it is not
            // queryable over the wire at v29 (a node-first follow-up), so asserting it would be
            // dishonest.
            Text {
                visible: root.accounts.length > 0
                Layout.fillWidth: true
                text: qsTr("Room invites are handled by the node; newly-joined rooms appear here automatically.")
                wrapMode: Text.Wrap
                font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
            }

            Repeater {
                model: root.accounts
                delegate: Rectangle {
                    id: acctRow
                    required property var modelData
                    property string conn: Presence.connectionState(modelData.transport)
                    property bool expanded: false
                    property var rooms: []

                    Layout.fillWidth: true
                    implicitHeight: abody.implicitHeight + 20
                    radius: 8
                    color: Theme.surface
                    border.color: Theme.border

                    // Live status dot: re-read on the presence seam's change for this transport.
                    Connections {
                        target: Presence
                        function onPresenceChanged(transport) {
                            if (transport === acctRow.modelData.transport)
                                acctRow.conn = Presence.connectionState(acctRow.modelData.transport);
                        }
                    }
                    // Refresh the room list when a live ConvList lands for this account.
                    Connections {
                        target: Transports
                        function onConversationsChanged(transport) {
                            if (transport === acctRow.modelData.transport)
                                acctRow.rooms = Transports.conversations(transport);
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            acctRow.expanded = !acctRow.expanded;
                            // [waveB:app-v30] D2: the per-expand live ConvList refetch is RETIRED.
                            // Read the cached rooms (seeded once per connect; kept fresh by the
                            // ConversationsChanged / MembershipChanged feed) — no client poll.
                            if (acctRow.expanded)
                                acctRow.rooms = Transports.conversations(acctRow.modelData.transport);
                        }
                    }

                    ColumnLayout {
                        id: abody
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        anchors.topMargin: 10
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            Text {
                                text: FontIcons.fa_circle
                                font.family: FontIcons.faSolid; font.pixelSize: 9
                                color: root.dotColor(acctRow.conn)
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                Text {
                                    text: acctRow.modelData.displayName.length > 0
                                          ? acctRow.modelData.displayName : acctRow.modelData.transport
                                    font.family: FontIcons.display; font.pixelSize: 13; color: Theme.text
                                    elide: Text.ElideRight; Layout.fillWidth: true
                                }
                                Text {
                                    text: acctRow.modelData.boundProfile.length > 0
                                          ? qsTr("%1 · %2").arg(acctRow.modelData.family)
                                                           .arg(acctRow.modelData.boundProfile)
                                          : acctRow.modelData.family
                                    font.family: FontIcons.mono; font.pixelSize: 10; color: Theme.textMuted
                                }
                            }
                            Text {
                                text: acctRow.conn
                                font.family: FontIcons.display; font.pixelSize: 11
                                color: acctRow.conn === "connected" ? Theme.accent : Theme.textMuted
                            }
                            // --- Account lifecycle (EIO-2/EIO-7; [waveB:app-v30] D1) ----------
                            // Re-authenticate: shown ONLY when the node marks the disconnect fatal
                            // (e.g. auth failed / invalid settings). Opens the shared sign-in sheet
                            // narrowed to this adapter family, bound back to the same profile.
                            Kit.IconButton {
                                visible: acctRow.modelData.fatal === true
                                icon: FontIcons.fa_rotate
                                iconColor: Theme.warning
                                iconPointSize: 12; implicitWidth: 30; implicitHeight: 26
                                tooltipText: qsTr("Re-authenticate this account")
                                onClicked: authSheet.openFlowForFamily(
                                    acctRow.modelData.family, acctRow.modelData.boundProfile)
                            }
                            // Disconnect the live transport session (TransportDisconnect). Enabled
                            // only when there is a session to tear down (not already offline).
                            Kit.IconButton {
                                enabled: acctRow.conn !== "offline"
                                         && acctRow.conn !== "disconnecting"
                                icon: FontIcons.fa_circle_xmark
                                iconPointSize: 12; implicitWidth: 30; implicitHeight: 26
                                tooltipText: qsTr("Disconnect this account")
                                onClicked: Transports.disconnect(acctRow.modelData.transport)
                            }
                            // Remove the account entirely (TransportRemove) — destructive, confirmed.
                            Kit.IconButton {
                                icon: FontIcons.fa_trash
                                iconColor: Theme.danger
                                iconPointSize: 12; implicitWidth: 30; implicitHeight: 26
                                tooltipText: qsTr("Remove this account…")
                                onClicked: removeAccountDialog.openFor(
                                    acctRow.modelData.transport,
                                    acctRow.modelData.displayName.length > 0
                                        ? acctRow.modelData.displayName
                                        : acctRow.modelData.transport)
                            }
                        }

                        // --- Disconnect provenance ([waveB:app-v30] D1) ---------------------
                        // The node's human message verbatim (authoritative); otherwise the coarse
                        // reason token's friendly label. Shown only when the node reported one.
                        Text {
                            Layout.fillWidth: true
                            visible: text.length > 0
                            text: {
                                var msg = acctRow.modelData.connectionMessage;
                                if (msg && String(msg).length > 0)
                                    return String(msg);
                                return root.reasonLabel(acctRow.modelData.connectionReason);
                            }
                            font.family: FontIcons.display; font.pixelSize: 11
                            color: acctRow.modelData.fatal === true ? Theme.danger : Theme.textMuted
                            wrapMode: Text.Wrap
                            textFormat: Text.PlainText
                        }

                        // --- Rooms for this account (EIO-8: live ConvList) ------
                        ColumnLayout {
                            visible: acctRow.expanded
                            Layout.fillWidth: true
                            spacing: 4
                            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
                            Text {
                                visible: acctRow.rooms.length === 0
                                text: qsTr("No rooms.")
                                font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
                            }
                            Repeater {
                                model: acctRow.rooms
                                delegate: RowLayout {
                                    id: roomRow
                                    required property var modelData
                                    // The room's routing pin (B6/EIO-12): re-read on pin changes.
                                    property string pinnedSession:
                                        DaemonNet.pinnedSessionFor(acctRow.modelData.transport,
                                                                   modelData.id)
                                    // [wave2:app-channels-liveness] B2: badged when the node
                                    // surfaced this room after the operator's baseline for the
                                    // account (e.g. an auto-accepted invite). Re-read when a live
                                    // ConvList lands; tap the chip to dismiss.
                                    property bool isNew:
                                        Transports.isNewConversation(acctRow.modelData.transport,
                                                                     modelData.id)
                                    Connections {
                                        target: DaemonNet
                                        function onChanged() {
                                            roomRow.pinnedSession = DaemonNet.pinnedSessionFor(
                                                acctRow.modelData.transport, roomRow.modelData.id);
                                        }
                                    }
                                    Connections {
                                        target: Transports
                                        function onConversationsChanged(transport) {
                                            if (transport === acctRow.modelData.transport)
                                                roomRow.isNew = Transports.isNewConversation(
                                                    acctRow.modelData.transport,
                                                    roomRow.modelData.id);
                                        }
                                    }
                                    Layout.fillWidth: true
                                    spacing: 8
                                    Text {
                                        text: FontIcons.fa_hashtag
                                        font.family: FontIcons.faSolid; font.pixelSize: 10
                                        color: Theme.iconMuted
                                    }
                                    // Newly-joined-room affordance (B2): dismiss on tap.
                                    Kit.Chip {
                                        visible: roomRow.isNew
                                        text: qsTr("new")
                                        tone: "accent"
                                        interactive: true
                                        tooltipText: qsTr("Newly joined room")
                                        onClicked: {
                                            Transports.markConversationSeen(
                                                acctRow.modelData.transport, roomRow.modelData.id);
                                            roomRow.isNew = false;
                                        }
                                    }
                                    Text {
                                        text: modelData.title.length > 0 ? modelData.title : modelData.id
                                        font.family: FontIcons.display; font.pixelSize: 12
                                        color: Theme.text; elide: Text.ElideRight; Layout.fillWidth: true
                                    }
                                    // Route-pin chip: this room's inbound messages route to the
                                    // pinned session; tap opens the routing manager.
                                    Kit.Chip {
                                        visible: roomRow.pinnedSession.length > 0
                                        text: qsTr("⇄ %1").arg(roomRow.pinnedSession)
                                        tone: "accent"
                                        interactive: true
                                        tooltipText: qsTr("Pinned to this session — open the routing manager")
                                        onClicked: Nav.open("routing")
                                    }
                                    // Unpinned rooms: pinning is an EXPLICIT act (B4 - no lazy
                                    // session create) via the shared RouteDialog, preselected to
                                    // this room's origin (the canonical originKey format from
                                    // routing_dtos.h: "<transport>|group|<chat>|").
                                    Kit.Chip {
                                        visible: roomRow.pinnedSession.length === 0
                                        text: qsTr("Pin to agent…")
                                        tone: "muted"
                                        interactive: true
                                        tooltipText: qsTr("Route this room's messages to a session")
                                        onClicked: pinDialog.openFor({
                                            id: acctRow.modelData.transport + "|group|"
                                                + roomRow.modelData.id + "|",
                                            session: "",
                                            profile: ""
                                        })
                                    }
                                    Text {
                                        text: modelData.kind
                                        font.family: FontIcons.mono; font.pixelSize: 10; color: Theme.textMuted
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // --- Add channel (EIO-1 adapter picker; connect deferred) ----------
            SectionLabel { text: qsTr("Add channel"); Layout.topMargin: 8 }

            Text {
                visible: root.adapters.length === 0
                Layout.fillWidth: true
                text: qsTr("Connect to a daemon to see available channel types.")
                wrapMode: Text.Wrap
                font.family: FontIcons.display; font.pixelSize: 12; color: Theme.textMuted
            }

            Repeater {
                model: root.adapters
                delegate: Rectangle {
                    required property var modelData
                    Layout.fillWidth: true
                    implicitHeight: pbody.implicitHeight + 20
                    radius: 8
                    color: Theme.surface
                    border.color: Theme.border

                    RowLayout {
                        id: pbody
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        anchors.topMargin: 10
                        spacing: 10
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            Text {
                                text: modelData.displayName
                                font.family: FontIcons.display; font.pixelSize: 13; color: Theme.text
                            }
                            Text {
                                text: modelData.family
                                font.family: FontIcons.mono; font.pixelSize: 10; color: Theme.textMuted
                            }
                            // [waveB:app-v30] D3: read-only node-labeled adapter policies. The node
                            // owns label + value; the client renders them verbatim and never keys
                            // behavior off the policy `key`.
                            Repeater {
                                model: modelData.policies !== undefined ? modelData.policies : []
                                delegate: RowLayout {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    spacing: 6
                                    Text {
                                        text: modelData.label
                                        font.family: FontIcons.display; font.pixelSize: 10
                                        color: Theme.textMuted
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.value
                                        font.family: FontIcons.mono; font.pixelSize: 10
                                        color: Theme.text; elide: Text.ElideRight
                                        horizontalAlignment: Text.AlignRight
                                    }
                                }
                            }
                        }
                        // [wave2:app-channels-liveness] B1: connect an account via the shared
                        // interactive-auth sheet (MatrixSso family). Enabled only for adapters the
                        // node reports as interactive-auth capable AND for which it advertises a
                        // sign-in provider; the account lands unbound (rooms are pinned to sessions
                        // explicitly via the routing manager — coordinator decision 1).
                        Kit.TextButton {
                            readonly property bool canConnect:
                                modelData.capabilities !== undefined
                                && modelData.capabilities.interactiveAuth === true
                                && root.hasAuthProvider(modelData.family)
                            text: qsTr("Connect")
                            enabled: canConnect
                            QQC.ToolTip.text: canConnect
                                ? qsTr("Sign in to connect this channel")
                                : qsTr("This channel type has no browser sign-in.")
                            QQC.ToolTip.visible: hovered && QQC.ToolTip.text.length > 0
                            onClicked: authSheet.openFlowForFamily(modelData.family, "")
                        }
                    }
                }
            }
        }
    }
}
