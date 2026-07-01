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
    property string wProvider: ""
    property string wBaseUrl: ""
    property var wSkills: []
    property var wTools: []

    // The REAL model providers offered by the picker, sourced from the store's single-source list.
    // Mock is deliberately absent there: it is never a default or a user choice, reachable only under
    // DAEMON_APP_SERVICE_MODE=mock. `cloud` marks providers that take an optional Base URL.
    readonly property var providerChoices: Profiles.pickerProviders()
    // The index of wProvider within the real-provider list, or -1 for an unlisted value (e.g. a
    // Mock-valued profile under the mock flag): -1 means "render the non-selectable caption".
    readonly property int providerIndex: {
        for (var i = 0; i < providerChoices.length; ++i)
            if (providerChoices[i].id === root.wProvider) return i;
        return -1;
    }
    readonly property bool providerIsCloud: providerIndex >= 0 && providerChoices[providerIndex].cloud

    // The default base URL to pre-fill for a cloud provider (empty = use the node/provider default).
    function _defaultBaseFor(providerId) {
        return providerId === "daemon_api" ? "https://api.daemon.ai/api/v1" : "";
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
        // Preserve whatever provider the profile has (incl. an unlisted "mock" under the mock flag);
        // never coerce it here - the caption renders it and save() writes it back verbatim.
        wProvider = p.provider !== undefined ? p.provider : "";
        wBaseUrl = p.baseUrl !== undefined ? p.baseUrl : "";
        providerCombo.syncFromModel();
        baseUrlField.text = wBaseUrl;
        wModel = p.model !== undefined ? p.model : "";
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
                    root.wProvider = root.providerChoices[currentIndex].id;
                    // Pre-fill the cloud default when switching to a cloud provider with no base set.
                    if (root.providerChoices[currentIndex].cloud && baseUrlField.text.length === 0)
                        baseUrlField.text = root._defaultBaseFor(root.wProvider);
                    root.wBaseUrl = baseUrlField.text;
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

            // Model is chosen from the installed models (shared catalog), filtered by provider.
            Kit.Dropdown {
                id: modelCombo
                Layout.fillWidth: true
                // Recompute the provider-filtered id list and reselect wModel (kept selectable even
                // if the filter omits it, so an existing value never silently drops).
                function ids() {
                    var list = ModelCatalog.installedIdsForProvider(root.wProvider);
                    if (root.wModel.length > 0 && list.indexOf(root.wModel) < 0)
                        list = [root.wModel].concat(list);
                    return list;
                }
                function reload() {
                    model = ids();
                    syncFromModel();
                }
                function syncFromModel() {
                    currentIndex = Math.max(0, model.indexOf(root.wModel));
                }
                onActivated: root.wModel = model[currentIndex]
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
