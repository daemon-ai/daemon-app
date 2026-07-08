// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Settings -> Gateway. The node's resident OpenAI-compatible gateway (GatewayGet/GatewaySet ->
// GatewayStatus): an enable toggle, an optional bind address, the live listening / last-error
// status line, and a node service-health readout. All node-authoritative — the app renders the
// status and sends intents (enable/disable, set addr); it never derives gateway state locally and
// never surfaces gateway tokens/credentials (the wire carries none). `Gateway` is null in mock
// mode and `Gateway.supported` is false on a node that predates the gateway; both cases fall back
// to a graceful "unavailable" message. Health rows come from the shared Status (StatusBarModel),
// fed by the single DaemonConnectionService health cache.
ColumnLayout {
    id: root
    spacing: 16

    readonly property bool _have: typeof Gateway !== "undefined" && Gateway
    readonly property bool _supported: root._have && Gateway.supported

    // Re-read the live status on show (there is no gateway node-event; poll on focus + on toggle).
    Component.onCompleted: if (root._have) Gateway.refresh()

    // Keep the address field in sync with the node's reported bind address until the operator edits
    // it (an in-progress edit must not be clobbered by a status refresh).
    Connections {
        target: root._have ? Gateway : null
        function onStatusChanged() {
            if (!addrField.activeFocus)
                addrField.text = Gateway.addr;
        }
    }

    SectionLabel { text: qsTr("OpenAI-compatible gateway") }

    Text {
        Layout.fillWidth: true
        text: qsTr("The node runs a resident OpenAI-compatible gateway so foreign agents (codex, opencode) can be routed through the node — keys stay on the node and never reach the agent.")
        color: Theme.textMuted
        font.family: FontIcons.display
        font.pixelSize: 12
        wrapMode: Text.WordWrap
    }

    // --- Unavailable fallback (no daemon / older node) --------------------------------------
    Text {
        visible: !root._supported
        Layout.fillWidth: true
        text: !root._have ? qsTr("Connect to a daemon to manage the gateway.")
                          : qsTr("This node does not provide the OpenAI-compatible gateway.")
        color: Theme.textMuted
        font.family: FontIcons.display
        font.pixelSize: 13
        wrapMode: Text.WordWrap
    }

    // --- Enable toggle ----------------------------------------------------------------------
    SettingRow {
        visible: root._supported
        Layout.fillWidth: true
        label: qsTr("Enable gateway")
        description: qsTr("Serve /v1/chat/completions and /v1/models from the node.")

        Kit.Switch {
            checked: root._have && Gateway.enabled
            onToggled: if (root._have) Gateway.setEnabled(checked, addrField.text)
        }
    }

    // --- Bind address -----------------------------------------------------------------------
    SettingRow {
        visible: root._supported
        Layout.fillWidth: true
        label: qsTr("Bind address")
        description: qsTr("Optional. Leave blank to let the node choose (e.g. a loopback port).")

        Kit.TextField {
            id: addrField
            implicitWidth: 220
            placeholderText: qsTr("127.0.0.1:0")
            text: root._have ? Gateway.addr : ""
            enabled: root._have && Gateway.enabled
            onEditingFinished: if (root._have && Gateway.enabled) Gateway.setEnabled(true, text)
        }
    }

    // --- Live status ------------------------------------------------------------------------
    RowLayout {
        visible: root._supported
        Layout.fillWidth: true
        spacing: 8

        Rectangle {
            width: 10; height: 10; radius: 5
            Layout.alignment: Qt.AlignVCenter
            color: !root._have || !Gateway.enabled ? Theme.textMuted
                 : Gateway.listening ? Theme.statusOk
                 : (Gateway.lastError.length > 0 ? Theme.danger : Theme.warning)
        }
        Text {
            Layout.fillWidth: true
            text: !root._have ? qsTr("unknown")
                : !Gateway.enabled ? qsTr("Disabled")
                : Gateway.listening ? (Gateway.addr.length > 0
                                       ? qsTr("Listening on %1").arg(Gateway.addr)
                                       : qsTr("Listening"))
                : (Gateway.lastError.length > 0 ? qsTr("Error: %1").arg(Gateway.lastError)
                                                : qsTr("Starting…"))
            color: Theme.textMuted
            font.family: FontIcons.mono
            font.pixelSize: 13
            wrapMode: Text.WordWrap
        }
    }

    // --- Node service health ----------------------------------------------------------------
    SectionLabel {
        visible: root._have && Status && Status.healthServices.length > 0
        text: qsTr("Service health")
    }

    Repeater {
        model: (root._have && Status) ? Status.healthServices : []
        delegate: RowLayout {
            required property var modelData
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                width: 8; height: 8; radius: 4
                Layout.alignment: Qt.AlignVCenter
                color: modelData.ok ? Theme.statusOk : Theme.danger
            }
            Text {
                text: modelData.name
                color: Theme.text
                font.family: FontIcons.display
                font.pixelSize: 13
            }
            Text {
                Layout.fillWidth: true
                text: modelData.detail && modelData.detail.length > 0
                      ? modelData.detail
                      : (modelData.ok ? qsTr("ok") : qsTr("down"))
                color: Theme.textMuted
                font.family: FontIcons.mono
                font.pixelSize: 12
                elide: Text.ElideRight
            }
        }
    }
}
