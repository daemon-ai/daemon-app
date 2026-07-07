// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme

// Themed MenuItem - a flat row with a rounded hover/highlight wash (Theme.hover),
// primary text, muted when disabled. The check indicator and submenu arrow are
// collapsed to zero width since the kit's menus are simple action lists.
QQC.MenuItem {
    id: root

    property string iconText: ""
    property string iconFamily: FontIcons.faSolid
    // Optional trailing count badge (TOOL-8): shows a pill with the count when > 0, hidden at 0.
    property int badgeCount: 0

    implicitHeight: 32
    leftPadding: 10
    rightPadding: 10
    spacing: 8

    indicator: Item { implicitWidth: 0; implicitHeight: 0 }
    arrow: Item { implicitWidth: 0; implicitHeight: 0 }

    contentItem: RowLayout {
        spacing: 10

        Glyph {
            visible: root.iconText !== ""
            Layout.preferredWidth: 16
            Layout.alignment: Qt.AlignVCenter
            glyph: root.iconText
            family: root.iconFamily
            font.pointSize: 12 + Theme.pointSizeOffset
            color: root.enabled ? Theme.iconMuted : Theme.textMuted
        }

        Text {
            Layout.fillWidth: true
            text: root.text
            font.family: FontIcons.display
            font.pixelSize: 13
            color: root.enabled ? Theme.text : Theme.textMuted
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        Badge {
            Layout.alignment: Qt.AlignVCenter
            count: root.badgeCount
        }
    }

    background: Rectangle {
        radius: 5
        color: root.highlighted ? Theme.hover : "transparent"
    }
}
