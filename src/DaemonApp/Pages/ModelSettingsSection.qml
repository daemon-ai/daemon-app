// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Settings
import DaemonApp.Controls as Kit

// Default-model settings (daemon-authoritative). The model itself is chosen in
// the Models hub; here we set the default and the inference effort/fast knobs.
ColumnLayout {
    id: root
    spacing: 14

    SectionLabel { text: qsTr("Default model") }

    SettingRow {
        label: qsTr("Active default")
        description: qsTr("Change or install models in the Models hub.")
        RowLayout {
            spacing: 8
            Text {
                // The live active model from the shared catalog (the same source
                // the composer picker activates), not a static config string.
                text: ModelCatalog.currentModelId !== "" ? ModelCatalog.currentModelId : "-"
                font.family: FontIcons.mono; font.pixelSize: 13; color: Theme.text
            }
            Kit.TextButton {
                text: qsTr("Open Models")
                onClicked: Nav.open("models")
            }
        }
    }

    SectionLabel { text: qsTr("Inference") }

    ComboRow {
        Layout.fillWidth: true
        label: qsTr("Reasoning effort")
        configKey: "model/effort"
        options: DaemonConfig.options("model/effort")
    }
    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Fast mode (lower latency)")
        checked: DaemonConfig.value("model/fast", false)
        onToggled: function(on) { DaemonConfig.setValue("model/fast", on); }
    }

    Item { Layout.fillHeight: true }
}
