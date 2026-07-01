// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Dialogs
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// The profile editor + Skills/tools curator. Loads a working copy of the selected
// profile, lets the user edit fields and toggle skills/tools, and writes back via
// IProfileStore.updateProfile on Save. The toggle rows are fully controlled (they
// reflect the working arrays) to avoid binding breaks.
Item {
    id: root

    property string profileId: ""
    // Working copy fields.
    property string wModel: ""
    // wProvider is the ProviderSelector wire string saved on the profile ("genai" | "daemon_api" |
    // "llama_cpp" | "mistral_rs" | "mock"); wProviderId is the ProviderCatalog descriptor id the
    // picker/discovery keys on ("anthropic" | "openai" | "daemon_cloud" | "llama_cpp" | ...). All
    // genai cloud vendors share the "genai" selector and are disambiguated by the id.
    property string wProvider: ""
    property string wProviderId: ""
    property string wBaseUrl: ""
    property var wSkills: []
    property var wTools: []

    // The providers the node offers (ProviderCatalog), fetched over the wire. Mock is never listed;
    // `cloud` marks providers that take a Base URL; `wireSelector` is what the profile persists.
    readonly property var providerChoices: ProviderCatalog ? ProviderCatalog.providers() : []
    // The index of wProviderId within the provider list, or -1 for an unlisted value (e.g. a
    // Mock-valued profile): -1 means "render the non-selectable caption".
    readonly property int providerIndex: {
        for (var i = 0; i < providerChoices.length; ++i)
            if (providerChoices[i].id === root.wProviderId) return i;
        return -1;
    }
    readonly property bool providerIsCloud: providerIndex >= 0 && providerChoices[providerIndex].cloud

    // Best-effort resolve of a saved (wireSelector, model) back to a ProviderCatalog descriptor id:
    // a unique selector match wins; for genai's shared selector, prefer the vendor whose id prefixes
    // the model id, else the first match. Empty when nothing matches (e.g. a Mock-valued profile).
    function _inferProviderId(selector, model) {
        if (selector.length === 0) return "";
        var matches = [];
        for (var i = 0; i < providerChoices.length; ++i)
            if (providerChoices[i].wireSelector === selector) matches.push(providerChoices[i]);
        if (matches.length === 0) return "";
        if (matches.length === 1) return matches[0].id;
        for (var j = 0; j < matches.length; ++j)
            if (model.length > 0 && model.indexOf(matches[j].id) === 0) return matches[j].id;
        return matches[0].id;
    }
    // PRO-8 revision history model (null in mock mode, which has no daemon revision log).
    readonly property var historyModel: Profiles.history()
    property bool showHistory: false

    signal deleted()

    onProfileIdChanged: load()
    Component.onCompleted: load()

    function load() {
        const p = profileId.length > 0 ? Profiles.profile(profileId) : ({});
        nameField.text = p.name !== undefined ? p.name : "";
        // Preserve whatever provider the profile has (incl. an unlisted "mock"); never coerce it -
        // the caption renders it and save() writes wProvider (the selector) back verbatim.
        wProvider = p.provider !== undefined ? p.provider : "";
        wBaseUrl = p.baseUrl !== undefined ? p.baseUrl : "";
        wModel = p.model !== undefined ? p.model : "";
        wProviderId = _inferProviderId(wProvider, wModel);
        providerCombo.syncFromModel();
        baseUrlField.text = wBaseUrl;
        // Ask the node for this provider's models (using the profile's stored credential) so the
        // model dropdown lists real, selectable models; the reactive refresh repopulates on arrival.
        if (ProviderCatalog && wProviderId.length > 0)
            ProviderCatalog.refreshModels(wProviderId, profileId, "");
        modelCombo.reload();
        descField.text = p.description !== undefined ? p.description : "";
        promptArea.text = p.systemPrompt !== undefined ? p.systemPrompt : "";
        wSkills = p.skills !== undefined ? p.skills.slice() : [];
        wTools = p.tools !== undefined ? p.tools.slice() : [];
        credText.refresh();
        if (profileId.length > 0)
            Profiles.requestHistory(profileId);
    }

    function _toggle(arr, id) {
        const i = arr.indexOf(id);
        const copy = arr.slice();
        if (i >= 0) copy.splice(i, 1); else copy.push(id);
        return copy;
    }

    function save() {
        Profiles.updateProfile(profileId, {
            "name": nameField.text,
            // wProvider is written verbatim (an unlisted Mock value survives untouched); baseUrl only
            // applies to cloud providers - empty clears it (=> node default).
            "provider": root.wProvider,
            "baseUrl": root.providerIsCloud ? root.wBaseUrl : "",
            "model": root.wModel,
            "description": descField.text,
            "systemPrompt": promptArea.text,
            "skills": wSkills,
            "tools": wTools,
        });
    }

    // Empty state.
    Text {
        anchors.centerIn: parent
        visible: root.profileId.length === 0
        text: qsTr("Select or create a profile.")
        font.family: FontIcons.display; font.pixelSize: 13; color: Theme.textMuted
    }

    QQC.ScrollView {
        anchors.fill: parent
        visible: root.profileId.length > 0
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: root.width - 48
            x: 24
            y: 20
            spacing: 14

            RowLayout {
                Layout.fillWidth: true
                SectionLabel { text: qsTr("Profile"); Layout.fillWidth: true }
                Kit.TextButton {
                    text: Profiles.defaultProfileId === root.profileId
                          ? qsTr("Default") : qsTr("Make default")
                    enabled: Profiles.defaultProfileId !== root.profileId
                    onClicked: Profiles.setDefault(root.profileId)
                }
                // PRO-7: export this profile to / import a profile from a portable .dist file.
                Kit.IconButton {
                    icon: FontIcons.fa_download
                    iconPointSize: 13; implicitWidth: 32; implicitHeight: 28
                    tooltipText: qsTr("Export profile")
                    onClicked: { exportDialog.currentFile = "file:" + root.profileId + ".dist";
                                 exportDialog.open(); }
                }
                Kit.IconButton {
                    icon: FontIcons.fa_arrow_up
                    iconPointSize: 13; implicitWidth: 32; implicitHeight: 28
                    tooltipText: qsTr("Import profile")
                    onClicked: importDialog.open()
                }
                Kit.IconButton {
                    icon: FontIcons.fa_trash; iconColor: Theme.danger
                    iconPointSize: 13; implicitWidth: 32; implicitHeight: 28
                    tooltipText: qsTr("Delete profile")
                    onClicked: { Profiles.remove(root.profileId); root.deleted(); }
                }
            }

            Kit.TextField { id: nameField; Layout.fillWidth: true; placeholderText: qsTr("Name") }

            // --- Provider -------------------------------------------------
            SectionLabel { text: qsTr("Provider") }
            Kit.Dropdown {
                id: providerCombo
                Layout.fillWidth: true
                model: root.providerChoices.map(function(p) { return p.name; })
                // Reflect root.wProvider into the current index; -1 for an unlisted (e.g. Mock) value
                // so no real provider is shown as selected (the caption below explains it).
                function syncFromModel() { currentIndex = root.providerIndex; }
                onActivated: {
                    var row = root.providerChoices[currentIndex];
                    root.wProviderId = row.id;
                    // The profile persists the ProviderSelector (row.wireSelector), NOT the id: all
                    // genai vendors share "genai" and are disambiguated by id at discovery time.
                    root.wProvider = row.wireSelector;
                    // Switching provider invalidates the current model (selection-only).
                    root.wModel = "";
                    // Pre-fill the provider's default base URL (e.g. Daemon Cloud) for a cloud
                    // provider with no base set; the node/provider default otherwise.
                    if (row.cloud && baseUrlField.text.length === 0)
                        baseUrlField.text = row.defaultBaseUrl !== undefined ? row.defaultBaseUrl : "";
                    root.wBaseUrl = baseUrlField.text;
                    // Fetch this provider's models (stored credential for an existing profile); the
                    // reactive refresh repopulates the dropdown when they arrive.
                    if (ProviderCatalog)
                        ProviderCatalog.refreshModels(root.wProviderId, root.profileId, "");
                    modelCombo.reload();
                }
            }
            // A loaded profile whose provider is not a real, selectable one (e.g. a Mock-valued
            // profile under DAEMON_APP_SERVICE_MODE=mock): show it read-only rather than offering Mock
            // or silently coercing it to a real provider on save.
            Text {
                Layout.fillWidth: true
                visible: root.providerIndex < 0 && root.wProvider.length > 0
                text: qsTr("Current provider: %1 (test provider — not selectable)").arg(root.wProvider)
                wrapMode: Text.Wrap
                font.family: FontIcons.display; font.pixelSize: 12; color: Theme.textMuted
            }

            // Optional Base URL (cloud providers only); empty = the provider/node default.
            Kit.TextField {
                id: baseUrlField
                Layout.fillWidth: true
                visible: root.providerIsCloud
                placeholderText: qsTr("Base URL (optional, e.g. https://api.daemon.ai/api/v1)")
                onEditingFinished: root.wBaseUrl = text
            }

            // Model is chosen (selection-only) from the node's per-provider list (ProviderModels for
            // cloud; installed + "Discover More Models" for local). Never free-text.
            Kit.Dropdown {
                id: modelCombo
                Layout.fillWidth: true
                // The offered-model rows for the selected provider ({id, name, kind}); the trailing
                // discover row (local) opens the download flow instead of selecting a model. An
                // existing wModel absent from the list is kept selectable so it never silently drops.
                property var rows: []
                function rebuildRows() {
                    var out = (ProviderCatalog && root.wProviderId.length > 0)
                              ? ProviderCatalog.offeredModels(root.wProviderId).slice() : [];
                    var present = root.wModel.length === 0;
                    for (var i = 0; i < out.length; ++i)
                        if (out[i].id === root.wModel) present = true;
                    if (!present)
                        out.unshift({ "id": root.wModel, "name": root.wModel, "kind": "model" });
                    rows = out;
                }
                function reload() {
                    rebuildRows();
                    model = rows.map(function(m) { return m.name; });
                    syncFromModel();
                }
                function syncFromModel() {
                    currentIndex = 0;
                    for (var i = 0; i < rows.length; ++i)
                        if (rows[i].id === root.wModel) { currentIndex = i; break; }
                }
                onActivated: {
                    var row = rows[currentIndex];
                    if (row === undefined) return;
                    if (row.kind === "discover") {
                        // Local "Discover More Models": route to the Models hub download flow; a
                        // finished download re-offers the new model (offeredModelsChanged).
                        if (Nav) Nav.open("models", "discover");
                        syncFromModel(); // don't leave the discover row selected
                        return;
                    }
                    root.wModel = row.id;
                }
            }
            // Repopulate the model dropdown when the node answers ProviderModels (or a local download
            // finishes) for the currently-selected provider.
            Connections {
                target: ProviderCatalog
                function onOfferedModelsChanged(providerId) {
                    if (providerId === root.wProviderId) modelCombo.reload();
                }
            }

            // --- Credential (profile-scoped) ------------------------------
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text {
                    id: credText
                    Layout.fillWidth: true
                    // Re-query on demand so the CredentialList refresh (credentialsChanged) updates it.
                    property var info: ({})
                    function refresh() { info = Accounts.credentialFor(root.profileId); }
                    text: info && info.present
                          ? qsTr("Credential: connected%1")
                                .arg(info.hint && info.hint.length > 0 ? " (" + info.hint + ")" : "")
                          : qsTr("Credential: none — add a key")
                    color: info && info.present ? Theme.text : Theme.textMuted
                    font.family: FontIcons.display; font.pixelSize: 12
                }
                Kit.TextButton {
                    text: qsTr("Set key")
                    onClicked: credWizard.openForProfile(root.profileId)
                }
            }

            Kit.TextField {
                id: descField; Layout.fillWidth: true
                placeholderText: qsTr("Short description")
            }

            SectionLabel { text: qsTr("System prompt") }
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 110
                radius: Theme.radius
                color: "transparent"
                border.color: Theme.border

                QQC.ScrollView {
                    anchors.fill: parent
                    anchors.margins: 6
                    clip: true
                    QQC.TextArea {
                        id: promptArea
                        wrapMode: TextEdit.Wrap
                        font.family: FontIcons.display
                        font.pixelSize: 13
                        color: Theme.text
                        background: null
                    }
                }
            }

            // --- Skills curator -------------------------------------------
            SectionLabel { text: qsTr("Skills") }
            Repeater {
                model: Profiles.availableSkills()
                delegate: CuratorRow {
                    required property var modelData
                    Layout.fillWidth: true
                    label: modelData.name
                    description: modelData.description
                    checked: root.wSkills.indexOf(modelData.id) >= 0
                    onToggled: root.wSkills = root._toggle(root.wSkills, modelData.id)
                }
            }

            // --- Tools curator --------------------------------------------
            SectionLabel { text: qsTr("Tools") }
            Repeater {
                model: Profiles.availableTools()
                delegate: CuratorRow {
                    required property var modelData
                    Layout.fillWidth: true
                    label: modelData.name
                    description: modelData.description
                    checked: root.wTools.indexOf(modelData.id) >= 0
                    onToggled: root.wTools = root._toggle(root.wTools, modelData.id)
                }
            }

            // --- Version history (PRO-8) ----------------------------------
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 6
                visible: root.historyModel !== null
                SectionLabel { text: qsTr("History"); Layout.fillWidth: true }
                Kit.TextButton {
                    visible: Profiles.historyAvailable
                    text: root.showHistory ? qsTr("Hide") : qsTr("Show")
                    onClicked: root.showHistory = !root.showHistory
                }
            }
            // Versioning needs a durable (revision-log) daemon; say so plainly instead of showing an
            // empty panel + a misleading toast when the daemon hosts no history.
            Text {
                Layout.fillWidth: true
                visible: root.historyModel !== null && !Profiles.historyAvailable
                text: qsTr("Version history needs a durable daemon.")
                wrapMode: Text.Wrap
                font.family: FontIcons.display; font.pixelSize: 12; color: Theme.textMuted
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                visible: root.showHistory && root.historyModel !== null && Profiles.historyAvailable

                Text {
                    visible: root.historyModel !== null && root.historyModel.count === 0
                    text: qsTr("No revisions yet.")
                    font.family: FontIcons.display; font.pixelSize: 12; color: Theme.textMuted
                }
                Repeater {
                    model: root.historyModel
                    delegate: Rectangle {
                        required property var entry
                        Layout.fillWidth: true
                        implicitHeight: 44
                        radius: 6
                        color: Theme.surface
                        border.color: Theme.border

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 8
                            spacing: 8
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                RowLayout {
                                    spacing: 6
                                    Text {
                                        text: qsTr("#%1").arg(entry.seq)
                                        font.family: FontIcons.mono; font.pixelSize: 11
                                        color: Theme.accent
                                    }
                                    Text {
                                        text: entry.author === "operator"
                                              ? qsTr("operator") : qsTr("agent %1").arg(entry.author)
                                        font.family: FontIcons.display; font.pixelSize: 11
                                        color: Theme.textMuted
                                    }
                                }
                                Text {
                                    text: entry.reason
                                    font.family: FontIcons.display; font.pixelSize: 12
                                    color: Theme.text; elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }
                            Kit.TextButton {
                                text: qsTr("Revert to this")
                                onClicked: Profiles.revertProfile(root.profileId, entry.seq)
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 6
                Layout.bottomMargin: 20
                Item { Layout.fillWidth: true }
                // Reloads the working copy from the last-saved spec (distinct from the durable
                // "Revert to this" revision action above).
                Kit.TextButton { text: qsTr("Discard changes"); onClicked: root.load() }
                Kit.TextButton { text: qsTr("Save"); accentFilled: true; onClicked: root.save() }
            }
        }
    }

    // PRO-7 file pickers. The artifact is the portable Distribution CBOR (lossless round-trip).
    FileDialog {
        id: exportDialog
        title: qsTr("Export profile")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Profile distributions (*.dist)"), qsTr("All files (*)")]
        defaultSuffix: "dist"
        onAccepted: Profiles.exportProfileToFile(root.profileId, selectedFile)
    }
    FileDialog {
        id: importDialog
        title: qsTr("Import profile")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Profile distributions (*.dist)"), qsTr("All files (*)")]
        onAccepted: Profiles.importProfileFromFile(selectedFile, "")
    }

    // Refresh the history panel when revisions arrive (or after a revert lands).
    Connections {
        target: Profiles
        function onReverted(id) { if (id === root.profileId) root.load(); }
    }

    // Profile-scoped credential entry: reuse the Add-account wizard, targeting THIS profile so the
    // key lands under it (not the active profile).
    AddAccountWizard { id: credWizard }

    // Keep the credential-status line in sync with the redacted CredentialList.
    Connections {
        target: Accounts
        function onCredentialsChanged() { credText.refresh(); }
    }
}
