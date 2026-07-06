// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme

// The SHARED native-vs-foreign engine picker (ENG-2 / CON-16): one list — "daemon-core" (the
// native engine, row 0, default) plus every foreign catalog entry with installed badges —
// extracted from NewAgentDialog so the "+ New agent" dialog and the first-run wizard render ONE
// component that cannot drift. Selection-only presentation: rows come from the Agents facade
// (AgentRepository: the durable catalog merged with the last discovery scan); an uninstalled
// foreign agent stays visible but unselectable (the badge explains why).
//
// refresh() re-reads the catalog AND kicks a fresh node-side discovery scan (AgentDiscover), so
// just-installed agents show live installed badges. Absent facade (mock mode / harnesses)
// => native only.
ColumnLayout {
    id: picker
    spacing: 6

    // The selected engine: "" = daemon-core (native); otherwise the foreign catalog agent name.
    property string engineAgent: ""
    // Whether the selected foreign agent is installed (the consumer's accept-gate half).
    property bool engineAgentInstalled: false
    // The foreign catalog rows ({name, source, protocol, installed, version}); re-read on
    // catalogRefreshed — Agents.agents() is a plain Q_INVOKABLE, not a reactive binding.
    property var agentRows: []

    readonly property bool nativeEngine: engineAgent.length === 0

    // Fired on every selection change (tap or revalidation fallback).
    signal selected(string agent, bool installed)

    function refreshRows() {
        picker.agentRows = (typeof Agents !== "undefined" && Agents) ? Agents.agents()
                                                                     : [];
        if (!picker.nativeEngine) {
            // Re-validate the current selection against the fresh catalog (it may have vanished
            // or flipped installed-ness).
            var still = false;
            for (var i = 0; i < picker.agentRows.length; ++i)
                if (picker.agentRows[i].name === picker.engineAgent) {
                    still = true;
                    picker.engineAgentInstalled = picker.agentRows[i].installed === true;
                }
            if (!still)
                picker.selectEngine("", false);
        }
    }

    function selectEngine(agent, installed) {
        picker.engineAgent = agent;
        picker.engineAgentInstalled = installed === true;
        picker.selected(picker.engineAgent, picker.engineAgentInstalled);
    }

    // Fresh catalog + a node-side discovery scan; call when the hosting surface opens.
    function refresh() {
        picker.selectEngine("", false);
        if (typeof Agents !== "undefined" && Agents) {
            Agents.refreshCatalog();
            Agents.discover(); // fresh installed badges beside the static catalog
        }
        picker.refreshRows();
    }

    Connections {
        target: (typeof Agents !== "undefined" && Agents) ? Agents : null
        function onCatalogRefreshed() {
            picker.refreshRows();
        }
    }

    ListView {
        id: engineList
        Layout.fillWidth: true
        Layout.preferredHeight: Math.min(contentHeight, 132)
        clip: true
        // Row 0 is always the native engine; catalog rows follow.
        model: 1 + picker.agentRows.length
        delegate: Rectangle {
            id: engineRow
            required property int index
            readonly property bool isNative: index === 0
            readonly property var agent: isNative ? ({}) : picker.agentRows[index - 1]
            readonly property bool isInstalled: !isNative && agent.installed === true
            readonly property bool selected: isNative ? picker.nativeEngine
                                                      : picker.engineAgent === agent.name
            width: ListView.view ? ListView.view.width : 0
            height: 30
            radius: 6
            color: engineRow.selected ? Theme.hover : "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 8
                Text {
                    // Foreign rows suffix the protocol: "· ACP <version>" (version when the probe
                    // reported one) or "· stream-json" for the Claude-Code bridge.
                    text: engineRow.isNative
                          ? qsTr("daemon-core (native)")
                          : engineRow.agent.name
                            + (engineRow.agent.protocol === "StreamJson"
                               ? qsTr("  ·  stream-json")
                               : engineRow.agent.version && engineRow.agent.version.length > 0
                                 ? qsTr("  ·  ACP %1").arg(engineRow.agent.version) : "")
                    font.family: FontIcons.display
                    font.pixelSize: 12
                    color: (engineRow.isNative || engineRow.isInstalled) ? Theme.text
                                                                         : Theme.textMuted
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                // Installed badge for catalog rows (the native engine is always available).
                Rectangle {
                    visible: !engineRow.isNative
                    radius: 4
                    color: "transparent"
                    border.width: 1
                    border.color: engineRow.isInstalled ? Theme.stateRunning : Theme.border
                    implicitWidth: badgeText.implicitWidth + 12
                    implicitHeight: 18
                    Text {
                        id: badgeText
                        anchors.centerIn: parent
                        text: engineRow.isInstalled ? qsTr("installed")
                                                    : qsTr("not installed")
                        font.family: FontIcons.display
                        font.pixelSize: 10
                        color: engineRow.isInstalled ? Theme.stateRunning : Theme.textMuted
                    }
                }
            }
            TapHandler {
                onTapped: {
                    // An uninstalled foreign agent stays visible but unselectable (the badge
                    // explains why); selecting row 0 returns to the native engine.
                    if (engineRow.isNative)
                        picker.selectEngine("", false);
                    else if (engineRow.isInstalled)
                        picker.selectEngine(engineRow.agent.name, true);
                }
            }
        }
    }
}
