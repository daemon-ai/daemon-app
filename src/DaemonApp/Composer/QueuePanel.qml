// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Queued-prompt rows docked above the composer - the QML port of Hermes'
// QueuePanel (apps/desktop/src/app/chat/composer/queue-panel.tsx). Each row
// shows the queued text with send-now / edit / delete affordances. Backed by a
// ListModel of { text } owned by the composer.
ColumnLayout {
    id: root

    property var model: null
    property int editingIndex: -1
    property bool busy: false

    signal sendNow(int index)
    signal editEntry(int index)
    signal deleteEntry(int index)

    spacing: 2

    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 4
        Layout.rightMargin: 4
        Layout.bottomMargin: 2
        spacing: 6

        Text {
            text: FontIcons.fa_layer_group
            font.family: FontIcons.faSolid
            font.pixelSize: 10
            color: Theme.textMuted
        }
        QQC.Label {
            Layout.fillWidth: true
            text: root.model ? qsTr("Queued (%1)").arg(root.model.count) : qsTr("Queued")
            font.family: FontIcons.display
            font.pixelSize: Theme.labelSize
            font.weight: Font.DemiBold
            font.letterSpacing: Theme.labelTracking
            font.capitalization: Font.AllUppercase
            color: Theme.textMuted
        }
    }

    Repeater {
        model: root.model
        delegate: Rectangle {
            id: rowRoot
            required property int index
            required property string text
            Layout.fillWidth: true
            implicitHeight: 32
            radius: 6
            color: rowRoot.index === root.editingIndex ? Theme.activeBlockBackground
                 : rowArea.containsMouse ? Theme.hover : "transparent"

            HoverHandler { id: rowArea }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 4
                spacing: 6

                QQC.Label {
                    Layout.fillWidth: true
                    text: rowRoot.text
                    font.family: FontIcons.display
                    font.pixelSize: 12
                    color: Theme.text
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }

                Kit.IconButton {
                    implicitWidth: 26
                    implicitHeight: 26
                    icon: FontIcons.fa_arrow_up
                    iconColor: Theme.iconMuted
                    iconPointSize: 11
                    tooltipText: root.busy ? qsTr("Send next (interrupts)") : qsTr("Send now")
                    onClicked: root.sendNow(rowRoot.index)
                }
                Kit.IconButton {
                    implicitWidth: 26
                    implicitHeight: 26
                    icon: FontIcons.fa_pen_to_square
                    iconColor: Theme.iconMuted
                    iconPointSize: 11
                    tooltipText: qsTr("Edit")
                    onClicked: root.editEntry(rowRoot.index)
                }
                Kit.IconButton {
                    implicitWidth: 26
                    implicitHeight: 26
                    icon: FontIcons.fa_xmark
                    iconColor: Theme.iconMuted
                    iconPointSize: 12
                    tooltipText: qsTr("Remove")
                    onClicked: root.deleteEntry(rowRoot.index)
                }
            }
        }
    }
}
