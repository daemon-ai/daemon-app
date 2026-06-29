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
                                    required property var modelData
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
