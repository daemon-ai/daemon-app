// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// The SHARED agent-setup form (Phase D), rendered over the ONE shared AgentSetupModel: the single
// `engine + backend + inference` create pipeline surfaced identically wherever the operator makes
// a new agent (the "+ New agent" dialog and the Profiles "+ New" path). It composes the shared
// step components — AgentTypePicker (engine, with the node-derived verification badge),
// ForeignBackendPicker (backend, only when `needsBackendChoice`), and AgentInferencePicker
// (inference, only when `needsInference` — i.e. Core or a NodeProvider foreign backend) — so a
// Core profile and a foreign profile flow through one form instead of bespoke per-surface layouts.
//
// The form is selection-only: every field drives an AgentSetup intent, and submit() commits
// through the model (AgentSetup.commit -> committed(profileId)/failed(message), re-emitted here).
// Native persona is an optional extra applied on commit (the model's commit payload is
// provider/model/backend; the system prompt has no ProfileSpec home in that path).
ColumnLayout {
    id: form
    spacing: 12

    readonly property bool _hasSetup: (typeof AgentSetup !== "undefined" && AgentSetup)

    // The chosen agent name (the profile id the node keys it by) + an optional native persona.
    property alias nameText: nameField.text
    property alias personaText: personaField.text
    // Whether to render the Name field (the wizard supplies its own name row; a create dialog uses
    // this one).
    property bool showName: true

    // Commit is enabled once the model says the selection is complete AND a name was entered.
    readonly property bool commitReady: form._hasSetup && AgentSetup.commitReady
                                        && nameField.text.trim().length > 0

    // Re-emitted AgentSetup commit outcomes so the hosting surface can open the chat / show a
    // failure without reaching into the model itself.
    signal committed(string profileId)
    signal failed(string message)

    // The shared AgentSetupModel is a singleton the wizard + every create surface bind, so its
    // committed()/failed() reach ALL live forms. Only the form that submitted may react to the
    // outcome (otherwise an idle Profiles-page form would fire when the New-agent dialog commits).
    property bool _committing: false

    // Reset to a fresh Core setup + clear the local fields (call when the hosting surface opens).
    function resetForm() {
        form._committing = false;
        if (form._hasSetup)
            AgentSetup.reset();
        nameField.text = "";
        personaField.text = "";
        enginePicker.refresh(); // fresh catalog + discovery/verification badges
    }

    // Focus the Name field (the hosting create dialog opens straight onto it).
    function focusName() { nameField.forceActiveFocus(); }

    // Commit the current selection through the shared model. A native persona is applied on the
    // committed profile (the model's commit payload carries provider/model/backend only).
    function submit() {
        if (!form._hasSetup)
            return;
        form._committing = true;
        AgentSetup.commit(nameField.text.trim());
    }

    Connections {
        target: form._hasSetup ? AgentSetup : null
        function onCommitted(profileId) {
            if (!form._committing)
                return; // another surface's commit — not ours
            form._committing = false;
            const persona = personaField.text.trim();
            if (persona.length > 0 && profileId.length > 0 && AgentSetup.engineKind === "Core"
                && typeof Profiles !== "undefined" && Profiles)
                Profiles.updateProfile(profileId, { "systemPrompt": persona });
            form.committed(profileId);
        }
        function onFailed(message) {
            if (!form._committing)
                return;
            form._committing = false;
            form.failed(message);
        }
    }

    // --- Name ---------------------------------------------------------------------------------
    SectionLabel { visible: form.showName; text: qsTr("Name") }
    Kit.TextField {
        id: nameField
        visible: form.showName
        Layout.fillWidth: true
        placeholderText: qsTr("agent name")
    }

    // --- Engine -------------------------------------------------------------------------------
    SectionLabel { text: qsTr("Engine") }
    // Edit mode locks the engine (an existing profile's engine cannot change): render it as a
    // read-only identity chip instead of the picker.
    Kit.Chip {
        visible: form._hasSetup && AgentSetup.engineLocked
        text: AgentSetup.engineKind === "Foreign"
              ? (AgentSetup.foreignAgent.length > 0 ? AgentSetup.foreignAgent : qsTr("Foreign"))
              : qsTr("Native")
        iconGlyph: AgentSetup.engineKind === "Foreign" ? FontIcons.fa_robot
                                                       : FontIcons.fa_microchip
        tone: AgentSetup.engineKind === "Foreign" ? "accent" : "muted"
    }
    AgentTypePicker {
        id: enginePicker
        visible: !(form._hasSetup && AgentSetup.engineLocked)
        Layout.fillWidth: true
    }

    // --- Backend (foreign only) ---------------------------------------------------------------
    SectionLabel {
        visible: form._hasSetup && AgentSetup.needsBackendChoice
        text: qsTr("Backend")
    }
    ForeignBackendPicker {
        visible: form._hasSetup && AgentSetup.needsBackendChoice
        Layout.fillWidth: true
    }

    // --- Inference (Core OR NodeProvider) -----------------------------------------------------
    AgentInferencePicker {
        id: inferencePicker
        visible: form._hasSetup && AgentSetup.needsInference
        Layout.fillWidth: true
    }

    // --- Persona (native only, optional) ------------------------------------------------------
    SectionLabel {
        visible: form._hasSetup && AgentSetup.engineKind === "Core"
        text: qsTr("Persona (optional)")
    }
    Kit.TextField {
        id: personaField
        visible: form._hasSetup && AgentSetup.engineKind === "Core"
        Layout.fillWidth: true
        placeholderText: qsTr("system prompt")
    }
}
