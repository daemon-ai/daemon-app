// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Settings
import DaemonApp.Controls as Kit

// Notifications are a client-local concern (delivered via IPlatformServices), so
// they bind AppSettings rather than the daemon config.
ColumnLayout {
    id: root
    spacing: 14

    SectionLabel { text: qsTr("Desktop notifications") }

    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Notify when a turn needs my input")
        checked: AppSettings.value("notify/gates", true)
        onToggled: function(on) { AppSettings.setValue("notify/gates", on); }
    }
    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Notify when a turn finishes")
        checked: AppSettings.value("notify/turnDone", false)
        onToggled: function(on) { AppSettings.setValue("notify/turnDone", on); }
    }

    Kit.TextButton {
        text: qsTr("Send a test notification")
        onClicked: App.notifyGate(qsTr("daemon-app"), qsTr("Notifications are working."))
    }
    Text {
        text: qsTr("Notifications only appear while the window is hidden or inactive.")
        font.family: FontIcons.display; font.pixelSize: 12; color: Theme.textMuted
        Layout.fillWidth: true; wrapMode: Text.WordWrap
    }

    Item { Layout.fillHeight: true }
}
