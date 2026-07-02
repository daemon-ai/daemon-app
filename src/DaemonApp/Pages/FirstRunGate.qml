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

    // A7: the guided inference selection (provider -> key -> model) is now the SHARED
    // AgentInferencePicker, so the wizard and "+ New agent" use one node-driven picker. These read
    // the picker's live selection (or defaults before the inference step mounts the picker).
    readonly property string providerId: inferencePicker.item ? inferencePicker.item.providerId : ""
    readonly property string model: inferencePicker.item ? inferencePicker.item.model : ""
    readonly property bool inferenceComplete: inferencePicker.item
                                              && inferencePicker.item.inferenceComplete
    readonly property bool providerRequiresKey: inferencePicker.item
                                                && inferencePicker.item.providerRequiresKey
    readonly property bool keyValidated: inferencePicker.item && inferencePicker.item.keyValidated
    readonly property string pickerKey: inferencePicker.item ? inferencePicker.item.key : ""

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

            // --- Phase: inference gate (the SHARED provider -> key -> model picker, A7) ---
            Loader {
                id: inferencePicker
                Layout.fillWidth: true
                active: root.phase === "inference"
                visible: active
                sourceComponent: AgentInferencePicker {
                    // FIX 4: the wizard logs the authenticated-LIST key-validation outcome.
                    onKeyValidationResolved: function(provider, requiresKey, count, pass) {
                        if (FirstRun)
                            FirstRun.logKeyValidation(provider, requiresKey, count, pass);
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
                    // Enabled once a provider + concrete model are chosen and, for a key-required
                    // vendor, the key has been PROVEN to authenticate (FIX 4) - not merely typed.
                    enabled: root.inferenceComplete
                             && (!root.providerRequiresKey || root.keyValidated)
                    // Persist a working profile (ProviderSelector + model + base URL) + profile-scoped
                    // key + make default, then finish - zero env required.
                    onClicked: FirstRun.applyInferenceChoice(root.providerId, root.model,
                                                             root.pickerKey)
                }
            }
        }
    }
}
