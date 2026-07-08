// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme

// [wave2:app-delegation] F7/DEL-7: the node's delegation guardrail ceilings (Caps -> CapsReport),
// shown READ-ONLY. Delegation depth / fan-out are node-owned policy with no app-facing wire setter,
// so — like the sandbox rows in SafetySettingsSection — they are displayed, never written. Kept as
// a self-contained component so the settings page inserts it with a single tagged line (and the
// app-approvals-safety sibling can insert its own section beside it, unionably). `Caps` is null in
// mock mode; every bind is defensive on `Caps && Caps.loaded`.
ColumnLayout {
    id: root
    spacing: 14

    // Node-wide policy, re-fetched on show (never cached / stale-rendered as settable).
    Component.onCompleted: if (typeof Caps !== "undefined" && Caps) Caps.refresh()

    readonly property bool _have: typeof Caps !== "undefined" && Caps && Caps.loaded

    SectionLabel { text: qsTr("Delegation limits") }

    Text {
        Layout.fillWidth: true
        text: qsTr("How deep and how wide an agent may delegate. Enforced by the node; shown here for reference.")
        color: Theme.textMuted
        font.family: FontIcons.display
        font.pixelSize: 12
        wrapMode: Text.WordWrap
    }

    SettingRow {
        Layout.fillWidth: true
        enabled: false
        label: qsTr("Max delegation depth")
        description: qsTr("Enforced by the node — not configurable from the app.")

        Text {
            text: root._have ? String(Caps.orchestrateMaxDepth) : qsTr("—")
            color: Theme.text
            font.family: FontIcons.mono
            font.pixelSize: 13
        }
    }

    SettingRow {
        Layout.fillWidth: true
        enabled: false
        label: qsTr("Max background children")
        description: qsTr("Enforced by the node — not configurable from the app.")

        Text {
            text: root._have ? String(Caps.orchestrateMaxFanout) : qsTr("—")
            color: Theme.text
            font.family: FontIcons.mono
            font.pixelSize: 13
        }
    }

    // Agent-authored-profile ceilings (phase H): how many profiles a session may author and how
    // many concurrent ephemeral/inline children it may run. Advisory display; the node enforces.
    SettingRow {
        Layout.fillWidth: true
        enabled: false
        label: qsTr("Max composed profiles")
        description: qsTr("Profiles an agent may author. Enforced by the node.")

        Text {
            text: root._have ? String(Caps.maxComposedProfiles) : qsTr("—")
            color: Theme.text
            font.family: FontIcons.mono
            font.pixelSize: 13
        }
    }

    SettingRow {
        Layout.fillWidth: true
        enabled: false
        label: qsTr("Max ephemeral children per session")
        description: qsTr("Concurrent ephemeral children a session may run. Enforced by the node.")

        Text {
            text: root._have ? String(Caps.maxEphemeralPerSession) : qsTr("—")
            color: Theme.text
            font.family: FontIcons.mono
            font.pixelSize: 13
        }
    }
}
