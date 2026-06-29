// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// The shared connection chooser, reused by the first-run gate and by
// Settings -> Connection. Mode cards (embedded[disabled] / local / remote), a
// target field (socket path or URL), an optional auth token, a Test-connection
// probe, and Connect. Writes through the Connection seam (mock now) and persists
// the choice via AppSettings so the next boot reuses it.
ColumnLayout {
    id: root
    spacing: 16

    // When false, only the Test button is shown (the host drives Connect itself).
    property bool showConnect: true
    signal connectRequested(string mode, string target, string token)

    property string mode: Connection ? Connection.mode : "local"
    property string testMessage: ""
    property bool testOk: false

    function _placeholder() {
        return root.mode === "remote" ? qsTr("https://node.example:8080")
                                      : qsTr("/run/daemon/daemon.sock");
    }

    Connections {
        target: Connection
        function onTestResult(ok, message) { root.testOk = ok; root.testMessage = message; }
    }

    SectionLabel { text: qsTr("Transport") }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        Repeater {
            model: [
                { id: "embedded", title: qsTr("Embedded"), desc: qsTr("In-process node (coming soon)"), enabled: false },
                { id: "local",    title: qsTr("Local"),    desc: qsTr("Unix socket on this machine"), enabled: true },
                { id: "remote",   title: qsTr("Remote"),   desc: qsTr("Connect to a node over the network"), enabled: true },
            ]
            delegate: Rectangle {
                id: cardRoot
                required property var modelData
                Layout.fillWidth: true
                Layout.preferredHeight: 78
                radius: 10
                readonly property bool isSel: root.mode === modelData.id
                color: isSel ? Theme.hover : Theme.surface
                opacity: modelData.enabled ? 1.0 : 0.45
                border.width: isSel ? 2 : 1
                border.color: isSel ? Theme.accent : Theme.border

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 3
                    Text {
                        text: cardRoot.modelData.title
                        font.family: FontIcons.display; font.pixelSize: 14
                        font.weight: Font.DemiBold; color: Theme.text
                    }
                    Text {
                        text: cardRoot.modelData.desc
                        font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
                        wrapMode: Text.WordWrap; Layout.fillWidth: true
                    }
                }
                TapHandler {
                    enabled: cardRoot.modelData.enabled
                    onTapped: { root.mode = cardRoot.modelData.id; root.testMessage = ""; }
                }
            }
        }
    }

    SectionLabel { text: qsTr("Target"); visible: root.mode !== "embedded" }

    Kit.TextField {
        id: targetField
        Layout.fillWidth: true
        visible: root.mode !== "embedded"
        placeholderText: root._placeholder()
        // Two-way with the connection seam: seed from the active target and follow
        // external changes (connect/disconnect) while the user isn't editing, but
        // let typed edits stand (they drive the next connect/test). On first run the
        // seam has no target yet, so fall back to the resolved local target (which
        // honors DAEMON_APP_SOCKET / the saved target) instead of leaving it blank.
        Component.onCompleted: {
            var t = Connection ? Connection.target : "";
            if (!t && typeof AppSettings !== "undefined")
                t = AppSettings.resolvedConnectionTarget();
            text = t;
        }
        Connections {
            target: Connection
            function onConfigChanged() {
                if (!targetField.activeFocus)
                    targetField.text = Connection.target;
            }
        }
    }

    Kit.TextField {
        id: tokenField
        Layout.fillWidth: true
        visible: root.mode === "remote"
        placeholderText: qsTr("Auth token (optional)")
        echoMode: TextInput.Password
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        Kit.TextButton {
            text: Connection && Connection.testing ? qsTr("Testing...") : qsTr("Test connection")
            enabled: root.mode !== "embedded" && targetField.text.length > 0
                     && !(Connection && Connection.testing)
            onClicked: Connection.testConnection(root.mode, targetField.text, tokenField.text)
        }

        Item { Layout.fillWidth: true }

        Kit.TextButton {
            visible: root.showConnect
            text: qsTr("Connect")
            accentFilled: true
            enabled: root.mode !== "embedded" && targetField.text.length > 0
            onClicked: {
                AppSettings.setLastConnection(root.mode, targetField.text);
                Connection.connectTo(root.mode, targetField.text, tokenField.text);
                root.connectRequested(root.mode, targetField.text, tokenField.text);
            }
        }
    }

    Text {
        visible: root.testMessage.length > 0
        text: (root.testOk ? FontIcons.fa_circle_check + "  " : FontIcons.fa_circle_xmark + "  ") + root.testMessage
        font.family: FontIcons.display; font.pixelSize: 12
        color: root.testOk ? Theme.accent : Theme.danger
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
    }
}
