// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// A settings toggle row: caption on the left, themed Switch on the right. The
// whole row is a hover target; clicking anywhere requests the opposite state via
// toggled(newValue). State is unidirectional: the parent flips UiSettings, which
// flows back into `checked` (the Switch never mutates itself).
Item {
    id: root

    property string label: ""
    property bool checked: false

    signal toggled(bool checked)

    implicitWidth: parent ? parent.width : 200
    implicitHeight: 34

    // The whole row is the operable checkbox (the inner Switch is decorative and
    // ignored below), named for its caption.
    Accessible.role: Accessible.CheckBox
    Accessible.name: label
    Accessible.checkable: true
    Accessible.checked: checked
    Accessible.onToggleAction: root.toggled(!root.checked)
    Accessible.onPressAction: root.toggled(!root.checked)

    Rectangle {
        anchors.fill: parent
        radius: 5
        color: hover.containsMouse ? Theme.hover : "transparent"
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        text: root.label
        font.family: FontIcons.display
        font.pixelSize: 14
        color: Theme.text
    }

    Kit.Switch {
        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        checkable: false
        checked: root.checked
        // The row (above) is the named, operable checkbox; keep the visual
        // toggle out of the tree so a screen reader announces the row once.
        Accessible.ignored: true
    }

    MouseArea {
        id: hover
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.toggled(!root.checked)
    }
}
