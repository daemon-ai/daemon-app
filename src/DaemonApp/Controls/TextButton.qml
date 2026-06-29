// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import DaemonApp.Theme

// A flat/filled text button themed from tokens. Set `accentFilled` for a primary
// (accent background) button, otherwise it is a subtle hover/pressed button.
QQC.Button {
    id: root

    property bool accentFilled: false
    // Fill color used when accentFilled (defaults to the accent; set to a danger
    // token for destructive primary actions).
    property color fillColor: Theme.accent

    font.family: FontIcons.display
    font.pixelSize: 14
    leftPadding: Theme.spacing
    rightPadding: Theme.spacing
    topPadding: Theme.spacingSmall
    bottomPadding: Theme.spacingSmall

    contentItem: Text {
        text: root.text
        font: root.font
        color: root.accentFilled ? Theme.background
             : root.enabled ? Theme.text : Theme.textMuted
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: Theme.radius
        color: {
            if (root.accentFilled)
                return root.enabled ? (root.down ? Qt.darker(root.fillColor, 1.15) : root.fillColor)
                                    : Theme.hover;
            return root.down ? Theme.pressed : root.hovered ? Theme.hover : "transparent";
        }
        border.width: root.accentFilled ? 0 : 1
        border.color: Theme.border
    }
}
