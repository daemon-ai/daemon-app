// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme

// A labelled settings row: a title + optional description on the left and a
// caller-supplied control on the right (placed via the default property).
RowLayout {
    id: root
    property string label: ""
    property string description: ""
    default property alias control: holder.data

    Layout.fillWidth: true
    spacing: 12

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2
        Text {
            text: root.label
            font.family: FontIcons.display
            font.pixelSize: 14
            // A disabled row (a node-owned setting shown read-only) mutes its
            // title so it reads as non-editable, not just as a caption.
            color: root.enabled ? Theme.text : Theme.textMuted
            Layout.fillWidth: true
        }
        Text {
            visible: root.description.length > 0
            text: root.description
            font.family: FontIcons.display
            font.pixelSize: 12
            color: Theme.textMuted
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }

    Item {
        id: holder
        Layout.alignment: Qt.AlignVCenter
        implicitWidth: childrenRect.width
        implicitHeight: childrenRect.height
    }
}
