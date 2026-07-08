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

    // The name the Finish commit uses: a foreign profile is named in the agent-type pane
    // (acpNameField); a native profile is named on the inference step (agentNameField).
    readonly property bool foreignEngine: (typeof AgentSetup !== "undefined" && AgentSetup)
                                          && AgentSetup.engineKind === "Foreign"
    readonly property string finishName: foreignEngine ? acpNameField.text.trim()
                                                       : agentNameField.text.trim()

    // A7: the guided inference selection (provider -> key -> model) is now the SHARED
    // AgentInferencePicker, so the wizard and "+ New agent" use one node-driven picker. These read
    // the picker's live selection (or defaults before the inference step mounts the picker).
    readonly property string providerId: inferencePicker.item ? inferencePicker.item.providerId : ""
    readonly property string model: inferencePicker.item ? inferencePicker.item.model : ""
    readonly property bool inferenceComplete: inferencePicker.item
                                              && inferencePicker.item.inferenceComplete

    // The chosen provider's catalog label (e.g. "Anthropic"), the seed for the agent-name
    // prefill. Falls back to the descriptor id when the catalog has no display name.
    readonly property string providerLabel: {
        if (root.providerId.length === 0 || typeof ProviderCatalog === "undefined"
            || !ProviderCatalog)
            return "";
        var d = ProviderCatalog.descriptorFor(root.providerId);
        return (d && d.name) ? d.name : root.providerId;
    }
    // Re-seed the name from the provider label (lowercased) whenever the provider changes, until
    // the user takes ownership of the field by editing it.
    onProviderLabelChanged: {
        if (!agentNameField.userEdited)
            agentNameField.text = root.providerLabel.toLowerCase();
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
                          : root.phase === "agenttype"
                            ? qsTr("What kind of agent do you want?")
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

            // --- Phase: agenttype (CON-16 + Phase G): the SHARED engine picker, with the foreign
            // BACKEND choice folded into this pane. Native (default) continues into the inference
            // step. A foreign agent reveals the backend rows: "Agent's own backend" (AgentNative)
            // finishes here — the agent brings its own model — while "Node provider (gateway)"
            // (NodeProvider) needs the shared inference sub-form, so Continue advances to it.
            ColumnLayout {
                visible: root.phase === "agenttype"
                Layout.fillWidth: true
                spacing: 10
                SectionLabel { text: qsTr("Agent type") }
                AgentTypePicker {
                    id: agentTypePicker
                    Layout.fillWidth: true
                    visible: root.phase === "agenttype"
                    // Fresh catalog + discovery badges each time the step mounts.
                    onVisibleChanged: if (visible) refresh()
                }
                Text {
                    text: agentTypePicker.nativeEngine
                          ? qsTr("Runs in the daemon — pick a provider and model next.")
                          : (AgentSetup && AgentSetup.needsInference
                             ? qsTr("Routed through the node's provider gateway — pick a provider "
                                    + "and model next.")
                             : qsTr("This agent runs a foreign engine using its own backend — it "
                                    + "brings its own model, so no provider or key is needed."))
                    font.family: FontIcons.display; font.pixelSize: 12; color: Theme.textMuted
                    Layout.fillWidth: true; wrapMode: Text.WordWrap
                }
                // Foreign only: the backend choice (AgentNative vs NodeProvider) + optional model
                // hint / gateway affordance, driving the shared AgentSetupModel.
                SectionLabel { visible: !agentTypePicker.nativeEngine; text: qsTr("Backend") }
                ForeignBackendPicker {
                    visible: !agentTypePicker.nativeEngine
                    Layout.fillWidth: true
                }
                // Foreign agents get their NAME here (the native path names on the inference
                // step): prefilled from the agent, editable, required.
                SectionLabel { visible: !agentTypePicker.nativeEngine; text: qsTr("Agent name") }
                Kit.TextField {
                    id: acpNameField
                    visible: !agentTypePicker.nativeEngine
                    Layout.fillWidth: true
                    placeholderText: qsTr("agent name")
                    property bool userEdited: false
                    onTextEdited: userEdited = true
                }
                Connections {
                    target: agentTypePicker
                    function onSelected(agent, installed) {
                        if (!acpNameField.userEdited)
                            acpNameField.text = agent;
                    }
                }
            }

            // --- Phase: inference gate: the agent's NAME + the SHARED picker (A7) -------------
            // The name is the profile id the node keys the agent by (there is no separate
            // display-name field on the wire): prefilled from the chosen provider's label,
            // editable, required. The wizard mints a NAMED agent instead of leaving the node's
            // seeded placeholder id in the Fleet.
            ColumnLayout {
                // A foreign (NodeProvider) profile was already named in the agent-type pane; only
                // the native path names on the inference step.
                visible: root.phase === "inference" && !root.foreignEngine
                Layout.fillWidth: true
                spacing: 10
                SectionLabel { text: qsTr("Agent name") }
                Kit.TextField {
                    id: agentNameField
                    Layout.fillWidth: true
                    placeholderText: qsTr("agent name")
                    // Once the user edits the name it is theirs; provider changes stop re-seeding.
                    property bool userEdited: false
                    onTextEdited: userEdited = true
                }
            }

            // --- Phase: inference gate (the SHARED provider -> key -> model picker, A7) ---
            // FIX 4: the picker reports its provider/key selection to FirstRun, which evaluates
            // (and logs) the blocking key-validation gate shared with the TUI.
            Loader {
                id: inferencePicker
                Layout.fillWidth: true
                active: root.phase === "inference"
                visible: active
                sourceComponent: AgentInferencePicker {
                    // First-run defaults to LOCAL inference (llama.cpp): keyless, works out of
                    // the box after a model download — the "+ New agent" dialog keeps the
                    // Daemon Cloud preference. The FIX 4 key-validation logging moved into
                    // FirstRunModel (setInferenceSelection), shared with the TUI.
                    preferDaemonCloud: false
                }
            }
            // A7 finish-gate outcome: the capability probe's actionable failure ("Couldn't
            // reach your model…") keeps the wizard open with the reason visible here.
            Text {
                visible: root.phase === "inference" && FirstRun && FirstRun.error.length > 0
                text: FirstRun ? FirstRun.error : ""
                font.family: FontIcons.display; font.pixelSize: 12; color: Theme.danger
                Layout.fillWidth: true; wrapMode: Text.WordWrap
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
                // Agent-type step commit (CON-16 + Phase G): when inference is needed (native, or a
                // foreign NodeProvider backend) -> Continue into the inference step; a foreign
                // AgentNative agent brings its own model -> Finish here (commit the shared-model
                // selection: engine=Foreign, AgentNative backend, optional model hint).
                Kit.TextButton {
                    visible: root.phase === "agenttype"
                    text: (AgentSetup && AgentSetup.needsInference) ? qsTr("Continue")
                                                                    : qsTr("Finish setup")
                    accentFilled: true
                    enabled: agentTypePicker.nativeEngine
                             || (agentTypePicker.engineAgentInstalled
                                 && acpNameField.text.trim().length > 0
                                 && !(AgentSetup && AgentSetup.gatewayRequired))
                    onClicked: {
                        if (AgentSetup && AgentSetup.needsInference)
                            FirstRun.continueFromAgentType();
                        else
                            FirstRun.commitSetup(acpNameField.text.trim());
                    }
                }
                Kit.TextButton {
                    visible: root.phase === "inference"
                    text: qsTr("Finish setup")
                    accentFilled: true
                    // Enabled once the agent has a non-empty name, a provider + concrete model
                    // are chosen and, for a key-required vendor, the key has been PROVEN to
                    // authenticate - not merely typed. The key gate is evaluated by the shared
                    // AgentSetupModel (via FirstRun), so GUI and TUI block on one implementation.
                    enabled: root.inferenceComplete
                             && (FirstRun ? FirstRun.keyGatePassed : true)
                             && root.finishName.length > 0
                    // Commit the shared-model selection (Core, or foreign NodeProvider) under the
                    // chosen name + profile-scoped key + make default, then finish. The pickers
                    // already drove engine/backend/inference into AgentSetup.
                    onClicked: FirstRun.commitSetup(root.finishName)
                }
            }
        }
    }
}
