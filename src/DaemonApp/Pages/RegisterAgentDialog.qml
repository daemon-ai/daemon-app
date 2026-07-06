// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Register a custom foreign agent (C1 / ENG-3): a small form — Name, Protocol (ACP vs the
// Claude-Code stream-json bridge), and a launch recipe that is EITHER a stdio program
// (program + args + env) OR a tcp:// endpoint. Submit issues AgentRegister via the Agents facade;
// the NODE re-probes and never trusts a caller-supplied installed flag (ENG-3), so the outcome
// (handshake ✓ / binary not found / handshake failed) arrives on agentRegistered / operationFailed.
// The hosting section renders that outcome (C6). Recipes never travel back out — this is send-only.
Kit.Dialog {
    id: root
    title: qsTr("Register foreign agent")
    width: 460
    acceptText: qsTr("Register")
    closePolicy: QQC.Popup.CloseOnEscape

    readonly property bool useEndpoint: transportSwitch.checked
    readonly property bool recipeReady: useEndpoint ? endpointField.text.trim().length > 0
                                                    : programField.text.trim().length > 0
    acceptEnabled: nameField.text.trim().length > 0 && recipeReady

    function openForm() {
        nameField.text = "";
        protocolPicker.currentIndex = 0;
        transportSwitch.checked = false;
        programField.text = "";
        argsField.text = "";
        envArea.text = "";
        endpointField.text = "";
        nameField.forceActiveFocus();
        open();
    }

    // KEY=VALUE lines -> a { KEY: VALUE } map for the env recipe (blank/comment lines skipped).
    function parseEnv(text) {
        var env = ({});
        var lines = text.split("\n");
        for (var i = 0; i < lines.length; ++i) {
            var line = lines[i].trim();
            if (line.length === 0 || line.indexOf("=") < 0 || line.charAt(0) === "#")
                continue;
            var eq = line.indexOf("=");
            env[line.substring(0, eq).trim()] = line.substring(eq + 1).trim();
        }
        return env;
    }

    onAccepted: {
        if (typeof Agents === "undefined" || !Agents)
            return;
        var form = {
            "name": nameField.text.trim(),
            "protocol": protocolPicker.currentIndex === 1 ? "StreamJson" : "Acp"
        };
        if (root.useEndpoint) {
            form.endpoint = endpointField.text.trim();
        } else {
            form.program = programField.text.trim();
            var a = argsField.text.trim();
            form.args = a.length > 0 ? a.split(/\s+/) : [];
            form.env = root.parseEnv(envArea.text);
        }
        Agents.registerAgent(form);
    }

    contentItem: ColumnLayout {
        spacing: 10

        SectionLabel { text: qsTr("Name") }
        Kit.TextField {
            id: nameField
            Layout.fillWidth: true
            placeholderText: qsTr("catalog name (e.g. my-claude)")
        }

        SectionLabel { text: qsTr("Protocol") }
        Kit.Dropdown {
            id: protocolPicker
            Layout.fillWidth: true
            model: [qsTr("ACP (Agent Client Protocol)"), qsTr("Claude Code (stream-json)")]
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Kit.Switch { id: transportSwitch }
            Text {
                text: qsTr("Connect to a TCP endpoint instead of launching a program")
                font.family: FontIcons.display
                font.pixelSize: 12
                color: Theme.textMuted
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        // Stdio recipe: program + args + env.
        SectionLabel { visible: !root.useEndpoint; text: qsTr("Program") }
        Kit.TextField {
            id: programField
            visible: !root.useEndpoint
            Layout.fillWidth: true
            placeholderText: qsTr("executable (resolved on the daemon's PATH)")
        }
        SectionLabel { visible: !root.useEndpoint; text: qsTr("Arguments (space-separated)") }
        Kit.TextField {
            id: argsField
            visible: !root.useEndpoint
            Layout.fillWidth: true
            placeholderText: qsTr("--flag value")
        }
        SectionLabel { visible: !root.useEndpoint; text: qsTr("Environment (KEY=VALUE per line)") }
        Kit.TextArea {
            id: envArea
            visible: !root.useEndpoint
            Layout.fillWidth: true
            Layout.preferredHeight: 66
        }

        // Endpoint recipe.
        SectionLabel { visible: root.useEndpoint; text: qsTr("Endpoint") }
        Kit.TextField {
            id: endpointField
            visible: root.useEndpoint
            Layout.fillWidth: true
            placeholderText: qsTr("tcp://host:port")
        }

        Text {
            Layout.fillWidth: true
            text: qsTr("The daemon probes the recipe on Register (an ACP initialize handshake, or a "
                       + "PATH check for stream-json) — the result appears in the agent list.")
            font.family: FontIcons.display
            font.pixelSize: 11
            color: Theme.textMuted
            wrapMode: Text.WordWrap
        }
    }
}
