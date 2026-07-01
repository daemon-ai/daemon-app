// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Full-screen onboarding gate, mounted over the shell while FirstRun.active. It
// renders one of the FirstRun phases: the connection picker, a connecting
// splash, or the inference gate. Setup completes via FirstRun.completeInference,
// after which the shell is shown.
Rectangle {
    id: root
    anchors.fill: parent
    color: Theme.background
    focus: true

    readonly property string phase: FirstRun ? FirstRun.phase : "connect"

    // Guided inference selection state (provider -> key -> model). `providerId` is the
    // ProviderCatalog descriptor id; `model` is the selection-only model id the profile persists.
    property string providerId: ""
    property string model: ""
    readonly property var providerRows: ProviderCatalog ? ProviderCatalog.providers() : []
    readonly property var providerDescriptor: (ProviderCatalog && providerId.length > 0)
                                              ? ProviderCatalog.descriptorFor(providerId) : ({})
    readonly property bool providerRequiresKey: providerDescriptor
                                                && providerDescriptor.requiresKey === true
    // Ready to finish once a provider and a concrete model are chosen, and any required key is set.
    readonly property bool inferenceComplete: providerId.length > 0 && model.length > 0
                                              && (!providerRequiresKey || keyField.text.length > 0)

    // Pick the default provider (Daemon Cloud) once the provider list arrives, and fetch its models.
    function _seedProvider() {
        if (providerId.length > 0 || providerRows.length === 0)
            return;
        var pick = providerRows[0];
        for (var i = 0; i < providerRows.length; ++i)
            if (providerRows[i].id === "daemon_cloud" || providerRows[i].kind === "daemon_cloud") {
                pick = providerRows[i]; break;
            }
        root.selectProvider(pick.id);
    }
    function selectProvider(id) {
        root.providerId = id;
        root.model = "";
        if (ProviderCatalog)
            ProviderCatalog.refreshModels(id, "", keyField.text); // transient key (no profile yet)
    }

    // Centered onboarding card.
    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width - 80, 560)
        height: Math.min(parent.height - 80, content.implicitHeight + 64)
        radius: 14
        color: Theme.surface
        border.color: Theme.border
        border.width: 1

        ColumnLayout {
            id: content
            anchors.fill: parent
            anchors.margins: 32
            spacing: 18

            // --- Brand / title ---
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                Text {
                    text: qsTr("Welcome to daemon-app")
                    font.family: FontIcons.display; font.pixelSize: 22
                    font.weight: Font.DemiBold; color: Theme.text
                }
                Text {
                    text: root.phase === "inference"
                          ? qsTr("Almost there - confirm an inference model.")
                          : root.phase === "auth"
                            ? qsTr("Sign in to the node to continue.")
                            : qsTr("Connect to a daemon node to get started.")
                    font.family: FontIcons.display; font.pixelSize: 13; color: Theme.textMuted
                    Layout.fillWidth: true; wrapMode: Text.WordWrap
                }
            }

            // --- Phase: connect (the shared picker) ---
            ConnectionPicker {
                visible: root.phase === "connect"
                Layout.fillWidth: true
                showConnect: true
            }
            Text {
                visible: root.phase === "connect" && FirstRun && FirstRun.error.length > 0
                text: FirstRun ? FirstRun.error : ""
                font.family: FontIcons.display; font.pixelSize: 12; color: Theme.danger
                Layout.fillWidth: true; wrapMode: Text.WordWrap
            }

            // --- Phase: connecting (splash) ---
            ColumnLayout {
                visible: root.phase === "connecting"
                Layout.fillWidth: true
                spacing: 12
                Item { Layout.preferredHeight: 8 }
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 12
                    QQC.BusyIndicator { running: root.phase === "connecting"; implicitWidth: 28; implicitHeight: 28 }
                    Text {
                        text: qsTr("Connecting to %1...").arg(Connection ? Connection.target : "")
                        font.family: FontIcons.display; font.pixelSize: 14; color: Theme.text
                    }
                }
                Item { Layout.preferredHeight: 8 }
            }

            // --- Phase: auth (SASL login form) ---
            ColumnLayout {
                visible: root.phase === "auth"
                Layout.fillWidth: true
                spacing: 10

                SectionLabel { text: qsTr("Sign in") }
                Kit.TextField {
                    id: usernameField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Username")
                }
                Kit.TextField {
                    id: passwordField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Password")
                    echoMode: TextInput.Password
                    onAccepted: if (signInButton.enabled) signInButton.clicked()
                }
                Text {
                    visible: FirstRun && FirstRun.error.length > 0
                    text: FirstRun ? FirstRun.error : ""
                    font.family: FontIcons.display; font.pixelSize: 12; color: Theme.danger
                    Layout.fillWidth: true; wrapMode: Text.WordWrap
                }
                Kit.TextButton {
                    id: signInButton
                    text: (Connection && Connection.state === "connecting")
                          ? qsTr("Signing in…") : qsTr("Sign in")
                    accentFilled: true
                    // Password is bound to the field only; never persisted, never logged.
                    enabled: usernameField.text.length > 0 && passwordField.text.length > 0
                             && !(Connection && Connection.state === "connecting")
                    onClicked: {
                        FirstRun.submitLogin(usernameField.text, passwordField.text);
                        passwordField.text = "";
                    }
                }
            }

            // --- Phase: inference gate (provider -> key -> model, selection-only) ---
            ColumnLayout {
                visible: root.phase === "inference"
                Layout.fillWidth: true
                spacing: 10

                // Seed Daemon Cloud + fetch its models once the node's provider list arrives.
                Component.onCompleted: root._seedProvider()
                Connections {
                    target: ProviderCatalog
                    function onProvidersChanged() { root._seedProvider(); }
                }

                SectionLabel { text: qsTr("Provider") }
                Kit.Dropdown {
                    id: providerBox
                    Layout.fillWidth: true
                    model: root.providerRows.map(function(p) { return p.name; })
                    function syncFromModel() {
                        for (var i = 0; i < root.providerRows.length; ++i)
                            if (root.providerRows[i].id === root.providerId) { currentIndex = i; return; }
                    }
                    onActivated: root.selectProvider(root.providerRows[currentIndex].id)
                }

                // API key: only for a key-requiring provider (Daemon Cloud lists keyless). Editing
                // it re-lists the provider's models with the entered key as a transient credential.
                Kit.TextField {
                    id: keyField
                    Layout.fillWidth: true
                    visible: root.providerRequiresKey
                    placeholderText: qsTr("Paste API key")
                    echoMode: TextInput.Password
                    onEditingFinished: if (ProviderCatalog && root.providerId.length > 0)
                                           ProviderCatalog.refreshModels(root.providerId, "", text);
                }

                SectionLabel { text: qsTr("Model") }
                Text {
                    visible: modelList.count === 0
                    text: root.providerRequiresKey && keyField.text.length === 0
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
                    // The node's per-provider offered models (selection-only); a local provider ends
                    // with a "Discover More Models" row that opens the download flow.
                    property var rows: (ProviderCatalog && root.providerId.length > 0)
                                       ? ProviderCatalog.offeredModels(root.providerId) : []
                    model: rows
                    Connections {
                        target: ProviderCatalog
                        function onOfferedModelsChanged(pid) {
                            if (pid === root.providerId)
                                modelList.rows = ProviderCatalog.offeredModels(root.providerId);
                        }
                    }
                    delegate: Rectangle {
                        required property var modelData
                        width: ListView.view ? ListView.view.width : 0
                        height: 30
                        radius: 6
                        color: modelData.id === root.model ? Theme.hover : "transparent"
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 8
                            text: modelData.kind === "discover"
                                  ? "+ " + modelData.name : modelData.name
                            font.family: FontIcons.display; font.pixelSize: 12
                            color: modelData.kind === "discover" ? Theme.accent : Theme.text
                        }
                        TapHandler {
                            onTapped: {
                                if (modelData.kind === "discover") {
                                    if (Nav) Nav.open("models", "discover");
                                } else {
                                    root.model = modelData.id;
                                }
                            }
                        }
                    }
                }
            }

            // --- Footer actions ---
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Kit.TextButton {
                    text: qsTr("Skip for now")
                    onClicked: FirstRun.skip()
                }
                Item { Layout.fillWidth: true }
                Kit.TextButton {
                    visible: root.phase === "inference"
                    text: qsTr("Finish setup")
                    accentFilled: true
                    // Enabled once a provider + concrete model are chosen (and any required key set).
                    enabled: root.inferenceComplete
                    // Persist a working profile (ProviderSelector + model + base URL) + profile-scoped
                    // key + make default, then finish - zero env required.
                    onClicked: FirstRun.applyInferenceChoice(root.providerId, root.model,
                                                             keyField.text)
                }
            }
        }
    }
}
