// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import DaemonApp.Theme

// A compact themed SpinBox: bordered transparent fill with the accent focus
// ring (a la Kit.Dropdown), a centered value, and glyph -/+ steppers that get
// the kit's hover/pressed washes (a la Kit.IconButton). Steppers dim when the
// value is pinned at a bound.
QQC.SpinBox {
    id: root

    implicitWidth: 96
    implicitHeight: 26
    hoverEnabled: true
    font.family: FontIcons.display
    font.pixelSize: 12

    contentItem: TextInput {
        z: 2
        text: root.displayText
        font: root.font
        color: Theme.text
        selectionColor: Theme.selection
        selectedTextColor: Theme.selectionText
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        readOnly: !root.editable
        validator: root.validator
        inputMethodHints: root.inputMethodHints
        selectByMouse: true
    }

    down.indicator: Item {
        x: 0
        width: root.height
        height: root.height

        Rectangle {
            anchors.fill: parent
            anchors.margins: 2
            radius: Theme.radiusSmall
            color: root.down.pressed ? Theme.pressed
                 : root.down.hovered ? Theme.hover
                 : "transparent"
        }
        Text {
            anchors.centerIn: parent
            text: FontIcons.fa_minus
            font.family: FontIcons.faSolid
            font.pixelSize: 8
            color: Theme.text
            opacity: root.value > root.from || root.wrap ? 1.0 : 0.35
        }
        HoverHandler { cursorShape: Qt.PointingHandCursor }
    }

    up.indicator: Item {
        x: root.width - width
        width: root.height
        height: root.height

        Rectangle {
            anchors.fill: parent
            anchors.margins: 2
            radius: Theme.radiusSmall
            color: root.up.pressed ? Theme.pressed
                 : root.up.hovered ? Theme.hover
                 : "transparent"
        }
        Text {
            anchors.centerIn: parent
            text: FontIcons.fa_plus
            font.family: FontIcons.faSolid
            font.pixelSize: 8
            color: Theme.text
            opacity: root.value < root.to || root.wrap ? 1.0 : 0.35
        }
        HoverHandler { cursorShape: Qt.PointingHandCursor }
    }

    background: Rectangle {
        radius: 3
        color: "transparent"
        border.width: root.activeFocus ? 2 : 1
        border.color: root.activeFocus ? Theme.accent : Theme.border
    }
}
