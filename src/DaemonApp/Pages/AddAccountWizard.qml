// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Generic Add-account wizard: pick a provider, pick an auth method (the provider
// advertises which it supports), then either store an API key or run a simulated
// OAuth handshake. Reusable for every provider via IAccountsService metadata.
Kit.Dialog {
    id: root

    readonly property var providers: Accounts.availableProviders()
    property var provider: providers.length > 0 ? providers[0] : ({})
    property string method: "apikey"

    function openFresh() {
        providerBox.currentIndex = 0;
        root.provider = root.providers.length > 0 ? root.providers[0] : ({});
        root._syncMethod();
        keyField.text = "";
        labelField.text = "";
        baseUrlField.text = "";
        root.open();
    }

    function _syncMethod() {
        const kinds = root.provider && root.provider.kinds ? root.provider.kinds : ["apikey"];
        root.method = methodBox.visible && methodBox.currentIndex >= 0
                ? kinds[methodBox.currentIndex] : kinds[0];
    }

    title: qsTr("Add account")
    width: 460
    showCancel: true
    acceptText: root.method === "oauth" ? qsTr("Connect") : qsTr("Save")
    closePolicy: QQC.Popup.CloseOnEscape

    // For API keys we accept synchronously; for OAuth we kick off the handshake and
    // close when it resolves (oauthCompleted), keeping the dialog up while busy.
    onAccepted: {
        if (root.method === "oauth") {
            Accounts.beginOAuth(root.provider.id);
        } else {
            Accounts.addApiKey(root.provider.id, labelField.text, keyField.text,
                               baseUrlField.text);
        }
    }

    Connections {
        target: Accounts
        function onOauthCompleted(accountId) { if (root.opened) root.close(); }
    }

    contentItem: ColumnLayout {
        spacing: 14

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            SectionLabel { text: qsTr("Provider") }
            Kit.Dropdown {
                id: providerBox
                Layout.fillWidth: true
                model: root.providers.map(function(p) { return p.name; })
                onActivated: {
                    root.provider = root.providers[currentIndex];
                    methodBox.currentIndex = 0;
                    root._syncMethod();
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            visible: root.provider && root.provider.kinds && root.provider.kinds.length > 1
            SectionLabel { text: qsTr("Authentication") }
            Kit.Dropdown {
                id: methodBox
                Layout.fillWidth: true
                visible: parent.visible
                model: (root.provider && root.provider.kinds ? root.provider.kinds : [])
                       .map(function(k) { return k === "oauth" ? qsTr("Sign in (OAuth)")
                                                              : qsTr("API key"); })
                onActivated: root._syncMethod()
            }
        }

        // API-key fields.
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: root.method === "apikey"

            Kit.TextField {
                id: labelField
                Layout.fillWidth: true
                placeholderText: qsTr("Label (optional)")
            }
            Kit.TextField {
                id: keyField
                Layout.fillWidth: true
                placeholderText: qsTr("API key")
                echoMode: TextInput.Password
            }
            Kit.TextField {
                id: baseUrlField
                Layout.fillWidth: true
                visible: root.provider && root.provider.needsBaseUrl === true
                placeholderText: qsTr("Base URL (e.g. https://…)")
            }
        }

        // OAuth explainer / busy state.
        RowLayout {
            Layout.fillWidth: true
            visible: root.method === "oauth"
            spacing: 10
            QQC.BusyIndicator {
                running: Accounts.busy
                visible: Accounts.busy
                implicitWidth: 22; implicitHeight: 22
            }
            Text {
                Layout.fillWidth: true
                text: Accounts.busy ? qsTr("Waiting for the browser to complete sign-in…")
                                    : qsTr("Opens your browser to sign in securely.")
                wrapMode: Text.WordWrap
                font.family: FontIcons.display; font.pixelSize: 12; color: Theme.textMuted
            }
        }
    }
}
