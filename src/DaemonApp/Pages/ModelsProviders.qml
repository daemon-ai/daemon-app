// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Providers tab: model sources / inference providers, with a configured badge.
// Configuring a remote provider deep-links to the Accounts wizard (Phase 4).
QQC.ScrollView {
    id: root
    anchors.fill: parent
    contentWidth: availableWidth
    clip: true

    ColumnLayout {
        width: root.availableWidth
        spacing: 8

        Item { implicitHeight: 12 }

        Repeater {
            model: ModelCatalog.providers()
            delegate: Rectangle {
                required property var modelData
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                Layout.preferredHeight: 60
                radius: 8
                color: Theme.surface
                border.color: Theme.border

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text {
                            text: modelData.name
                            font.family: FontIcons.display; font.pixelSize: 14
                            font.bold: true; color: Theme.text
                        }
                        Text {
                            text: modelData.kind === "local" ? qsTr("Local runtime")
                                                             : qsTr("Remote API")
                            font.family: FontIcons.display; font.pixelSize: 11
                            color: Theme.textMuted
                        }
                    }

                    Text {
                        visible: modelData.configured === true
                        text: qsTr("Configured")
                        font.family: FontIcons.display; font.pixelSize: 11; color: Theme.accent
                    }
                    Kit.TextButton {
                        text: modelData.configured ? qsTr("Manage") : qsTr("Configure")
                        onClicked: Nav.open("accounts")
                    }
                }
            }
        }
    }
}
