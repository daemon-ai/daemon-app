// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme

// Overview dashboard: summary stat cards (derived live from the roster / fleet /
// approvals seams) over a recent-activity feed.
Item {
    id: root

    PageHeader {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr("Dashboard")
        icon: FontIcons.fa_gauge_high
    }

    QQC.ScrollView {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: root.width - 48
            x: 24
            y: 20
            spacing: 16

            // --- Stat cards ----------------------------------------------
            Flow {
                Layout.fillWidth: true
                spacing: 12

                component Stat: Rectangle {
                    id: card
                    property string value: ""
                    property string label: ""
                    property color tint: Theme.accent
                    width: 168; height: 84
                    radius: 10
                    color: Theme.surface
                    border.color: Theme.border
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 2
                        Text {
                            text: card.value
                            font.family: FontIcons.display; font.pixelSize: 26
                            font.bold: true; color: card.tint
                        }
                        Text {
                            text: card.label
                            font.family: FontIcons.display; font.pixelSize: 12
                            color: Theme.textMuted
                        }
                    }
                }

                Stat { value: Dashboard.activeSessions; label: qsTr("Active sessions") }
                Stat { value: Dashboard.runningAgents; label: qsTr("Running agents") }
                Stat {
                    value: Dashboard.pendingApprovals
                    label: qsTr("Pending approvals")
                    tint: Dashboard.pendingApprovals > 0 ? Theme.danger : Theme.accent
                }
                Stat { value: Dashboard.tokensToday; label: qsTr("Tokens today") }
                Stat {
                    value: Dashboard.healthy ? qsTr("OK") : qsTr("!")
                    label: qsTr("Daemon health")
                    tint: Dashboard.healthy ? Theme.accent : Theme.danger
                }
            }

            SectionLabel { text: qsTr("Recent activity") }

            Repeater {
                model: Dashboard.activity
                delegate: Rectangle {
                    required property var entry
                    Layout.fillWidth: true
                    implicitHeight: 40
                    radius: 6
                    color: Theme.surface
                    border.color: Theme.border
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 10
                        Text {
                            text: entry.kind === "approval" ? FontIcons.fa_circle_exclamation
                                : entry.kind === "error" ? FontIcons.fa_xmark
                                : FontIcons.fa_circle_check
                            font.family: FontIcons.faSolid; font.pixelSize: 12
                            color: entry.kind === "approval" ? Theme.danger
                                 : entry.kind === "error" ? Theme.danger : Theme.accent
                        }
                        Text {
                            text: entry.text
                            font.family: FontIcons.display; font.pixelSize: 13; color: Theme.text
                            Layout.fillWidth: true; elide: Text.ElideRight
                        }
                        Text {
                            text: entry.time
                            font.family: FontIcons.mono; font.pixelSize: 11; color: Theme.textMuted
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 12 }
        }
    }
}
