// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// A7: the SHARED, node-driven provider/model/key picker. Extracted from the first-run inference
// step so the SAME picker backs BOTH the onboarding wizard AND the "+ New agent" form (a
// hand-rolled dialog with a hardcoded provider string + free-text model is not acceptable).
//
// Invariants (daemon-supervision-spec §0; node-authoritative):
//   - the provider list comes from the node's ProviderCatalog (real vendor names) — NEVER a
//     hardcoded/`genai` string;
//   - the model is SELECTED from the node's ProviderModels (per-provider offered models) — NEVER a
//     free-text string;
//   - Daemon Kit theming throughout.
//
// Consumers read `providerId` / `model` / `key` and gate their commit on `inferenceComplete`
// (+ AgentSetup.keyGatePassed for a key-required vendor). `descriptorFor(providerId)` on
// ProviderCatalog yields the ProviderSelector + base URL the persisted profile needs.
//
// Phase D: the picker is the shared inference sub-form for the ONE setup pipeline, so it reports
// its live selection to the shared AgentSetupModel — both the full provider/model/key/baseUrl
// selection (AgentSetup.setInference, what commit persists) and the key-gate re-arm
// (AgentSetup.setInferenceSelection) — and renders the model's keyGateMessage. The first-run
// wizard, the "+ New agent" form, and the profiles "+ New" path all bind one gate + commit path.
ColumnLayout {
    id: picker
    spacing: 10

    // The ProviderCatalog descriptor id (e.g. "anthropic"/"openai"/"daemon_cloud"/"llama_cpp"),
    // or the synthetic kCustomId for the self-hosted endpoint row (A1 / CON-10).
    property string providerId: ""
    // The selection-only model id the profile persists (chosen from ProviderModels; free text
    // for the custom endpoint, whose models no wire op can enumerate).
    property string model: ""
    // The entered API key (transient; a key-requiring vendor authenticates its LIST with it).
    property alias key: keyField.text
    // Whether the API key field is unmasked (masked by default; the eye toggle flips it).
    property bool revealKey: false
    // Prefer Daemon Cloud as the initial provider (keyless, works out of the box).
    property bool preferDaemonCloud: true

    // A1 (CON-10): the synthetic "Custom endpoint…" row id + its state. The row is client-side
    // only (never a catalog descriptor): the profile persists provider="daemon_api" (genai's
    // OpenAI-compatible adapter pinned at the base URL) + the user base URL + typed model.
    readonly property string kCustomId: "__custom__"
    readonly property bool customEndpoint: providerId === kCustomId
    property alias customBaseUrl: baseUrlField.text

    // NOT a reactive binding: ProviderCatalog.providers() is a plain Q_INVOKABLE and the node's
    // catalog arrives asynchronously, so it is re-read explicitly on providersChanged. The
    // custom-endpoint row is appended client-side after the node rows.
    property var providerRows: picker._composeRows()
    readonly property var providerDescriptor: (ProviderCatalog && providerId.length > 0
                                               && !customEndpoint)
                                              ? ProviderCatalog.descriptorFor(providerId) : ({})
    readonly property bool providerRequiresKey: providerDescriptor
                                                && providerDescriptor.requiresKey === true
    // A provider + a concrete model are chosen, and any required key is at least entered. The
    // custom endpoint needs its base URL + a typed model (the key stays optional there).
    readonly property bool inferenceComplete: customEndpoint
                                              ? (baseUrlField.text.trim().length > 0
                                                 && model.length > 0)
                                              : (providerId.length > 0 && model.length > 0
                                                 && (!providerRequiresKey
                                                     || keyField.text.length > 0))

    function _composeRows() {
        var rows = ProviderCatalog ? ProviderCatalog.providers() : [];
        // Vision (CON-10): a "Custom endpoint…" row at the bottom of the provider list.
        return rows.concat([{ id: picker.kCustomId, name: qsTr("Custom endpoint…"),
                              kind: "custom", requiresKey: false }]);
    }

    readonly property bool _hasSetup: (typeof AgentSetup !== "undefined" && AgentSetup)

    // Report the live provider/key selection to the shared key gate (re-arms it: a proven key
    // must re-prove via the next authenticated LIST after any change).
    function _reportSelection() {
        if (picker._hasSetup)
            AgentSetup.setInferenceSelection(picker.providerId, keyField.text);
        picker._pushInference();
    }

    // Push the full inference selection into the shared model (what commit persists): provider,
    // selected model, transient key, and — on the custom-endpoint path — the user base URL.
    function _pushInference() {
        if (picker._hasSetup)
            AgentSetup.setInference(picker.providerId, picker.model, keyField.text,
                                    picker.customEndpoint ? baseUrlField.text.trim() : "");
    }

    function _seedProvider() {
        if (providerId.length > 0 || providerRows.length === 0)
            return;
        // Editor hydration / re-open: prefer a selection the shared model already holds
        // (AgentSetup.providerId/model) over the fresh default, so re-editing a NodeProvider
        // foreign profile shows its persisted provider + model.
        if (picker._hasSetup && AgentSetup.providerId && AgentSetup.providerId.length > 0) {
            var hydrateModel = AgentSetup.model ? AgentSetup.model : "";
            picker.selectProvider(AgentSetup.providerId);
            if (hydrateModel.length > 0) {
                picker.model = hydrateModel;
                picker._pushInference();
            }
            return;
        }
        // Seed the preferred default: Daemon Cloud (keyless listing) or the first LOCAL engine
        // (llama.cpp — first-run wants zero-key local inference). Falls back to row 0.
        var wantKind = preferDaemonCloud ? "daemon_cloud" : "local";
        var pick = providerRows[0];
        for (var i = 0; i < providerRows.length; ++i)
            if (providerRows[i].id === wantKind || providerRows[i].kind === wantKind) {
                pick = providerRows[i];
                break;
            }
        picker.selectProvider(pick.id);
    }
    function selectProvider(id) {
        picker.providerId = id;
        picker.model = "";
        picker.revealKey = false;
        picker._reportSelection(); // re-arms the key gate + pushes the (model-less) selection
        // The custom endpoint has nothing to list: no wire op can enumerate an arbitrary
        // OpenAI-compatible server's models (the model is typed instead).
        if (ProviderCatalog && id !== picker.kCustomId)
            ProviderCatalog.refreshModels(id, "", keyField.text); // transient key (no profile yet)
    }
    // The catalog kind ("local" | "remote") of the SELECTED provider's row, "" before seeding.
    function currentProviderKind() {
        for (var i = 0; i < providerRows.length; ++i)
            if (providerRows[i].id === providerId)
                return providerRows[i].kind;
        return "";
    }
    // Keep the visible dropdown in lockstep with the SELECTED provider for every programmatic
    // change (seeding, external writes): the key field / model rows follow providerId, so a
    // desynced ComboBox shows one provider while the form drives another.
    onProviderIdChanged: {
        providerBox.syncFromModel();
        modelList.refreshRows();
    }

    Component.onCompleted: {
        picker.providerRows = picker._composeRows();
        picker._seedProvider();
        providerBox.syncFromModel();
    }
    Connections {
        target: ProviderCatalog
        function onProvidersChanged() {
            picker.providerRows = picker._composeRows();
            picker._seedProvider();
            // The rebuilt dropdown model reset currentIndex; re-point it at the selection.
            providerBox.syncFromModel();
        }
    }

    // [waveB:app-v30] CON-15: the shared generic sign-in sheet. Opened for the selected provider's
    // node-advertised sign_in.family via the existing AuthBegin path (Matrix SSO + generic oauth2
    // already work); the family + label come entirely off the wire — zero vendor strings here.
    AuthFlowSheet { id: signInSheet }

    SectionLabel { text: qsTr("Provider") }
    Kit.Dropdown {
        id: providerBox
        Layout.fillWidth: true
        model: picker.providerRows.map(function(p) { return p.name; })
        function syncFromModel() {
            for (var i = 0; i < picker.providerRows.length; ++i)
                if (picker.providerRows[i].id === picker.providerId) { currentIndex = i; return; }
        }
        onActivated: picker.selectProvider(picker.providerRows[currentIndex].id)
    }

    // [waveB:app-v30] CON-15: a sign-in affordance shown IFF the node's catalog row for the
    // selected provider carries a sign_in. Label = node-supplied; action = open the generic
    // AuthFlowSheet driving auth_begin{family: sign_in.family}. Hidden when the node advertises
    // nothing.
    Kit.TextButton {
        readonly property string signInFamily:
            picker.providerDescriptor && picker.providerDescriptor.signInFamily
            ? String(picker.providerDescriptor.signInFamily) : ""
        readonly property string signInLabel:
            picker.providerDescriptor && picker.providerDescriptor.signInLabel
            ? String(picker.providerDescriptor.signInLabel) : ""
        visible: signInFamily.length > 0
        Layout.alignment: Qt.AlignLeft
        text: signInLabel.length > 0 ? signInLabel : qsTr("Sign in")
        onClicked: signInSheet.openFlowForFamily(signInFamily, "")
    }

    // A1 (CON-10): the custom endpoint's base URL + typed model. No wire op can enumerate a
    // self-hosted server's models or probe it pre-commit, so the fields are honest about that:
    // the first message verifies the endpoint, and a failure deep-links back here (CON-8).
    ColumnLayout {
        visible: picker.customEndpoint
        Layout.fillWidth: true
        spacing: 8
        SectionLabel { text: qsTr("Base URL") }
        Kit.TextField {
            id: baseUrlField
            Layout.fillWidth: true
            placeholderText: qsTr("Base URL (e.g. https://…)")
            onTextEdited: picker._pushInference() // the custom-endpoint base URL rides the commit
        }
        SectionLabel { text: qsTr("Model id") }
        Kit.TextField {
            id: customModelField
            Layout.fillWidth: true
            placeholderText: qsTr("model id (as your server names it)")
            // The typed model IS the selection on the custom path (nothing to list).
            onTextEdited: { picker.model = text.trim(); picker._pushInference(); }
        }
        Text {
            text: qsTr("The endpoint is used as-is — your first message verifies it, and a "
                       + "failure will guide you back here.")
            font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
            Layout.fillWidth: true; wrapMode: Text.WordWrap
        }
    }

    // API key: for a key-requiring provider (Daemon Cloud lists keyless), or optional on the
    // custom endpoint. Editing it re-lists the provider's models with the entered key as a
    // transient credential (catalog providers only). A reveal toggle lets the user verify the
    // pasted key; masked by default.
    RowLayout {
        Layout.fillWidth: true
        visible: picker.providerRequiresKey || picker.customEndpoint
        spacing: 6
        Kit.TextField {
            id: keyField
            Layout.fillWidth: true
            placeholderText: picker.customEndpoint ? qsTr("API key (optional)")
                                                   : qsTr("Paste API key")
            echoMode: picker.revealKey ? TextInput.Normal : TextInput.Password
            onTextEdited: picker._reportSelection() // every keystroke re-arms the shared key gate
            onEditingFinished: if (ProviderCatalog && picker.providerId.length > 0
                                   && !picker.customEndpoint)
                                   ProviderCatalog.refreshModels(picker.providerId, "", text);
        }
        Kit.IconButton {
            icon: picker.revealKey ? FontIcons.fa_eye_slash : FontIcons.fa_eye
            iconPointSize: 13
            tooltipText: picker.revealKey ? qsTr("Hide API key") : qsTr("Show API key")
            onClicked: picker.revealKey = !picker.revealKey
        }
    }

    // The shared key gate's blocking reason (empty until an authenticated LIST fails).
    Text {
        visible: picker.providerRequiresKey && text.length > 0
        text: picker._hasSetup ? AgentSetup.keyGateMessage : ""
        font.family: FontIcons.display; font.pixelSize: 12; color: Theme.danger
        Layout.fillWidth: true; wrapMode: Text.WordWrap
    }

    SectionLabel { visible: !picker.customEndpoint; text: qsTr("Model") }
    Text {
        visible: !picker.customEndpoint && modelList.count === 0
        text: picker.providerRequiresKey && keyField.text.length === 0
              ? qsTr("Enter your API key to list models.")
              : qsTr("Discovering models…")
        font.family: FontIcons.display; font.pixelSize: 12; color: Theme.textMuted
        Layout.fillWidth: true; wrapMode: Text.WordWrap
    }
    ListView {
        id: modelList
        visible: !picker.customEndpoint
        Layout.fillWidth: true
        Layout.preferredHeight: picker.customEndpoint ? 0 : Math.min(contentHeight, 132)
        clip: true
        // Rebuilt explicitly (never a broken binding): on provider change (picker
        // onProviderIdChanged) and whenever the catalog re-offers this provider's models.
        property var rows: []
        function refreshRows() {
            rows = (ProviderCatalog && picker.providerId.length > 0)
                   ? ProviderCatalog.offeredModels(picker.providerId) : [];
        }
        Component.onCompleted: refreshRows()
        model: rows
        Connections {
            target: ProviderCatalog
            function onOfferedModelsChanged(pid) {
                if (pid !== picker.providerId)
                    return;
                var rows = ProviderCatalog.offeredModels(picker.providerId);
                modelList.rows = rows;
                // The key gate itself (FIX 4) is resolved by FirstRunModel off this same signal;
                // here only drop a stale model selection the re-listed (authenticated) rows no
                // longer offer.
                if (picker.providerRequiresKey && keyField.text.length > 0
                    && picker.model.length > 0) {
                    var still = false;
                    for (var i = 0; i < rows.length; ++i)
                        if (rows[i].id === picker.model) { still = true; break; }
                    if (!still)
                        picker.model = "";
                }
            }
        }
        delegate: Rectangle {
            required property var modelData
            width: ListView.view ? ListView.view.width : 0
            height: 30
            radius: 6
            color: modelData.id === picker.model ? Theme.hover : "transparent"
            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 8
                text: modelData.kind === "discover" ? "+ " + modelData.name : modelData.name
                font.family: FontIcons.display; font.pixelSize: 12
                color: modelData.kind === "discover" ? Theme.accent : Theme.text
            }
            TapHandler {
                onTapped: {
                    if (modelData.kind === "discover") {
                        // In-place discovery: a dialog in the window overlay, so it works above
                        // the first-run gate AND from the "+ New agent" dialog (a Nav.open would
                        // land underneath the wizard overlay).
                        discoverDialog.openDialog();
                    } else {
                        picker.model = modelData.id;
                        picker._pushInference();
                    }
                }
            }
        }
    }

    // Active downloads stay visible here after the discover dialog closes (compact single-line
    // rows, terminal rows hidden); tapping a row reopens the dialog, where the full queue
    // (pause/retry/cancel/use) lives. Self-hides when nothing is downloading.
    DownloadsPanel {
        Layout.fillWidth: true
        compact: true
        activeOnly: true
        onOpenRequested: discoverDialog.openDialog()
    }

    // Wizard convenience: when a download finishes while a LOCAL provider is selected and no
    // model is picked yet, auto-select the freshly installed model — the wizard's Finish enables
    // without another click. (The provider catalog re-offers local models on the same signal, so
    // the matching row appears in the list independently of this.) Only ids the provider actually
    // OFFERS are eligible: a finished download may be a non-offerable companion artifact (e.g. a
    // vision-projector "mmproj" file), which must never become the chat model.
    Connections {
        target: ModelCatalog
        function onDownloadFinished(modelId) {
            if (picker.model.length > 0 || picker.currentProviderKind() !== "local")
                return;
            var offered = ProviderCatalog
                          ? ProviderCatalog.offeredModels(picker.providerId) : [];
            for (var i = 0; i < offered.length; ++i)
                if (offered[i].id === modelId) {
                    picker.model = modelId;
                    picker._pushInference();
                    return;
                }
        }
    }

    // Model-operation failures surfaced by the catalog facade (downloads / activation): without
    // this, an HF network error makes the flow look silently dead.
    Text {
        visible: typeof ModelCatalog !== "undefined" && ModelCatalog
                 && ModelCatalog.lastError.length > 0
        text: (typeof ModelCatalog !== "undefined" && ModelCatalog) ? ModelCatalog.lastError : ""
        font.family: FontIcons.display; font.pixelSize: 11; color: Theme.danger
        Layout.fillWidth: true; wrapMode: Text.WordWrap
    }

    DiscoverDialog {
        id: discoverDialog
        // "Use this model" on a finished download: select it here (same catalog id space as the
        // offered-model rows); the dialog closes itself.
        onUseModelRequested: function(modelId) {
            picker.model = modelId;
            picker._pushInference();
        }
    }
}
