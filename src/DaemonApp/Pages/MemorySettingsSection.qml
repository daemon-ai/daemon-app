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

    SectionLabel { text: qsTr("Context window") }

    SettingRow {
        label: qsTr("Max context tokens")
        Kit.TextField {
            implicitWidth: 120
            inputMethodHints: Qt.ImhDigitsOnly
            text: String(DaemonConfig.value("memory/contextWindow", 128000))
            onEditingFinished: DaemonConfig.setValue("memory/contextWindow", parseInt(text) || 0)
        }
    }
    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Auto-compact when context is full")
        checked: DaemonConfig.value("memory/autoCompact", true)
        onToggled: function(on) { DaemonConfig.setValue("memory/autoCompact", on); }
    }

    SectionLabel { text: qsTr("Long-term memory") }

    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Persist memory across sessions")
        checked: DaemonConfig.value("memory/persistMemory", true)
        onToggled: function(on) { DaemonConfig.setValue("memory/persistMemory", on); }
    }

    Item { Layout.fillHeight: true }
}
