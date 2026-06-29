// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls
import DaemonApp.Theme

// A bespoke font-glyph button, a MouseArea
// over a rounded Rectangle that swaps to hover/pressed token colors, with a
// centered FontAwesome glyph. Token-driven (reads Theme/FontIcons directly).
Item {
    id: root

    property string icon
    property string iconFontFamily: FontIcons.faSolid
    property color iconColor: Theme.iconMuted
    property int iconPointSize: 16
    property real backgroundRadius: Theme.radius
    property bool usePointingHand: true
    property string tooltipText: ""

    property alias containsMouse: mouseArea.containsMouse
    property alias pressedState: mouseArea.pressed

    // Finger-sized on touch (Theme.touch), dense 30x28 chrome on desktop.
    implicitWidth: Theme.iconButtonWidth
    implicitHeight: Theme.iconButtonHeight

    signal clicked()
    signal pressedSignal()
    signal released()
    signal pressedAndHold()

    Rectangle {
        id: background
        anchors.fill: parent
        radius: root.backgroundRadius
        color: !root.enabled ? "transparent"
             : mouseArea.pressed ? Theme.pressed
             : mouseArea.containsMouse ? Theme.hover
             : "transparent"

        Behavior on color {
            ColorAnimation { duration: Theme.motionFast }
        }

        Text {
            id: glyph
            anchors.centerIn: parent
            text: root.icon
            font.family: root.iconFontFamily
            font.pointSize: root.iconPointSize + Theme.pointSizeOffset
            renderType: Text.NativeRendering
            color: root.iconColor
            opacity: root.enabled ? 1.0 : 0.35
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        cursorShape: root.usePointingHand ? Qt.PointingHandCursor : Qt.ArrowCursor

        onClicked: root.clicked()
        onPressed: root.pressedSignal()
        onReleased: root.released()
        onPressAndHold: root.pressedAndHold()
    }

    // Themed tooltip (the attached ToolTip would fall back to the bland Basic
    // style). Centered just below the button.
    Tooltip {
        text: root.tooltipText
        visible: root.tooltipText.length > 0 && mouseArea.containsMouse
        x: Math.round((root.width - implicitWidth) / 2)
        y: root.height + 6
    }
}
