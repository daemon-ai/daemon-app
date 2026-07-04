// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Presentation

// Approvals inbox: pending tool-execution requests with a risk badge and
// approve / deny actions.
Item {
    id: root

    PageHeader {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr("Approvals")
        icon: FontIcons.fa_circle_exclamation
    }

    Text {
        anchors.centerIn: parent
        visible: Approvals.count === 0
        text: qsTr("Inbox zero — no pending approvals.")
        font.family: FontIcons.display; font.pixelSize: 14; color: Theme.textMuted
    }

    QQC.ScrollView {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        clip: true
        contentWidth: availableWidth

        ListView {
            model: Approvals.pending
            spacing: 8
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                required property var entry
                width: ListView.view.width
                height: 92
                radius: 8
                color: Theme.surface
                border.color: entry.risk === "high" ? Theme.danger : Theme.border
                border.width: entry.risk === "high" ? 2 : 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text {
                            text: entry.tool
                            font.family: FontIcons.display; font.pixelSize: 13
                            font.bold: true; color: Theme.text
                        }
                        Rectangle {
                            radius: 3
                            color: entry.risk === "high" ? Theme.danger
                                 : entry.risk === "medium" ? Theme.hover : Theme.hover
                            implicitWidth: rk.implicitWidth + 10
                            implicitHeight: rk.implicitHeight + 4
                            Text {
                                id: rk; anchors.centerIn: parent
                                text: DisplayPresenter.enumLabel("approval.risk", entry.risk)
                                font.family: FontIcons.display; font.pixelSize: 9
                                color: entry.risk === "high" ? Theme.background : Theme.textMuted
                            }
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: entry.session + " · " + entry.requested
                            font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
                        }
                    }

                    Text {
                        text: entry.command
                        font.family: FontIcons.mono; font.pixelSize: 11; color: Theme.text
                        Layout.fillWidth: true; elide: Text.ElideRight
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Item { Layout.fillWidth: true }
                        Kit.TextButton { text: qsTr("Deny"); onClicked: Approvals.deny(entry.id) }
                        Kit.TextButton {
                            text: qsTr("Approve"); accentFilled: true
                            onClicked: Approvals.approve(entry.id)
                        }
                    }
                }
            }
        }
    }
}
