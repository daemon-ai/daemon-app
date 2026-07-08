// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// The SHARED foreign-agent backend sub-form (Phase D), rendered over the shared AgentSetupModel.
// A foreign profile is `engine + backend + inference`; this component is the backend dimension:
//
//   (o) Agent's own backend      "uses the agent's login/config; the node steers its model only"
//   ( ) Node provider (gateway)  "routed through the node's gateway; keys stay on the node"
//
// Selecting a row drives `AgentSetup.chooseBackend(mode)`; the model's `needsInference`
// (== NodeProvider) then reveals the shared inference sub-form at the hosting surface. AgentNative
// exposes an optional model-steer hint (`AgentSetup.setAgentNativeModel`). When NodeProvider is
// chosen while the node's OpenAI gateway is disabled the model reports `gatewayRequired`, so we
// surface an inline "gateway disabled" affordance whose Enable arms `AgentSetup.gatewayEnabled`
// (the phase-F gateway surface takes over the real GatewayGet/Set later).
//
// Reused by AgentSetupForm, the first-run wizard's agent-type pane, and the profile editor, so the
// three surfaces render one backend UX that cannot drift. Selection-only presentation: the model
// owns all backend state and validation.
ColumnLayout {
    id: backend
    spacing: 6

    readonly property bool _hasSetup: (typeof AgentSetup !== "undefined" && AgentSetup)
    readonly property string mode: backend._hasSetup ? AgentSetup.backendMode : "AgentNative"

    // The two backend modes, node-neutral labels + one-line explanations (translated here).
    readonly property var _rows: [
        { mode: "AgentNative",
          title: qsTr("Agent's own backend"),
          hint: qsTr("Uses the agent's own login and config; the node only steers its model.") },
        { mode: "NodeProvider",
          title: qsTr("Node provider (gateway)"),
          hint: qsTr("Routed through the node's provider gateway; keys stay on the node.") }
    ]

    Repeater {
        model: backend._rows
        delegate: Rectangle {
            id: row
            required property var modelData
            readonly property bool selected: backend.mode === modelData.mode
            Layout.fillWidth: true
            implicitHeight: rowCol.implicitHeight + 12
            radius: 6
            color: row.selected ? Theme.hover : "transparent"
            border.width: 1
            border.color: row.selected ? Theme.accent : Theme.border

            ColumnLayout {
                id: rowCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 1
                Text {
                    text: row.modelData.title
                    font.family: FontIcons.display; font.pixelSize: 13
                    color: Theme.text; Layout.fillWidth: true; elide: Text.ElideRight
                }
                Text {
                    text: row.modelData.hint
                    font.family: FontIcons.display; font.pixelSize: 11
                    color: Theme.textMuted; Layout.fillWidth: true; wrapMode: Text.WordWrap
                }
            }
            TapHandler {
                onTapped: if (backend._hasSetup) AgentSetup.chooseBackend(row.modelData.mode)
            }
        }
    }

    // Inline "gateway disabled" affordance for a NodeProvider backend (the node's OpenAI gateway is
    // off): the node routes NodeProvider agents through it, so it must be enabled first. Enable arms
    // the model's gateway flag (the phase-F gateway surface owns the real GatewayGet/Set).
    RowLayout {
        Layout.fillWidth: true
        visible: backend._hasSetup && AgentSetup.gatewayRequired
        spacing: 8
        Text {
            text: qsTr("The node's provider gateway is disabled.")
            font.family: FontIcons.display; font.pixelSize: 12; color: Theme.danger
            Layout.fillWidth: true; wrapMode: Text.WordWrap
        }
        Kit.TextButton {
            text: qsTr("Enable")
            onClicked: if (backend._hasSetup) AgentSetup.gatewayEnabled = true
        }
    }

    // Optional model-steer hint (AgentNative only): the node best-effort steers the agent's model
    // over ACP; empty leaves the agent's own default.
    SectionLabel {
        visible: backend.mode === "AgentNative"
        text: qsTr("Model hint (optional)")
    }
    Kit.TextField {
        visible: backend.mode === "AgentNative"
        Layout.fillWidth: true
        placeholderText: qsTr("model id the agent should prefer")
        text: backend._hasSetup ? AgentSetup.agentNativeModel : ""
        onTextEdited: if (backend._hasSetup) AgentSetup.setAgentNativeModel(text)
    }
}
