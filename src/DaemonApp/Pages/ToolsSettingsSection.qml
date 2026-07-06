// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// [wave2:app-approvals-safety] D2: read-only Tools inventory. Renders the node's advertised tool
// set (ToolList -> Tools, wire v29): each tool's enabled state and, when disabled, the unmet
// requirement with honest copy. There is NO enable/disable control — the node owns tool gating
// (ToolRegister is a register verb, Unsupported at v29). For credential-shaped requirements the
// "Set up…" affordance deep-links to Connection, where the wave-1 auth stack owns key entry.
ColumnLayout {
    id: root
    spacing: 14

    Component.onCompleted: if (typeof Tools !== "undefined" && Tools) Tools.refresh()

    SectionLabel { text: qsTr("Tools") }

    Text {
        Layout.fillWidth: true
        text: qsTr("Tools are compiled and gated by the node. This inventory is read-only; a disabled tool names what it needs.")
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

                Kit.Chip {
                    Layout.alignment: Qt.AlignRight
                    iconGlyph: model.enabled ? FontIcons.fa_circle_check : FontIcons.fa_circle_xmark
                    tone: model.enabled ? "accent" : "muted"
                    text: model.enabled ? qsTr("Enabled") : qsTr("Disabled")
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
