// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme

// A compact themed chip: label + optional leading glyph + optional close
// affordance. Tones tint the text/border only (no dedicated background
// tokens): "neutral" | "accent" | "muted" | "danger". Set `interactive` to
// make the whole chip clickable (hover/pressed feedback + clicked()).
Rectangle {
    id: root

    property string text: ""
    property string iconGlyph: ""
    property string iconFamily: FontIcons.faSolid
    property string tone: "neutral"
    property bool interactive: false
    property bool closable: false
    property string tooltipText: ""

    signal clicked()
    signal closeRequested()

    readonly property color toneColor: tone === "accent" ? Theme.accent
                                     : tone === "danger" ? Theme.danger
                                     : tone === "muted" ? Theme.textMuted
                                     : Theme.text

    implicitWidth: row.implicitWidth + 16
    implicitHeight: 22
    radius: Theme.radius
    color: interactive && chipArea.pressed ? Theme.pressed
         : interactive && chipArea.containsMouse ? Theme.hover
         : Theme.codeBackground
    border.width: 1
    border.color: tone === "accent" || tone === "danger"
                  ? Qt.alpha(root.toneColor, 0.55) : Theme.border
    opacity: enabled ? 1.0 : 0.5

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: 5

        Text {
            visible: root.iconGlyph.length > 0
            text: root.iconGlyph
            font.family: root.iconFamily
            font.pixelSize: 10
            renderType: Text.NativeRendering
            color: root.tone === "neutral" ? Theme.textMuted : root.toneColor
            verticalAlignment: Text.AlignVCenter
        }

        QQC.Label {
            Layout.maximumWidth: 180
            text: root.text
            font.family: FontIcons.display
            font.pixelSize: 11
            color: root.toneColor
            elide: Text.ElideMiddle
            verticalAlignment: Text.AlignVCenter
        }

        Text {
            visible: root.closable
            text: FontIcons.fa_xmark
            font.family: FontIcons.faSolid
            font.pixelSize: 10
            renderType: Text.NativeRendering
            color: closeArea.containsMouse ? Theme.text : Theme.textMuted

            MouseArea {
                id: closeArea
                anchors.fill: parent
                anchors.margins: -4
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.closeRequested()
            }
        }
    }

    MouseArea {
        id: chipArea
        anchors.fill: parent
        // Below the close affordance so the xmark keeps its own hit area.
        z: -1
        hoverEnabled: true
        enabled: root.interactive || root.tooltipText.length > 0
        cursorShape: root.interactive ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: if (root.interactive) root.clicked()
    }

    Tooltip {
        text: root.tooltipText
        visible: root.tooltipText.length > 0 && chipArea.containsMouse
        x: Math.round((root.width - implicitWidth) / 2)
        y: root.height + 6
    }
}
