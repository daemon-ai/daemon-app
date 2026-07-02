// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Pages

// "+ New agent" (A7 + foreign engines): the Daemon-Kit-themed create-agent dialog, extracted from
// Sidebar.qml so the sidebar only instantiates it.
//
// Structure: a required Name field + an ENGINE picker listing "daemon-core" (the native engine,
// default) plus every ACP catalog entry (AcpAgents / wire v23) with installed badges.
//   - daemon-core: embeds the SHARED node-driven AgentInferencePicker (provider -> key -> model;
//     a FROZEN component) + optional persona, and commits through the SAME node-authoritative
//     path as before: ProfileCreate -> configure provider/model/base-url (+ key + persona) ->
//     setDefault -> a blank node SessionCreate under it (App.openNewAgentChat).
//   - a foreign ACP agent: the inference picker is hidden (no provider/model/key — the launch
//     recipe is fixed by the node's catalog, referenced BY NAME ONLY), and accept issues the named
//     ProfileCreate carrying engine=Acp{agent} (Profiles.createAcpProfile).
// Accept is gated: native -> name + inferenceComplete; foreign -> name + an INSTALLED agent.
Kit.Dialog {
    id: root
    title: qsTr("New agent")
    width: 440
    acceptText: qsTr("Create")
    closePolicy: QQC.Popup.CloseOnEscape

    // The selected engine: "" = daemon-core (native); otherwise the ACP catalog agent name.
    property string engineAgent: ""
    // Whether the selected foreign agent is installed (accept-gate half for the foreign path).
    property bool engineAgentInstalled: false
    // The ACP catalog rows ({name, source, installed, version}); re-read on catalogRefreshed —
    // AcpAgents.agents() is a plain Q_INVOKABLE, not a reactive binding (mirrors the picker's
    // ProviderCatalog handling). Absent facade (mock mode / harnesses) => native only.
    property var acpRows: []

    readonly property bool nativeEngine: engineAgent.length === 0
    acceptEnabled: nameField.text.trim().length > 0
                   && (nativeEngine ? agentPicker.inferenceComplete
                                    : engineAgentInstalled)

    function refreshAcpRows() {
        root.acpRows = (typeof AcpAgents !== "undefined" && AcpAgents) ? AcpAgents.agents() : [];
        if (!root.nativeEngine) {
            // Re-validate the current selection against the fresh catalog (it may have vanished
            // or flipped installed-ness).
            var still = false;
            for (var i = 0; i < root.acpRows.length; ++i)
                if (root.acpRows[i].name === root.engineAgent) {
                    still = true;
                    root.engineAgentInstalled = root.acpRows[i].installed === true;
                }
            if (!still)
                root.selectEngine("", false);
        }
    }

    function selectEngine(agent, installed) {
        root.engineAgent = agent;
        root.engineAgentInstalled = installed === true;
    }

    function openForm() {
        nameField.text = "";
        personaField.text = "";
        root.selectEngine("", false);
        if (typeof AcpAgents !== "undefined" && AcpAgents)
            AcpAgents.refreshCatalog(); // fresh installed badges each open
        root.refreshAcpRows();
        nameField.forceActiveFocus();
        open();
    }

    Connections {
        target: (typeof AcpAgents !== "undefined" && AcpAgents) ? AcpAgents : null
        function onCatalogRefreshed() {
            root.refreshAcpRows();
        }
    }

    onAccepted: {
        var name = nameField.text.trim();
        if (name.length === 0)
            return;
        if (!root.nativeEngine) {
            // Foreign engine: the named ProfileCreate carries engine=Acp{agent} — a catalog NAME,
            // never a recipe; no provider/model/key applies. Same activate + open flow as native.
            if (!root.engineAgentInstalled)
                return;
            var acpId = Profiles.createAcpProfile(name, root.engineAgent);
            if (!acpId || acpId.length === 0)
                return;
            Profiles.setDefault(acpId);
            if (typeof App !== "undefined" && App)
                App.openNewAgentChat();
            return;
        }
        if (!agentPicker.inferenceComplete)
            return;
        var fields = {};
        var desc = (ProviderCatalog && agentPicker.providerId.length > 0)
                   ? ProviderCatalog.descriptorFor(agentPicker.providerId) : ({});
        // Persist the ProviderSelector wire string + base URL from the node descriptor (never a
        // hardcoded provider); the model is the picker's selection (never free text).
        if (desc && desc.wireSelector)
            fields.provider = desc.wireSelector;
        if (agentPicker.model.length > 0)
            fields.model = agentPicker.model;
        if (desc && desc.defaultBaseUrl && desc.defaultBaseUrl.length > 0)
            fields.baseUrl = desc.defaultBaseUrl;
        if (personaField.text.trim().length > 0)
            fields.systemPrompt = personaField.text.trim();
        // ONE full-spec ProfileCreate (same seam as the wizard): the node dispatches pipelined
        // Calls concurrently, so a create + follow-up update could race node-side.
        var id = Profiles.createProfileWithSpec(name, fields);
        if (!id || id.length === 0)
            return;
        // Profile-scoped credential for a key-requiring provider (stored under the new agent).
        if (agentPicker.key.length > 0 && typeof Accounts !== "undefined" && Accounts)
            Accounts.addApiKeyForProfile(id, agentPicker.providerId, "", agentPicker.key, "");
        // Same create path as the wizard: make the new agent active, then a blank node
        // SessionCreate under it opens + auto-selects a transcript (event-driven, node-minted id).
        Profiles.setDefault(id);
        if (typeof App !== "undefined" && App)
            App.openNewAgentChat();
    }

    contentItem: ColumnLayout {
        spacing: 10

        SectionLabel { text: qsTr("Name") }
        Kit.TextField {
            id: nameField
            Layout.fillWidth: true
            placeholderText: qsTr("agent name")
        }

        SectionLabel { text: qsTr("Engine") }
        ListView {
            id: engineList
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 132)
            clip: true
            // Row 0 is always the native engine; catalog rows follow.
            model: 1 + root.acpRows.length
            delegate: Rectangle {
                id: engineRow
                required property int index
                readonly property bool isNative: index === 0
                readonly property var acp: isNative ? ({}) : root.acpRows[index - 1]
                readonly property bool isInstalled: !isNative && acp.installed === true
                readonly property bool selected: isNative ? root.nativeEngine
                                                          : root.engineAgent === acp.name
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
                                   ? "  ·  ACP " + engineRow.acp.version : "")
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
                            root.selectEngine("", false);
                        else if (engineRow.isInstalled)
                            root.selectEngine(engineRow.acp.name, true);
                    }
                }
            }
        }

        // Native engine: the SHARED node-driven provider/model/key picker (A7; FROZEN component)
        // + optional persona. Hidden entirely for a foreign engine.
        AgentInferencePicker {
            id: agentPicker
            visible: root.nativeEngine
            Layout.fillWidth: true
        }
        SectionLabel { visible: root.nativeEngine; text: qsTr("Persona (optional)") }
        Kit.TextField {
            id: personaField
            visible: root.nativeEngine
            Layout.fillWidth: true
            placeholderText: qsTr("system prompt")
        }

        // Foreign engine: no inference to configure — the launch recipe is fixed by the node's
        // ACP catalog and the profile references it by name only.
        Text {
            visible: !root.nativeEngine
            text: qsTr("This agent runs a foreign ACP engine. Its launch recipe is managed by "
                       + "the daemon's ACP catalog — no provider, model, or key to configure.")
            font.family: FontIcons.display
            font.pixelSize: 12
            color: Theme.textMuted
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
