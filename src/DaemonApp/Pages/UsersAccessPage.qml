// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Users & Access admin page (auth6). Shows the authenticated principal (WhoAmI) and scaffolds the
// user/role administration surface. The page host (Session.qml) and the command palette gate this
// on the access_admin capability, and the node enforces access server-side regardless.
//
// SCAFFOLD: the user-CRUD data path depends on the node access-admin API (Auth 5), which is not yet
// built. Until it lands, the management section shows a pending state; the WhoAmI panel is live.
Item {
    id: root

    PageHeader {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr("Users & Access")
        icon: FontIcons.fa_lock
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
            width: parent.width
            spacing: 16

            // --- WhoAmI (live, from the AuthOk PrincipalView) ---
            SectionLabel { text: qsTr("Signed in as") }
            Rectangle {
                Layout.fillWidth: true
                radius: 10
                color: Theme.surface
                border.color: Theme.border
                border.width: 1
                implicitHeight: whoami.implicitHeight + 24

                ColumnLayout {
                    id: whoami
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 4
                    Text {
                        text: (typeof Principal !== "undefined" && Principal && Principal.authenticated)
                              ? Principal.username : qsTr("Not authenticated")
                        font.family: FontIcons.display; font.pixelSize: 16
                        font.weight: Font.DemiBold; color: Theme.text
                    }
                    Text {
                        visible: typeof Principal !== "undefined" && Principal && Principal.userId.length > 0
                        text: qsTr("User id: %1").arg(Principal ? Principal.userId : "")
                        font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
                    }
                    Text {
                        text: qsTr("Roles: %1").arg(
                                  (typeof Principal !== "undefined" && Principal && Principal.roles.length > 0)
                                  ? Principal.roles.join(", ") : qsTr("(none)"))
                        font.family: FontIcons.display; font.pixelSize: 12; color: Theme.text
                        Layout.fillWidth: true; wrapMode: Text.WordWrap
                    }
                    Text {
                        text: qsTr("Capabilities: %1").arg(
                                  (typeof Principal !== "undefined" && Principal && Principal.capabilities.length > 0)
                                  ? Principal.capabilities.join(", ") : qsTr("(none)"))
                        font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
                        Layout.fillWidth: true; wrapMode: Text.WordWrap
                    }
                }
            }

            // --- User management (pending Auth 5: access-admin-api) ---
            SectionLabel { text: qsTr("Users & roles") }
            Rectangle {
                Layout.fillWidth: true
                radius: 10
                color: Theme.surface
                border.color: Theme.border
                border.width: 1
                implicitHeight: pending.implicitHeight + 24

                ColumnLayout {
                    id: pending
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 6
                    Text {
                        text: qsTr("User administration is not available yet.")
                        font.family: FontIcons.display; font.pixelSize: 13; color: Theme.text
                    }
                    Text {
                        text: qsTr("Listing, creating, disabling users and assigning roles requires "
                                   + "the node access-admin API, which is still being built.")
                        font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
                        Layout.fillWidth: true; wrapMode: Text.WordWrap
                    }
                    Kit.TextButton {
                        text: qsTr("Add user")
                        enabled: false // pending the access-admin API (Auth 5)
                    }
                }
            }
        }
    }
}
