// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC
import DaemonApp.Theme

Rectangle {
    id: root

    color: Theme.background

    signal closeRequested()

    implicitHeight: message.implicitHeight + 24

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: Theme.spacing

        QQC.Label {
            id: message
            Layout.fillWidth: true
            color: Theme.textMuted
            text: qsTr("Embedded terminal is not available in the browser build.")
            wrapMode: Text.WordWrap
        }

        QQC.Button {
            text: qsTr("Hide")
            onClicked: root.closeRequested()
        }
    }
}
