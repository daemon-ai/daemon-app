// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Settings

ColumnLayout {
    id: root
    spacing: 14

    SectionLabel { text: qsTr("Behaviour") }

    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Stream responses")
        checked: DaemonConfig.value("chat/streaming", true)
        onToggled: function(on) { DaemonConfig.setValue("chat/streaming", on); }
    }
    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Send on Enter (Shift+Enter for newline)")
        checked: DaemonConfig.value("chat/sendOnEnter", true)
        onToggled: function(on) { DaemonConfig.setValue("chat/sendOnEnter", on); }
    }
    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Show token counts")
        checked: DaemonConfig.value("chat/showTokenCounts", true)
        onToggled: function(on) { DaemonConfig.setValue("chat/showTokenCounts", on); }
    }

    Item { Layout.fillHeight: true }
}
