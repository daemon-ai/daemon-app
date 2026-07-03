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
// (+ `keyValidated` for a key-required vendor). `descriptorFor(providerId)` on ProviderCatalog
// yields the ProviderSelector + base URL the persisted profile needs.
ColumnLayout {
    id: picker
    spacing: 10

    // The ProviderCatalog descriptor id (e.g. "anthropic"/"openai"/"daemon_cloud"/"llama_cpp").
    property string providerId: ""
    // The selection-only model id the profile persists (chosen from ProviderModels).
    property string model: ""
    // The entered API key (transient; a key-requiring vendor authenticates its LIST with it).
    property alias key: keyField.text
    // Whether the API key field is unmasked (masked by default; the eye toggle flips it).
    property bool revealKey: false
    // Whether the entered key has been PROVEN to authenticate (a non-empty authenticated
    // ProviderModels listing). Reset whenever the provider or key changes.
    property bool keyValidated: false
    property string keyValidateMessage: ""
    // Prefer Daemon Cloud as the initial provider (keyless, works out of the box).
    property bool preferDaemonCloud: true

    // NOT a reactive binding: ProviderCatalog.providers() is a plain Q_INVOKABLE and the node's
    // catalog arrives asynchronously, so it is re-read explicitly on providersChanged.
    property var providerRows: ProviderCatalog ? ProviderCatalog.providers() : []
    readonly property var providerDescriptor: (ProviderCatalog && providerId.length > 0)
                                              ? ProviderCatalog.descriptorFor(providerId) : ({})
    readonly property bool providerRequiresKey: providerDescriptor
                                                && providerDescriptor.requiresKey === true
    // A provider + a concrete model are chosen, and any required key is at least entered.
    readonly property bool inferenceComplete: providerId.length > 0 && model.length > 0
                                              && (!providerRequiresKey || keyField.text.length > 0)

    // Emitted after a key-required vendor's authenticated model LIST resolves, so a consumer can log
    // / react to the validation outcome (the wizard funnels this into FirstRun.logKeyValidation).
    signal keyValidationResolved(string provider, bool requiresKey, int count, bool pass)

    function _seedProvider() {
        if (providerId.length > 0 || providerRows.length === 0)
            return;
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
        picker.keyValidated = false;
        picker.keyValidateMessage = "";
        if (ProviderCatalog)
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
        picker.providerRows = ProviderCatalog ? ProviderCatalog.providers() : [];
        picker._seedProvider();
        providerBox.syncFromModel();
    }
    Connections {
        target: ProviderCatalog
        function onProvidersChanged() {
            picker.providerRows = ProviderCatalog ? ProviderCatalog.providers() : [];
            picker._seedProvider();
            // The rebuilt dropdown model reset currentIndex; re-point it at the selection.
            providerBox.syncFromModel();
        }
    }

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

    // API key: only for a key-requiring provider (Daemon Cloud lists keyless). Editing it re-lists
    // the provider's models with the entered key as a transient credential. A reveal toggle lets the
    // user verify the pasted key; masked by default.
    RowLayout {
        Layout.fillWidth: true
        visible: picker.providerRequiresKey
        spacing: 6
        Kit.TextField {
            id: keyField
            Layout.fillWidth: true
            placeholderText: qsTr("Paste API key")
            echoMode: picker.revealKey ? TextInput.Normal : TextInput.Password
            onTextEdited: { picker.keyValidated = false; picker.keyValidateMessage = ""; }
            onEditingFinished: if (ProviderCatalog && picker.providerId.length > 0)
                                   ProviderCatalog.refreshModels(picker.providerId, "", text);
        }
        Kit.IconButton {
            icon: picker.revealKey ? FontIcons.fa_eye_slash : FontIcons.fa_eye
            iconPointSize: 13
            tooltipText: picker.revealKey ? qsTr("Hide API key") : qsTr("Show API key")
            onClicked: picker.revealKey = !picker.revealKey
        }
    }

    Text {
        visible: picker.providerRequiresKey && picker.keyValidateMessage.length > 0
        text: picker.keyValidateMessage
        font.family: FontIcons.display; font.pixelSize: 12; color: Theme.danger
        Layout.fillWidth: true; wrapMode: Text.WordWrap
    }

    SectionLabel { text: qsTr("Model") }
    Text {
        visible: modelList.count === 0
        text: picker.providerRequiresKey && keyField.text.length === 0
              ? qsTr("Enter your API key to list models.")
              : qsTr("Discovering models…")
        font.family: FontIcons.display; font.pixelSize: 12; color: Theme.textMuted
        Layout.fillWidth: true; wrapMode: Text.WordWrap
    }
    ListView {
        id: modelList
        Layout.fillWidth: true
        Layout.preferredHeight: Math.min(contentHeight, 132)
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
                // For a key-required vendor the LIST is authenticated, so a non-empty result proves
                // the entered key works. Gate on it and surface a clear message otherwise.
                if (picker.providerRequiresKey && keyField.text.length > 0) {
                    var count = rows.length;
                    picker.keyValidated = count > 0;
                    var vendor = (picker.providerDescriptor
                                  && picker.providerDescriptor.name !== undefined)
                                 ? picker.providerDescriptor.name : picker.providerId;
                    picker.keyValidateMessage = picker.keyValidated
                        ? "" : qsTr("Couldn't verify this API key with %1 — check it and try again.").arg(vendor);
                    picker.keyValidationResolved(picker.providerId, true, count, picker.keyValidated);
                    if (picker.model.length > 0) {
                        var still = false;
                        for (var i = 0; i < rows.length; ++i)
                            if (rows[i].id === picker.model) { still = true; break; }
                        if (!still)
                            picker.model = "";
                    }
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
        }
    }
}
