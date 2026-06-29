// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import DaemonApp.Theme

// A neutral "stub" panel used while a section/page is still being wired to its
// (mock) seam. Carries a short label so each placeholder is identifiable.
Item {
    id: root
    property string label: qsTr("Coming soon")

    Column {
        anchors.centerIn: parent
        spacing: 10

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: FontIcons.fa_sliders
            font.family: FontIcons.faSolid
            font.pixelSize: 28
            color: Theme.border
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.label
            font.family: FontIcons.display
            font.pixelSize: 14
            color: Theme.textMuted
        }
    }
}
