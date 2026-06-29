// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Timeline sub-tab: memory/recall/consolidation events grouped by day or session.
// Backed by MemoryTimelineModel (flattened header + event rows), shared with the
// TUI grouped view.
Item {
    id: root

    MemoryTimelineModel {
        id: timeline
        service: typeof Memory !== "undefined" ? Memory : null
        group: "day"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Text {
                text: qsTr("Group by")
                font.family: FontIcons.display
                font.pixelSize: 12
                color: Theme.textMuted
            }
            Kit.TextButton {
                text: qsTr("Day")
                accentFilled: timeline.group === "day"
                onClicked: timeline.group = "day"
            }
            Kit.TextButton {
                text: qsTr("Session")
                accentFilled: timeline.group === "session"
                onClicked: timeline.group = "session"
            }
            Item { Layout.fillWidth: true }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 8
            color: Theme.surface
            border.color: Theme.border

            ListView {
                anchors.fill: parent
                anchors.margins: 6
                clip: true
                model: timeline
                boundsBehavior: Flickable.StopAtBounds
                QQC.ScrollBar.vertical: Kit.ScrollBar {}

                delegate: Item {
                    required property bool isHeader
                    required property string groupKey
                    required property string eventKind
                    required property string summary
                    required property string at
                    width: ListView.view.width
                    height: isHeader ? 30 : 26

                    // Group header row.
                    Text {
                        visible: isHeader
                        anchors.left: parent.left
                        anchors.leftMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        text: groupKey
                        font.family: FontIcons.display
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        color: Theme.accent
                    }

                    // Event row.
                    RowLayout {
                        visible: !isHeader
                        anchors.fill: parent
                        anchors.leftMargin: 18
                        anchors.rightMargin: 6
                        spacing: 8
                        Rectangle {
                            radius: 3
                            implicitWidth: kindText.implicitWidth + 10
                            implicitHeight: 15
                            color: Theme.hover
                            Text {
                                id: kindText
                                anchors.centerIn: parent
                                text: eventKind
                                font.family: FontIcons.mono
                                font.pixelSize: 9
                                color: eventKind === "invalidated" ? Theme.danger : Theme.accent
                            }
                        }
                        Text {
                            text: summary
                            font.family: FontIcons.display
                            font.pixelSize: 11
                            color: Theme.text
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Text {
                            text: at
                            font.family: FontIcons.mono
                            font.pixelSize: 9
                            color: Theme.textMuted
                        }
                    }
                }
            }
        }
    }
}
