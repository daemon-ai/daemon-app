// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme

// A fully-controlled toggle row (label + description + checkbox-style indicator)
// used by the Skills/tools curator. `checked` is driven by the parent's working
// set; clicking emits `toggled` for the parent to mutate that set. Controlled (no
// internal state) so it never breaks its binding.
Item {
    id: root

    property string label: ""
    property string description: ""
    property bool checked: false

    signal toggled()

    implicitHeight: 44

    // The whole row is a checkbox named for its label, with the sub-line as its
    // description.
    Accessible.role: Accessible.CheckBox
    Accessible.name: label
    Accessible.description: description
    Accessible.checkable: true
    Accessible.checked: checked
    Accessible.onToggleAction: root.toggled()
    Accessible.onPressAction: root.toggled()

    Rectangle {
        anchors.fill: parent
        radius: 6
        color: hover.hovered ? Theme.hover : "transparent"
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 10

        Rectangle {
            implicitWidth: 18; implicitHeight: 18
            radius: 3
            color: root.checked ? Theme.accent : "transparent"
            border.color: root.checked ? Theme.accent : Theme.textMuted

            Text {
                anchors.centerIn: parent
                visible: root.checked
                text: FontIcons.check
                font.family: FontIcons.faSolid
                font.pixelSize: 11
                color: Theme.background
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 1
            Text {
                text: root.label
                font.family: FontIcons.display; font.pixelSize: 13; color: Theme.text
            }
            Text {
                visible: root.description.length > 0
                text: root.description
                font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
                elide: Text.ElideRight; Layout.fillWidth: true
            }
        }
    }

    HoverHandler { id: hover }
    TapHandler { onTapped: root.toggled() }
}
