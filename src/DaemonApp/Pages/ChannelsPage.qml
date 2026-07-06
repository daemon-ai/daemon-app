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
        if (state === "connecting")
            return Theme.warning;
        if (state === "error")
            return Theme.danger;
        return Theme.iconMuted; // offline / unknown
    }

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

    // Remove-credential confirm (B3 partial): the ONLY account-lifecycle lever the wire offers
    // today. Clearly labeled — it removes the STORED CREDENTIAL for the bound profile
    // (CredentialRemove); the node's transport session itself has no disconnect/remove op yet.
    Kit.Dialog {
        id: removeCredentialDialog
        property string profileRef: ""
        property string accountLabel: ""
        title: qsTr("Remove stored credential?")
        acceptText: qsTr("Remove credential")
        destructive: true
        onAccepted: {
            if (profileRef.length > 0)
                Accounts.remove(profileRef);
            profileRef = "";
        }
        onRejected: profileRef = ""

        contentItem: ColumnLayout {
            spacing: 6
            Text {
                Layout.fillWidth: true
                Layout.maximumWidth: 340
                text: qsTr("Removes the credential stored for profile “%1” (used by %2).")
                      .arg(removeCredentialDialog.profileRef)
                      .arg(removeCredentialDialog.accountLabel)
                font.family: FontIcons.display; font.pixelSize: 13; color: Theme.text
                wrapMode: Text.WordWrap
            }
            Text {
                Layout.fillWidth: true
                Layout.maximumWidth: 340
                text: qsTr("The account's transport session on the node is not affected — a disconnect/remove operation is not available yet.")
                font.family: FontIcons.display; font.pixelSize: 12; color: Theme.textMuted
                wrapMode: Text.WordWrap
            }
        }

        function openFor(profileRef, accountLabel) {
            removeCredentialDialog.profileRef = profileRef;
            removeCredentialDialog.accountLabel = accountLabel;
            removeCredentialDialog.open();
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
                            if (acctRow.expanded) {
                                acctRow.rooms = Transports.conversations(acctRow.modelData.transport);
                                Transports.refreshConversations(acctRow.modelData.transport);
                            }
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
                            // --- Account lifecycle (B3/EIO-2/EIO-7) -------------------------
                            // Disconnect is VISIBLE-DISABLED with the reason: the wire has no
                            // transport disconnect/remove op yet (node-first follow-up;
                            // placeholder-inventory policy: disabled controls explain an
                            // unavailable capability).
                            Kit.IconButton {
                                enabled: false
                                icon: FontIcons.fa_circle_xmark
                                iconPointSize: 12; implicitWidth: 30; implicitHeight: 26
                                tooltipText: qsTr("Disconnect isn't available yet — the node has no transport disconnect operation")
                            }
                            // The PARTIAL remove lever the wire does offer: drop the stored
                            // credential for the bound profile (confirmed; clearly labeled).
                            Kit.IconButton {
                                visible: acctRow.modelData.boundProfile.length > 0
                                icon: FontIcons.fa_trash
                                iconColor: Theme.danger
                                iconPointSize: 12; implicitWidth: 30; implicitHeight: 26
                                tooltipText: qsTr("Remove the stored credential…")
                                onClicked: removeCredentialDialog.openFor(
                                    acctRow.modelData.boundProfile,
                                    acctRow.modelData.displayName.length > 0
                                        ? acctRow.modelData.displayName
                                        : acctRow.modelData.transport)
                            }
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
                                    Connections {
                                        target: DaemonNet
                                        function onChanged() {
                                            roomRow.pinnedSession = DaemonNet.pinnedSessionFor(
                                                acctRow.modelData.transport, roomRow.modelData.id);
                                        }
                                    }
                                    Layout.fillWidth: true
                                    spacing: 8
                                    Text {
                                        text: FontIcons.fa_hashtag
                                        font.family: FontIcons.faSolid; font.pixelSize: 10
                                        color: Theme.iconMuted
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
                        }
                        // EIO-2 (browser SSO / account setup) is a later slice.
                        Kit.TextButton {
                            text: qsTr("Connect")
                            enabled: false
                        }
                    }
                }
            }
        }
    }
}
