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

    SectionLabel { text: qsTr("Workspace root") }

    Kit.TextField {
        Layout.fillWidth: true
        placeholderText: qsTr("Path the agent operates in")
        text: DaemonConfig.value("workspace/root", "")
        onEditingFinished: DaemonConfig.setValue("workspace/root", text)
    }

    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Respect .gitignore")
        checked: DaemonConfig.value("workspace/followGitignore", true)
        onToggled: function(on) { DaemonConfig.setValue("workspace/followGitignore", on); }
    }

    Item { Layout.fillHeight: true }
}
