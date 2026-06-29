// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme

// A single attachment pill below the composer input - a file glyph, the name,
// and a remove affordance. Drag-dropped files become these chips and ride the
// submitted message as @file: refs (there is no real upload pipeline yet).
Rectangle {
    id: root

    property string name: ""
    property string kind: "file" // "file" | "image" | "folder" | "url"

    signal removeRequested()

    implicitWidth: row.implicitWidth + 16
    implicitHeight: 26
    radius: Theme.radius
    color: Theme.codeBackground
    border.width: 1
    border.color: Theme.border

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: 6

        Text {
            text: root.kind === "image" ? FontIcons.fa_image
                : root.kind === "folder" ? FontIcons.fa_folder
                : root.kind === "url" ? FontIcons.fa_link
                : FontIcons.fa_paperclip
            font.family: FontIcons.faSolid
            font.pixelSize: 11
            color: Theme.textMuted
        }

        QQC.Label {
            Layout.maximumWidth: 160
            text: root.name
            font.family: FontIcons.display
            font.pixelSize: 12
            color: Theme.text
            elide: Text.ElideMiddle
            verticalAlignment: Text.AlignVCenter
        }

        Text {
            text: FontIcons.fa_xmark
            font.family: FontIcons.faSolid
            font.pixelSize: 11
            color: removeArea.containsMouse ? Theme.text : Theme.textMuted

            MouseArea {
                id: removeArea
                anchors.fill: parent
                anchors.margins: -4
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.removeRequested()
            }
        }
    }
}
