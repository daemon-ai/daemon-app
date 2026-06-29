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

    SectionLabel { text: qsTr("System prompt") }

    Kit.TextField {
        Layout.fillWidth: true
        placeholderText: qsTr("Optional default system prompt for new chats")
        text: DaemonConfig.value("chat/systemPrompt", "")
        onEditingFinished: DaemonConfig.setValue("chat/systemPrompt", text)
    }

    Item { Layout.fillHeight: true }
}
