// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import DaemonApp.Theme

// Theme picker chip, a circular color
// swatch above a label; hover/press swap the background; the selected one shows
// the accent-colored label.
MouseArea {
    id: root

    property string themeName: "Light"
    property color chipColor: "white"
    property bool selected: false

    signal picked()

    width: 71
    height: 71
    hoverEnabled: true
    cursorShape: Qt.PointingHandCursor

    onClicked: root.picked()

    Rectangle {
        id: backgroundRect
        anchors.fill: parent
        radius: 3
        color: root.pressed ? Theme.pressed
             : root.containsMouse ? Theme.hover
             : "transparent"

        Column {
            anchors.centerIn: parent
            spacing: 7

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 30
                height: 30
                radius: width / 2
                color: root.chipColor
                border.width: (root.themeName === "Dark" || root.themeName === "Midnight") ? 0 : 1
                border.color: "gray"
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.themeName
                color: root.selected ? Theme.accent : Theme.text
                font.family: FontIcons.display
                font.pixelSize: 13
            }
        }
    }
}
