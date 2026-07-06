// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Foreign-agent management surface (C1 / ENG-2/ENG-3): the durable agent catalog (curated builtins
// + manual registrations, merged with the last discovery scan) with per-row source / protocol /
// installed state, a "Register custom…" form (AgentRegister), a "Re-scan installed" action
// (AgentDiscover), and Remove on manual rows (AgentRemove, with a fail-closed consequence confirm).
// Register/remove outcomes — including the node's honest failure cause (binary not found / handshake
// failed) — surface in the status banner (C6). Recipes never leave the node; rows are read-only.
ColumnLayout {
    id: root
    spacing: 14

    // Agents.agents() is a plain Q_INVOKABLE (not reactive) — re-read on every catalog change.
    property var rows: []
    // Last register/remove outcome for the status banner: {kind: "error"|"ok", text}.
    property var status: ({ kind: "", text: "" })

    function refreshRows() {
        root.rows = (typeof Agents !== "undefined" && Agents) ? Agents.agents() : [];
    }

    Component.onCompleted: {
        if (typeof Agents !== "undefined" && Agents) {
            Agents.refreshCatalog();
            Agents.discover();
        }
        root.refreshRows();
    }

    Connections {
        target: (typeof Agents !== "undefined" && Agents) ? Agents : null
        function onCatalogRefreshed() { root.refreshRows(); }
        function onAgentRegistered() {
            root.status = { "kind": "ok", "text": qsTr("Agent registered.") };
        }
        function onAgentRemoved(name) {
            root.status = { "kind": "ok", "text": qsTr("Removed “%1”.").arg(name) };
        }
        function onOperationFailed(message) {
            root.status = { "kind": "error", "text": message };
        }
    }

    SectionLabel { text: qsTr("Foreign agents") }
    Text {
        Layout.fillWidth: true
        text: qsTr("Agents that run a foreign engine (ACP or the Claude Code stream-json bridge). "
                   + "Launch recipes are stored and validated by the daemon — profiles reference an "
                   + "agent by name only.")
        color: Theme.textMuted
        font.family: FontIcons.display
        font.pixelSize: 12
        wrapMode: Text.WordWrap
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8
        Kit.TextButton {
            text: qsTr("Register custom…")
            accentFilled: true
            onClicked: registerDialog.openForm()
        }
        Kit.TextButton {
            text: qsTr("Re-scan installed")
            onClicked: {
                if (typeof Agents !== "undefined" && Agents)
                    Agents.discover();
            }
        }
        Item { Layout.fillWidth: true }
    }

    // Register/remove outcome (C6): the node's honest cause on failure; a confirmation on success.
    Rectangle {
        Layout.fillWidth: true
        visible: root.status.text.length > 0
        radius: Theme.radius
        color: Theme.codeBackground
        border.width: 1
        border.color: root.status.kind === "error" ? Theme.danger : Theme.border
        implicitHeight: statusText.implicitHeight + 16
        Text {
            id: statusText
            anchors.fill: parent
            anchors.margins: 8
            text: root.status.text
            color: root.status.kind === "error" ? Theme.danger : Theme.textMuted
            font.family: FontIcons.display
            font.pixelSize: 12
            wrapMode: Text.WordWrap
        }
    }

    // The catalog rows.
    Text {
        visible: root.rows.length === 0
        Layout.fillWidth: true
        text: qsTr("No foreign agents in the catalog yet.")
        color: Theme.textMuted
        font.family: FontIcons.display
        font.pixelSize: 12
    }

    Repeater {
        model: root.rows
        delegate: Rectangle {
            id: rowItem
            required property var modelData
            readonly property bool manual: modelData.source === "Manual"
            readonly property bool installed: modelData.installed === true
            Layout.fillWidth: true
            implicitHeight: 44
            radius: 8
            color: Theme.surface
            border.color: Theme.border

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 8
                spacing: 8

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1
                    Text {
                        text: rowItem.modelData.name
                        font.family: FontIcons.display; font.pixelSize: 13; color: Theme.text
                        elide: Text.ElideRight; Layout.fillWidth: true
                    }
                    Text {
                        text: (rowItem.modelData.protocol === "StreamJson" ? qsTr("stream-json")
                                                                           : qsTr("ACP"))
                              + (rowItem.modelData.version && rowItem.modelData.version.length > 0
                                 ? "  ·  " + rowItem.modelData.version : "")
                        font.family: FontIcons.mono; font.pixelSize: 10; color: Theme.textMuted
                    }
                }

                Kit.Chip {
                    text: rowItem.manual ? qsTr("manual") : qsTr("builtin")
                    tone: "muted"
                }
                // Installed badge (mirrors the engine picker).
                Rectangle {
                    radius: 4
                    color: "transparent"
                    border.width: 1
                    border.color: rowItem.installed ? Theme.stateRunning : Theme.border
                    implicitWidth: badge.implicitWidth + 12
                    implicitHeight: 18
                    Text {
                        id: badge
                        anchors.centerIn: parent
                        text: rowItem.installed ? qsTr("installed") : qsTr("not installed")
                        font.family: FontIcons.display; font.pixelSize: 10
                        color: rowItem.installed ? Theme.stateRunning : Theme.textMuted
                    }
                }
                // Remove is offered only for manual registrations (builtins are node-owned).
                Kit.IconButton {
                    visible: rowItem.manual
                    implicitWidth: 28
                    implicitHeight: 28
                    icon: FontIcons.fa_trash
                    iconColor: Theme.iconMuted
                    iconPointSize: 12
                    tooltipText: qsTr("Remove")
                    onClicked: removeConfirm.confirmFor(rowItem.modelData.name)
                }
            }
        }
    }

    RegisterAgentDialog { id: registerDialog }

    // Remove confirm: state the fail-closed consequence before AgentRemove commits (ENG-3).
    Kit.Dialog {
        id: removeConfirm
        title: qsTr("Remove agent")
        acceptText: qsTr("Remove")
        destructive: true
        property string agentName: ""
        function confirmFor(name) { removeConfirm.agentName = name; open(); }
        onAccepted: {
            if (typeof Agents !== "undefined" && Agents)
                Agents.removeAgent(removeConfirm.agentName);
        }
        contentItem: Text {
            text: qsTr("Remove “%1”? Profiles that use it will fail to start until it is "
                       + "re-registered.").arg(removeConfirm.agentName)
            color: Theme.text
            font.family: FontIcons.display
            font.pixelSize: 13
            wrapMode: Text.WordWrap
        }
    }
}
