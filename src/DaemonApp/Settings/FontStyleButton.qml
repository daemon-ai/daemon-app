// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// One of the three "Style" cards (Sans / Serif / Mono): a large "Ag" sample in
// the category's font (accent-colored when active), a caption, and a font picker
// dropdown beneath. Clicking the sample activates/cycles the category; the
// dropdown selects a specific font within it.
Column {
    id: root

    property string category: "Sans"
    property string label: qsTr("Sans")
    property string previewFamily: ""
    property var fonts: []
    property int currentIndex: 0
    property bool selected: false

    signal sampleClicked()
    signal fontPicked(int index)

    spacing: 4

    MouseArea {
        id: sample
        width: 71
        height: 61
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.sampleClicked()

        // The specimen card is a checkable style choice named for its category.
        Accessible.role: Accessible.RadioButton
        Accessible.name: root.label
        Accessible.checkable: true
        Accessible.checked: root.selected
        Accessible.onPressAction: root.sampleClicked()

        Rectangle {
            anchors.fill: parent
            radius: 3
            color: sample.containsMouse ? Theme.hover : "transparent"

            Column {
                anchors.centerIn: parent
                spacing: 2

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Ag", "font specimen sample")
                    font.family: root.previewFamily
                    font.pixelSize: 28
                    font.weight: Font.Medium
                    color: root.selected ? Theme.accent : Theme.text
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: root.label
                    font.family: FontIcons.display
                    font.pixelSize: 12
                    color: Theme.textMuted
                }
            }
        }
    }

    Kit.Dropdown {
        width: 71
        model: root.fonts
        currentIndex: root.currentIndex
        accessibleName: qsTr("%1 font").arg(root.label)
        onActivated: function(index) { root.fontPicked(index); }
    }
}
