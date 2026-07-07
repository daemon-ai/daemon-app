// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// [waveB:app-v30] D4: read-WRITE Tools inventory. Renders the node's advertised tool set
// (ToolList -> Tools) with an enable/disable toggle (ToolSetEnabled, wire v30). The node stays
// authoritative: toggling re-fetches ToolList, so a force-disabled / build-gated tool comes back
// disabled with its unmet requirement — the client renders whatever the node returns. For
// credential-shaped requirements the "Set up…" affordance deep-links to Connection, where the
// wave-1 auth stack owns key entry.
ColumnLayout {
    id: root
    spacing: 14

    Component.onCompleted: if (typeof Tools !== "undefined" && Tools) Tools.refresh()

    SectionLabel { text: qsTr("Tools") }

    Text {
        Layout.fillWidth: true
        text: qsTr("Tools are gated by the node. Toggling asks the node to enable or disable a tool; a tool that names a requirement stays disabled until it is met.")
        color: Theme.textMuted
        font.family: FontIcons.display
        font.pixelSize: 12
        wrapMode: Text.WordWrap
    }

    Text {
        Layout.fillWidth: true
        visible: !Tools || Tools.count === 0
        text: qsTr("No tools reported by the node.")
        color: Theme.textMuted
        font.family: FontIcons.display
        font.pixelSize: 12
    }

    Repeater {
        model: Tools ? Tools.tools : null
        delegate: SettingRow {
            id: toolRow
            required property var model
            Layout.fillWidth: true
            label: model.name
            description: model.description

            ColumnLayout {
                spacing: 4

                // [waveB:app-v30] D4: enable/disable toggle (ToolSetEnabled). The node is
                // authoritative — the model re-reads from the re-fetched ToolList, so a
                // build-gated tool snaps back to disabled with its requirement shown.
                Kit.Switch {
                    Layout.alignment: Qt.AlignRight
                    checked: model.enabled
                    onToggled: if (typeof Tools !== "undefined" && Tools)
                                   Tools.setEnabled(model.name, checked)
                }

                Text {
                    Layout.alignment: Qt.AlignRight
                    visible: !model.enabled && model.requirementLabel
                             && String(model.requirementLabel).length > 0
                    text: model.requirementLabel
                    font.family: FontIcons.display
                    font.pixelSize: 11
                    color: Theme.textMuted
                }

                Kit.TextButton {
                    Layout.alignment: Qt.AlignRight
                    visible: !model.enabled && model.requirementIsCredential === true
                    text: qsTr("Set up…")
                    // The credential is owned by the wave-1 auth stack; deep-link to Connection
                    // rather than duplicating key entry here (coordinator Q3).
                    onClicked: Nav.open("settings", "connection")
                }
            }
        }
    }

    Item { Layout.fillHeight: true }
}
