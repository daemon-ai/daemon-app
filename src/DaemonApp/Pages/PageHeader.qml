// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme

// The shared header strip for an app-level page tab: a title (optional icon) on
// the left. Pages now live in singleton tabs, so closing is handled by the tab
// chip's own close button - there is no in-page close affordance here.
Rectangle {
    id: root

    property string title: ""
    property string icon: ""

    implicitHeight: 48
    color: "transparent"

    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: Theme.border
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 10

        Text {
            visible: root.icon.length > 0
            text: root.icon
            font.family: FontIcons.faSolid
            font.pixelSize: 16
            color: Theme.textMuted
        }
        Text {
            text: root.title
            font.family: FontIcons.display
            font.pixelSize: 16
            font.weight: Font.DemiBold
            color: Theme.text
            Layout.fillWidth: true
        }
    }
}
