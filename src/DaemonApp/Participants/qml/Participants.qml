// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Participants

// Right-sidebar "Participants" section, mirroring the left sidebar's Tags section: a collapsible
// header followed by one row per chat participant, each with a colored presence dot (green when
// "available"). Sits above the file Explorer in the right panel and sizes itself to its content.
Rectangle {
    id: root

    color: Theme.sidebar
    implicitHeight: col.implicitHeight

    ParticipantsModel {
        id: participantsModel
        store: SessionStoreMirror
    }

    Column {
        id: col
        width: parent.width
        spacing: 1

        Repeater {
            model: participantsModel

            delegate: Item {
                id: del
                required property int index
                required property string label
                required property bool isSeparator
                required property bool hasChildren
                required property bool expanded
                required property string color
                required property string presence
                required property bool isAgent

                width: col.width
                height: isSeparator ? 30 : Theme.rowHeight

                // Hover pill for participant rows (matches the Tags rows).
                Rectangle {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.rowInset
                    anchors.rightMargin: Theme.rowInset
                    anchors.topMargin: Theme.rowVInset
                    anchors.bottomMargin: Theme.rowVInset
                    radius: Theme.rowRadius
                    visible: !del.isSeparator
                    color: Theme.sidebarHover
                    opacity: rowMouse.containsMouse ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: Theme.motionFast } }
                }

                // --- Separator row: twistie + section label ----------------
                Item {
                    anchors.fill: parent
                    visible: del.isSeparator

                    Kit.Glyph {
                        id: headerChevron
                        anchors.left: parent.left
                        anchors.leftMargin: 8
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 6
                        glyph: FontIcons.fa_chevron_right
                        font.pointSize: 9 + Theme.pointSizeOffset
                        rotation: del.expanded ? 90 : 0
                        color: Theme.separatorText
                        Behavior on rotation { NumberAnimation { duration: Theme.motionFast } }
                    }

                    QQC.Label {
                        anchors.left: headerChevron.right
                        anchors.leftMargin: 6
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 5
                        text: del.label
                        color: Theme.separatorText
                        font.family: FontIcons.display
                        font.pixelSize: Theme.labelSize
                        font.weight: Font.DemiBold
                        font.letterSpacing: Theme.labelTracking
                        font.capitalization: Font.AllUppercase
                    }
                }

                // --- Participant row: presence dot + name ------------------
                Item {
                    anchors.fill: parent
                    visible: !del.isSeparator

                    Item {
                        id: iconBox
                        anchors.left: parent.left
                        anchors.leftMargin: 14 + Theme.twistieSize
                        anchors.verticalCenter: parent.verticalCenter
                        width: 18
                        height: 18

                        // Presence dot (fa_circle), green for "available" participants.
                        Kit.Glyph {
                            anchors.centerIn: parent
                            glyph: FontIcons.fa_circle
                            font.pointSize: 12 + Theme.pointSizeOffset
                            color: del.color !== "" ? del.color : Theme.sidebarIcon
                        }
                    }

                    QQC.Label {
                        anchors.left: iconBox.right
                        anchors.leftMargin: 11
                        anchors.right: parent.right
                        anchors.rightMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        text: del.label
                        elide: Text.ElideRight
                        font.family: FontIcons.display
                        font.pixelSize: 13
                        font.weight: Font.Medium
                        color: rowMouse.containsMouse ? Theme.sidebarSelectedText : Theme.sidebarText
                    }
                }

                MouseArea {
                    id: rowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: del.isSeparator ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: {
                        if (del.isSeparator)
                            participantsModel.toggleExpand(del.index);
                    }
                }
            }
        }
    }
}
