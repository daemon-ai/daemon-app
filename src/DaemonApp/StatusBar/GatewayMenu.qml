// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Upward dropdown opened from the Gateway status-bar item - the native port of
// GatewayMenuPanel (w-72, side="top"). Header with a status dot, the
// connection line, the last few gateway log lines, a "view all logs" affordance,
// and the messaging-platform rows. Content is placeholder until wired to the
// daemon backend.
QQC.Popup {
    id: root

    // The shared StatusBarModel owns the gateway state + dropdown content (state,
    // connection line, log lines, messaging platforms, and the degraded/offline
    // derivations), so both front ends read one source.
    required property var statusModel

    readonly property bool degraded: root.statusModel ? root.statusModel.gatewayDegraded : false
    readonly property bool offline: root.statusModel ? root.statusModel.gatewayOffline : false

    signal openSystemRequested()
    signal viewAllLogsRequested()

    width: 288
    padding: 0
    modal: false
    focus: true

    background: Rectangle {
        color: Theme.background
        border.color: Theme.border
        border.width: 1
        radius: 10
    }

    contentItem: ColumnLayout {
        spacing: 0

        // ----- Header ----------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            Layout.bottomMargin: 8
            spacing: 8

            Rectangle {
                width: 8; height: 8; radius: 4
                Layout.alignment: Qt.AlignVCenter
                color: root.offline ? Theme.danger : root.degraded ? Theme.warning : Theme.statusOk
            }
            QQC.Label {
                text: qsTr("Gateway")
                font.family: FontIcons.display
                font.pixelSize: 13
                font.weight: Font.DemiBold
                color: Theme.text
                Layout.fillWidth: true
            }
            Kit.TextButton {
                text: qsTr("Open system")
                onClicked: { root.openSystemRequested(); root.close(); }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

        // ----- Connection line -------------------------------------------
        QQC.Label {
            Layout.fillWidth: true
            Layout.margins: 12
            Layout.bottomMargin: 6
            text: root.statusModel ? root.statusModel.gatewayConnectionText : ""
            font.family: FontIcons.display
            font.pixelSize: 12
            color: Theme.textMuted
            wrapMode: Text.WordWrap
        }

        // ----- OpenAI-compatible gateway ---------------------------------
        // Phase F: the node's resident OpenAI-gateway status (distinct from the connection line
        // above), fed from GatewayRepository via the shared StatusBarModel.
        RowLayout {
            visible: root.statusModel ? root.statusModel.openAiGatewaySupported : false
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.bottomMargin: 6
            spacing: 8

            Rectangle {
                width: 7; height: 7; radius: 3.5
                Layout.alignment: Qt.AlignVCenter
                color: !root.statusModel || !root.statusModel.openAiGatewayEnabled ? Theme.textMuted
                     : root.statusModel.openAiGatewayListening ? Theme.statusOk
                     : Theme.warning
            }
            QQC.Label {
                Layout.fillWidth: true
                text: qsTr("OpenAI gateway: %1")
                    .arg(root.statusModel ? root.statusModel.openAiGatewayStatusText : "")
                font.family: FontIcons.display
                font.pixelSize: 12
                color: Theme.textMuted
                elide: Text.ElideRight
            }
        }

        // ----- Log lines -------------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            spacing: 2

            Repeater {
                model: root.statusModel ? root.statusModel.gatewayLog : []
                delegate: QQC.Label {
                    required property string modelData
                    Layout.fillWidth: true
                    text: modelData
                    font.family: FontIcons.mono
                    font.pixelSize: 11
                    color: Theme.textMuted
                    elide: Text.ElideRight
                }
            }
        }

        Kit.TextButton {
            Layout.leftMargin: 8
            Layout.topMargin: 4
            text: qsTr("View all logs")
            onClicked: { root.viewAllLogsRequested(); root.close(); }
        }

        Rectangle {
            visible: root.statusModel ? root.statusModel.healthServices.length > 0 : false
            Layout.fillWidth: true; Layout.topMargin: 6; height: 1; color: Theme.border
        }

        // ----- Node service health ---------------------------------------
        // Phase F: the node's per-service health (gateway, local-inference, ...), fed from the
        // single DaemonConnectionService health cache via the shared StatusBarModel.
        ColumnLayout {
            visible: root.statusModel ? root.statusModel.healthServices.length > 0 : false
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 6

            Repeater {
                model: root.statusModel ? root.statusModel.healthServices : []
                delegate: RowLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: 8

                    Rectangle {
                        width: 7; height: 7; radius: 3.5
                        Layout.alignment: Qt.AlignVCenter
                        color: modelData.ok ? Theme.statusOk : Theme.danger
                    }
                    QQC.Label {
                        text: modelData.name
                        font.family: FontIcons.display
                        font.pixelSize: 12
                        color: Theme.text
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: modelData.ok ? qsTr("ok") : qsTr("down")
                        font.family: FontIcons.display
                        font.pixelSize: 11
                        color: Theme.textMuted
                    }
                }
            }
        }
    }
}
