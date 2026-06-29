// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Settings

// Settings -> Connection. Shows the live gateway state and hosts the shared
// ConnectionPicker so reconfiguring the node uses the exact same control as the
// first-run gate.
ColumnLayout {
    id: root
    spacing: 16

    SectionLabel { text: qsTr("Status") }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8
        Rectangle {
            width: 10; height: 10; radius: 5
            color: Connection && Connection.ready ? Theme.accent
                 : (Connection && Connection.state === "offline" ? Theme.danger : Theme.warning)
        }
        Text {
            text: Connection ? (Connection.state + "  -  " + Connection.mode
                                + (Connection.target.length ? "  (" + Connection.target + ")" : ""))
                             : qsTr("unknown")
            font.family: FontIcons.mono; font.pixelSize: 13; color: Theme.textMuted
            Layout.fillWidth: true
        }
        // Drop the active connection (data stays readable from cache). Hidden once
        // already offline so the action only appears when there's a live link.
        Kit.TextButton {
            text: qsTr("Disconnect")
            visible: Connection && Connection.state !== "offline"
            onClicked: Connection.disconnect()
        }
    }

    ConnectionPicker {
        Layout.fillWidth: true
        showConnect: true
    }

    SectionLabel { text: qsTr("Local daemon") }

    // CON-1b: when "Local" can't reach a running daemon, the client can start one for you.
    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Start a local daemon automatically")
        checked: AppSettings.managedLocalDaemon
        onToggled: function(on) { AppSettings.managedLocalDaemon = on; }
    }
    // A daemon the client started keeps running by default (background work outlives the window);
    // enable this to stop it on close. Only affects daemons the client itself spawned.
    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Stop the managed daemon when I close the app")
        checked: AppSettings.managedDaemonShutdownOnExit
        onToggled: function(on) { AppSettings.managedDaemonShutdownOnExit = on; }
    }
}
