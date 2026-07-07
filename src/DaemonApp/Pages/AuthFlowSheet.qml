// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// The SHARED interactive-auth sheet (A2 + B1): one begin -> browser hand-off -> await-redirect ->
// complete surface over the generic AuthBegin/AuthComplete contract, bound to the shared
// AuthFlow controller (auth::AuthFlowController) — the same seam that serves provider OAuth
// (AuthFlowKind::OAuth2Pkce) and Matrix SSO (AuthFlowKind::MatrixSso). Callers pick the family +
// params and call openFlow(); the sheet renders the controller phases:
//
//   beginning  -> "Preparing sign-in…" spinner
//   challenge  -> the current AuthChallenge, rendered per kind:
//                   redirect -> the authorization URL (open-in-browser + copyable) + a paste field
//                               for the no-loopback path
//                   form     -> the title + a field per formFields, then "Continue" (submitFields)
//                   qr       -> the QR image/payload; auto-polls until the flow completes
//                   message  -> the informational text; auto-polls until the flow completes
//   stepping   -> "Finishing sign-in…" spinner
//   success    -> the resolved account label; auto-dismisses
//   failed/cancelled -> the error + Retry
Kit.Dialog {
    id: root
    title: qsTr("Sign in")
    width: 480
    showCancel: false
    acceptText: qsTr("Close")
    closePolicy: QQC.Popup.CloseOnEscape

    // The flow request openFlow() replays on Retry.
    property string family: ""
    property var params: ({})
    property string bindProfile: ""
    // [wave2:app-channels-liveness] B1: when set (via openFlowForFamily), the idle picker is
    // pre-narrowed to this one family and the provider dropdown is hidden — the caller (the
    // Channels "Connect" button) already knows the family, so the user only fills its params.
    property string forcedFamily: ""
    // The controller's provider rows ({family, flowKind, name, params:[{key,label,required}]}),
    // re-read on providersChanged (a plain Q_INVOKABLE, not a reactive binding).
    property var providerRows: []
    // Idle-state form: the picked provider row index + the entered param values (key -> text).
    property int pickedProvider: 0
    property var paramValues: ({})

    readonly property string phase: (typeof AuthFlow !== "undefined" && AuthFlow)
                                    ? AuthFlow.phase : "idle"
    // The current challenge kind: "" | "redirect" | "form" | "qr" | "message".
    readonly property string challengeKind: (typeof AuthFlow !== "undefined" && AuthFlow)
                                             ? AuthFlow.challengeKind : ""
    readonly property var pickedRow: (providerRows.length > 0
                                      && pickedProvider < providerRows.length)
                                     ? providerRows[pickedProvider] : ({})
    // Values entered into a Form challenge's fields (key -> text).
    property var formValues: ({})

    // Open with the family picked by the CALLER (e.g. a provider row's "Sign in…" button).
    function openFlow(family, params, bindProfile) {
        root.family = family;
        root.params = params || {};
        root.bindProfile = bindProfile || "";
        if (typeof AuthFlow !== "undefined" && AuthFlow)
            AuthFlow.start(family, root.params, root.bindProfile);
        callbackField.text = "";
        open();
    }

    // Open at the family picker (the AccountsPage "Sign in via browser…" entry): the user picks
    // the family and fills its schema-driven params, then Begin starts the flow.
    function openPicker(bindProfile) {
        root.forcedFamily = "";
        root.bindProfile = bindProfile || "";
        if (typeof AuthFlow !== "undefined" && AuthFlow) {
            AuthFlow.reset();
            AuthFlow.refreshProviders();
        }
        root.refreshProviderRows();
        callbackField.text = "";
        open();
    }

    // [wave2:app-channels-liveness] B1: open the picker pre-narrowed to one family (the Channels
    // "Connect" button knows the adapter family). The provider dropdown hides; the family's
    // schema-driven param fields (e.g. Matrix SSO's homeserver) still render. Presentation-only —
    // no AuthFlowController change.
    function openFlowForFamily(family, bindProfile) {
        root.forcedFamily = family || "";
        root.bindProfile = bindProfile || "";
        if (typeof AuthFlow !== "undefined" && AuthFlow) {
            AuthFlow.reset();
            AuthFlow.refreshProviders();
        }
        root.refreshProviderRows();
        for (var i = 0; i < root.providerRows.length; ++i)
            if (root.providerRows[i].family === root.forcedFamily) {
                root.pickedProvider = i;
                break;
            }
        callbackField.text = "";
        open();
    }

    function refreshProviderRows() {
        root.providerRows = (typeof AuthFlow !== "undefined" && AuthFlow) ? AuthFlow.providers()
                                                                          : [];
        // [wave2:app-channels-liveness] keep the pre-narrowed family selected across a providers
        // refresh (the rows can arrive after openFlowForFamily).
        if (root.forcedFamily.length > 0) {
            for (var i = 0; i < root.providerRows.length; ++i)
                if (root.providerRows[i].family === root.forcedFamily) {
                    root.pickedProvider = i;
                    break;
                }
        } else if (root.pickedProvider >= root.providerRows.length) {
            root.pickedProvider = 0;
        }
        root.paramValues = {};
    }

    Connections {
        target: (typeof AuthFlow !== "undefined" && AuthFlow) ? AuthFlow : null
        function onProvidersChanged() {
            root.refreshProviderRows();
        }
    }

    onRejected: {
        if (typeof AuthFlow !== "undefined" && AuthFlow && AuthFlow.active)
            AuthFlow.cancel();
    }
    onAccepted: {
        if (typeof AuthFlow !== "undefined" && AuthFlow && AuthFlow.active)
            AuthFlow.cancel();
    }

    // Auto-open the browser the moment the flow parks, and auto-dismiss shortly after success
    // (the succeeded() signal is the integration seam; the sheet is presentation only).
    Connections {
        target: (typeof AuthFlow !== "undefined" && AuthFlow) ? AuthFlow : null
        function onPhaseChanged() {
            if (AuthFlow.phase === "challenge" && AuthFlow.challengeKind === "redirect")
                AuthFlow.openInBrowser();
            else if (AuthFlow.phase === "success")
                dismissTimer.start();
        }
        function onChallengeChanged() {
            // A fresh Form challenge starts with empty inputs.
            if (AuthFlow.challengeKind === "form")
                root.formValues = {};
        }
    }
    Timer {
        id: dismissTimer
        interval: 1200
        onTriggered: root.close()
    }

    contentItem: ColumnLayout {
        spacing: 12

        // --- idle: family picker + schema-driven params (openPicker() path) ---
        ColumnLayout {
            visible: root.phase === "idle"
            Layout.fillWidth: true
            spacing: 8
            Text {
                visible: root.providerRows.length === 0
                Layout.fillWidth: true
                text: qsTr("No browser sign-in providers are available on this node.")
                wrapMode: Text.WordWrap
                font.family: FontIcons.display; font.pixelSize: 13; color: Theme.textMuted
            }
            SectionLabel {
                visible: root.providerRows.length > 0 && root.forcedFamily.length === 0
                text: qsTr("Provider")
            }
            Kit.Dropdown {
                visible: root.providerRows.length > 0 && root.forcedFamily.length === 0
                Layout.fillWidth: true
                model: root.providerRows.map(function(p) { return p.name; })
                currentIndex: root.pickedProvider
                onActivated: {
                    root.pickedProvider = currentIndex;
                    root.paramValues = {};
                }
            }
            // Schema-driven param fields (e.g. Matrix SSO's homeserver).
            Repeater {
                model: (root.pickedRow && root.pickedRow.params) ? root.pickedRow.params : []
                delegate: ColumnLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: 4
                    SectionLabel {
                        text: modelData.label && modelData.label.length > 0 ? modelData.label
                                                                            : modelData.key
                    }
                    Kit.TextField {
                        Layout.fillWidth: true
                        onTextEdited: {
                            var values = root.paramValues;
                            values[modelData.key] = text;
                            root.paramValues = values;
                        }
                    }
                }
            }
            Kit.TextButton {
                visible: root.providerRows.length > 0
                text: qsTr("Sign in via browser")
                accentFilled: true
                // Required schema params must be filled before Begin.
                enabled: {
                    var fields = (root.pickedRow && root.pickedRow.params)
                                 ? root.pickedRow.params : [];
                    for (var i = 0; i < fields.length; ++i)
                        if (fields[i].required === true) {
                            var v = root.paramValues[fields[i].key];
                            if (!v || String(v).trim().length === 0)
                                return false;
                        }
                    return true;
                }
                onClicked: {
                    root.family = root.pickedRow.family;
                    root.params = root.paramValues;
                    if (typeof AuthFlow !== "undefined" && AuthFlow)
                        AuthFlow.start(root.family, root.params, root.bindProfile);
                }
            }
        }

        // --- beginning / stepping: spinner line ---
        RowLayout {
            visible: root.phase === "beginning" || root.phase === "stepping"
            Layout.fillWidth: true
            spacing: 10
            QQC.BusyIndicator {
                running: visible
                implicitWidth: 22; implicitHeight: 22
            }
            Text {
                Layout.fillWidth: true
                text: root.phase === "beginning" ? qsTr("Preparing sign-in…")
                                                 : qsTr("Finishing sign-in…")
                font.family: FontIcons.display; font.pixelSize: 13; color: Theme.text
            }
        }

        // --- challenge: redirect — URL hand-off + paste fallback ---
        ColumnLayout {
            visible: root.phase === "challenge" && root.challengeKind === "redirect"
            Layout.fillWidth: true
            spacing: 8
            Text {
                Layout.fillWidth: true
                text: (typeof AuthFlow !== "undefined" && AuthFlow && AuthFlow.sinkListening)
                      ? qsTr("Complete the sign-in in your browser. Waiting for it to finish…")
                      : qsTr("Complete the sign-in in your browser, then paste the redirect "
                             + "URL below.")
                wrapMode: Text.WordWrap
                font.family: FontIcons.display; font.pixelSize: 13; color: Theme.text
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                Kit.TextField {
                    id: urlField
                    Layout.fillWidth: true
                    readOnly: true
                    text: (typeof AuthFlow !== "undefined" && AuthFlow)
                          ? AuthFlow.authorizationUrl : ""
                }
                Kit.IconButton {
                    icon: FontIcons.fa_copy
                    iconPointSize: 13
                    tooltipText: qsTr("Copy sign-in link")
                    onClicked: {
                        urlField.selectAll();
                        urlField.copy();
                        urlField.deselect();
                    }
                }
                Kit.TextButton {
                    text: qsTr("Open in browser")
                    onClicked: if (typeof AuthFlow !== "undefined" && AuthFlow)
                                   AuthFlow.openInBrowser()
                }
            }
            // Manual completion (remote daemon / no loopback / copy-paste users).
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                Kit.TextField {
                    id: callbackField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Paste the redirect URL")
                    onAccepted: if (text.trim().length > 0 && typeof AuthFlow !== "undefined"
                                    && AuthFlow)
                                    AuthFlow.submitCallback(text)
                }
                Kit.TextButton {
                    text: qsTr("Complete sign-in")
                    enabled: callbackField.text.trim().length > 0
                    onClicked: if (typeof AuthFlow !== "undefined" && AuthFlow)
                                   AuthFlow.submitCallback(callbackField.text)
                }
            }
        }

        // --- challenge: form — schema-driven inputs, then Continue (submitFields) ---
        ColumnLayout {
            visible: root.phase === "challenge" && root.challengeKind === "form"
            Layout.fillWidth: true
            spacing: 8
            Text {
                Layout.fillWidth: true
                visible: (typeof AuthFlow !== "undefined" && AuthFlow)
                         && AuthFlow.formTitle.length > 0
                text: (typeof AuthFlow !== "undefined" && AuthFlow) ? AuthFlow.formTitle : ""
                wrapMode: Text.WordWrap
                font.family: FontIcons.display; font.pixelSize: 13; color: Theme.text
            }
            Repeater {
                model: (typeof AuthFlow !== "undefined" && AuthFlow) ? AuthFlow.formFields : []
                delegate: ColumnLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: 4
                    SectionLabel {
                        text: modelData.label && modelData.label.length > 0 ? modelData.label
                                                                            : modelData.key
                    }
                    Kit.TextField {
                        Layout.fillWidth: true
                        onTextEdited: {
                            var values = root.formValues;
                            values[modelData.key] = text;
                            root.formValues = values;
                        }
                    }
                }
            }
            Kit.TextButton {
                text: qsTr("Continue")
                accentFilled: true
                // Required fields must be filled before the step is allowed.
                enabled: {
                    var fields = (typeof AuthFlow !== "undefined" && AuthFlow)
                                 ? AuthFlow.formFields : [];
                    for (var i = 0; i < fields.length; ++i)
                        if (fields[i].required === true) {
                            var v = root.formValues[fields[i].key];
                            if (!v || String(v).trim().length === 0)
                                return false;
                        }
                    return true;
                }
                onClicked: if (typeof AuthFlow !== "undefined" && AuthFlow)
                               AuthFlow.submitFields(root.formValues)
            }
        }

        // --- challenge: qr — image/payload; auto-polls until completion ---
        ColumnLayout {
            visible: root.phase === "challenge" && root.challengeKind === "qr"
            Layout.fillWidth: true
            spacing: 8
            Text {
                Layout.fillWidth: true
                text: qsTr("Scan this code with your other device to finish signing in.")
                wrapMode: Text.WordWrap
                font.family: FontIcons.display; font.pixelSize: 13; color: Theme.text
            }
            Image {
                visible: (typeof AuthFlow !== "undefined" && AuthFlow)
                         && AuthFlow.qrImageSource.length > 0
                Layout.alignment: Qt.AlignHCenter
                source: (typeof AuthFlow !== "undefined" && AuthFlow) ? AuthFlow.qrImageSource : ""
                sourceSize.width: 200; sourceSize.height: 200
                fillMode: Image.PreserveAspectFit
            }
            // Fallback when the node sent no pre-rendered image: show the payload to render/scan.
            Kit.TextField {
                visible: (typeof AuthFlow !== "undefined" && AuthFlow)
                         && AuthFlow.qrImageSource.length === 0
                Layout.fillWidth: true
                readOnly: true
                text: (typeof AuthFlow !== "undefined" && AuthFlow) ? AuthFlow.qrPayload : ""
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                QQC.BusyIndicator {
                    running: visible
                    implicitWidth: 18; implicitHeight: 18
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Waiting for approval…")
                    font.family: FontIcons.display; font.pixelSize: 12; color: Theme.textMuted
                }
            }
        }

        // --- challenge: message — informational text; auto-polls until completion ---
        RowLayout {
            visible: root.phase === "challenge" && root.challengeKind === "message"
            Layout.fillWidth: true
            spacing: 10
            QQC.BusyIndicator {
                running: visible
                implicitWidth: 22; implicitHeight: 22
            }
            Text {
                Layout.fillWidth: true
                text: (typeof AuthFlow !== "undefined" && AuthFlow) ? AuthFlow.messageText : ""
                wrapMode: Text.WordWrap
                font.family: FontIcons.display; font.pixelSize: 13; color: Theme.text
            }
        }

        // --- success ---
        Text {
            visible: root.phase === "success"
            Layout.fillWidth: true
            text: qsTr("Signed in as %1")
                  .arg((typeof AuthFlow !== "undefined" && AuthFlow) ? AuthFlow.accountLabel : "")
            wrapMode: Text.WordWrap
            font.family: FontIcons.display; font.pixelSize: 13; color: Theme.stateRunning
        }

        // --- failed / cancelled ---
        ColumnLayout {
            visible: root.phase === "failed" || root.phase === "cancelled"
            Layout.fillWidth: true
            spacing: 8
            Text {
                Layout.fillWidth: true
                text: root.phase === "cancelled"
                      ? qsTr("Sign-in cancelled")
                      : ((typeof AuthFlow !== "undefined" && AuthFlow && AuthFlow.error.length > 0)
                         ? AuthFlow.error : qsTr("Sign-in failed"))
                wrapMode: Text.WordWrap
                font.family: FontIcons.display; font.pixelSize: 13; color: Theme.danger
            }
            Kit.TextButton {
                text: qsTr("Retry")
                accentFilled: true
                onClicked: if (typeof AuthFlow !== "undefined" && AuthFlow)
                               AuthFlow.start(root.family, root.params, root.bindProfile)
            }
        }
    }
}
