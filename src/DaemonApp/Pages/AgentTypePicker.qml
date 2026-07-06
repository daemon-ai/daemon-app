// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme

// The SHARED native-vs-foreign engine picker (ENG-2 / CON-16): one list — "daemon-core" (the
// native engine, row 0, default) plus every ACP catalog entry with installed badges — extracted
// from NewAgentDialog so the "+ New agent" dialog and the first-run wizard render ONE component
// that cannot drift. Selection-only presentation: rows come from the AcpAgents facade
// (AcpRepository: the durable catalog merged with the last discovery scan); an uninstalled
// foreign agent stays visible but unselectable (the badge explains why).
//
// refresh() re-reads the catalog AND kicks a fresh node-side discovery scan (AcpDiscover), so
// just-installed agents show live installed badges. Absent facade (mock mode / harnesses)
// => native only.
ColumnLayout {
    id: picker
    spacing: 6

    // The selected engine: "" = daemon-core (native); otherwise the ACP catalog agent name.
    property string engineAgent: ""
    // Whether the selected foreign agent is installed (the consumer's accept-gate half).
    property bool engineAgentInstalled: false
    // The ACP catalog rows ({name, source, installed, version}); re-read on catalogRefreshed —
    // AcpAgents.agents() is a plain Q_INVOKABLE, not a reactive binding.
    property var acpRows: []

    readonly property bool nativeEngine: engineAgent.length === 0

    // Fired on every selection change (tap or revalidation fallback).
    signal selected(string agent, bool installed)

    function refreshAcpRows() {
        picker.acpRows = (typeof AcpAgents !== "undefined" && AcpAgents) ? AcpAgents.agents()
                                                                         : [];
        if (!picker.nativeEngine) {
            // Re-validate the current selection against the fresh catalog (it may have vanished
            // or flipped installed-ness).
            var still = false;
            for (var i = 0; i < picker.acpRows.length; ++i)
                if (picker.acpRows[i].name === picker.engineAgent) {
                    still = true;
                    picker.engineAgentInstalled = picker.acpRows[i].installed === true;
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
        if (typeof AcpAgents !== "undefined" && AcpAgents) {
            AcpAgents.refreshCatalog();
            AcpAgents.discover(); // fresh installed badges beside the static catalog
        }
        picker.refreshAcpRows();
    }

    Connections {
        target: (typeof AcpAgents !== "undefined" && AcpAgents) ? AcpAgents : null
        function onCatalogRefreshed() {
            picker.refreshAcpRows();
        }
    }

    ListView {
        id: engineList
        Layout.fillWidth: true
        Layout.preferredHeight: Math.min(contentHeight, 132)
        clip: true
        // Row 0 is always the native engine; catalog rows follow.
        model: 1 + picker.acpRows.length
        delegate: Rectangle {
            id: engineRow
            required property int index
            readonly property bool isNative: index === 0
            readonly property var acp: isNative ? ({}) : picker.acpRows[index - 1]
            readonly property bool isInstalled: !isNative && acp.installed === true
            readonly property bool selected: isNative ? picker.nativeEngine
                                                      : picker.engineAgent === acp.name
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
                    text: engineRow.isNative
                          ? qsTr("daemon-core (native)")
                          : engineRow.acp.name
                            + (engineRow.acp.version && engineRow.acp.version.length > 0
                               ? qsTr("  ·  ACP %1").arg(engineRow.acp.version) : "")
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
                        picker.selectEngine(engineRow.acp.name, true);
                }
            }
        }
    }
}
