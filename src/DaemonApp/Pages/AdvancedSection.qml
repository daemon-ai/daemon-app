// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Settings
import DaemonApp.Controls as Kit

ColumnLayout {
    id: root
    spacing: 14

    SectionLabel { text: qsTr("Diagnostics") }

    ComboRow {
        Layout.fillWidth: true
        label: qsTr("Log level")
        configKey: "advanced/logLevel"
        options: DaemonConfig.options("advanced/logLevel")
    }
    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Send anonymous telemetry")
        checked: DaemonConfig.value("advanced/telemetry", false)
        onToggled: function(on) { DaemonConfig.setValue("advanced/telemetry", on); }
    }
    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Enable experimental tools")
        checked: DaemonConfig.value("advanced/experimentalTools", false)
        onToggled: function(on) { DaemonConfig.setValue("advanced/experimentalTools", on); }
    }

    SectionLabel { text: qsTr("Onboarding") }

    Kit.TextButton {
        text: qsTr("Re-run first-run setup")
        onClicked: FirstRun.restart()
    }

    Item { Layout.fillHeight: true }
}
