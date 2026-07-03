// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Stand-in for the embedded shell terminal on builds that cannot carry the
// real QMLTermWidget panel: the browser (a native PTY cannot exist in the
// wasm sandbox) and the portable static-Qt desktop build (QMLTermWidget is a
// shared qmake-built plugin, kept out of the static artifact - see
// THIRD-PARTY-NOTICES.md). Compiled into the same DaemonApp.Terminal module
// under the same TerminalPanel type name (see CMakeLists.txt). It keeps the
// public surface its hosts rely on - the closeRequested() signal Session.qml
// wires to UiSettings.showTerminal - and shows a themed notice instead of a
// shell.
Rectangle {
    id: root

    color: Theme.background

    // Same contract as the desktop panel: the host flips the terminal toggle
    // off in response (the panel never hides itself).
    signal closeRequested()

    Column {
        anchors.centerIn: parent
        spacing: Theme.spacing

        Kit.Glyph {
            anchors.horizontalCenter: parent.horizontalCenter
            glyph: FontIcons.fa_terminal
            font.pointSize: 24 + Theme.pointSizeOffset
            color: Theme.border
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("The terminal is not available in this build")
            color: Theme.textMuted
            font.family: FontIcons.display
            font.pixelSize: 14
        }

        Kit.TextButton {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Close")
            onClicked: root.closeRequested()
        }
    }
}
