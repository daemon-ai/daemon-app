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
// Structure: a required Name field + the SHARED AgentTypePicker (native + ACP catalog with
// installed badges — the same component the first-run wizard mounts, so they cannot drift).
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

    // Mirrors of the shared picker's selection (read by the accept gate + commit).
    readonly property string engineAgent: enginePicker.engineAgent
    readonly property bool engineAgentInstalled: enginePicker.engineAgentInstalled
    readonly property bool nativeEngine: enginePicker.nativeEngine

    acceptEnabled: nameField.text.trim().length > 0
                   && (nativeEngine ? agentPicker.inferenceComplete
                                    : engineAgentInstalled)

    function openForm() {
        nameField.text = "";
        personaField.text = "";
        enginePicker.refresh(); // native default + fresh catalog/discovery badges each open
        nameField.forceActiveFocus();
        open();
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
        // A1 (CON-10): a custom endpoint pins genai's OpenAI-compatible adapter ("daemon_api")
        // at the user-supplied base URL with a typed model — there is no catalog descriptor.
        if (agentPicker.customEndpoint) {
            fields.provider = "daemon_api";
            fields.model = agentPicker.model;
            fields.baseUrl = agentPicker.customBaseUrl.trim();
        } else {
            var desc = (ProviderCatalog && agentPicker.providerId.length > 0)
                       ? ProviderCatalog.descriptorFor(agentPicker.providerId) : ({});
            // Persist the ProviderSelector wire string + base URL from the node descriptor
            // (never a hardcoded provider); the model is the picker's selection (never free
            // text).
            if (desc && desc.wireSelector)
                fields.provider = desc.wireSelector;
            if (agentPicker.model.length > 0)
                fields.model = agentPicker.model;
            if (desc && desc.defaultBaseUrl && desc.defaultBaseUrl.length > 0)
                fields.baseUrl = desc.defaultBaseUrl;
        }
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
        // The SHARED native-vs-foreign picker (also the wizard's agent-type step).
        AgentTypePicker {
            id: enginePicker
            Layout.fillWidth: true
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
