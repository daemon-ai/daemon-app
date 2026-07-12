// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme

// A reusable left navigation rail for the split-pane pages (settings sections,
// command-center sections, ...). `items` is a JS array of { id, label, icon }.
// The current id is highlighted; clicking a row emits selected(id).
Rectangle {
    id: root

    property var items: []
    property string current: ""
    signal selected(string id)

    implicitWidth: 196
    color: Theme.surface

    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: Theme.border
    }

    ListView {
        id: list
        anchors.fill: parent
        anchors.topMargin: 8
        anchors.bottomMargin: 8
        clip: true
        model: root.items
        spacing: 1

        delegate: Item {
            id: row
            required property var modelData
            required property int index
            width: list.width
            height: 36

            readonly property bool isCurrent: root.current === modelData.id

            Rectangle {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                radius: 6
                color: row.isCurrent ? Theme.hover
                                     : (hover.hovered ? Theme.hover : "transparent")
                opacity: row.isCurrent ? 1.0 : (hover.hovered ? 0.6 : 1.0)
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 14
                spacing: 12

                Text {
                    text: row.modelData.icon === undefined ? "" : row.modelData.icon
                    font.family: FontIcons.faSolid
                    font.pixelSize: 14
                    color: row.isCurrent ? Theme.accent : Theme.iconMuted
                    Layout.preferredWidth: 16
                    horizontalAlignment: Text.AlignHCenter
                }
                Text {
                    text: row.modelData.label
                    font.family: FontIcons.display
                    font.pixelSize: 13
                    font.weight: row.isCurrent ? Font.DemiBold : Font.Normal
                    color: row.isCurrent ? Theme.text : Theme.textMuted
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            // Selectable navigation entry named for its label.
            Accessible.role: Accessible.ListItem
            Accessible.name: row.modelData.label === undefined ? "" : row.modelData.label
            Accessible.selectable: true
            Accessible.selected: row.isCurrent
            Accessible.onPressAction: root.selected(row.modelData.id)

            HoverHandler { id: hover }
            TapHandler { onTapped: root.selected(row.modelData.id) }
        }
    }
}
