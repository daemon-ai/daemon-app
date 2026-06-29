// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Settings

ColumnLayout {
    id: root
    spacing: 14

    SectionLabel { text: qsTr("Voice input") }

    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Enable voice transcription")
        checked: DaemonConfig.value("voice/enabled", false)
        onToggled: function(on) { DaemonConfig.setValue("voice/enabled", on); }
    }
    ComboRow {
        Layout.fillWidth: true
        label: qsTr("Transcription model")
        configKey: "voice/model"
        options: DaemonConfig.options("voice/model")
    }

    Item { Layout.fillHeight: true }
}
