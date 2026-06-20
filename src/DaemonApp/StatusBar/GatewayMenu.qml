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

    // "ready" | "needs setup" | "checking" | "connecting" | "offline".
    property string gatewayState: "ready"
    property string connectionText: qsTr("Connected to local gateway")
    property var logLines: [
        "gateway: ready (pid 4821)",
        "session: resumed conv-1",
        "inference: model warm",
        "messaging: telegram connected",
        "cron: 0 jobs due"
    ]
    property var platforms: [
        { name: "Telegram", online: true },
        { name: "Slack", online: false },
        { name: "Email", online: true }
    ]

    readonly property bool degraded: gatewayState === "needs setup" || gatewayState === "connecting"
                                     || gatewayState === "checking"
    readonly property bool offline: gatewayState === "offline"

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
                color: root.offline ? Theme.danger : root.degraded ? Theme.warning : "#3fbf63"
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
            text: root.connectionText
            font.family: FontIcons.display
            font.pixelSize: 12
            color: Theme.textMuted
            wrapMode: Text.WordWrap
        }

        // ----- Log lines -------------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            spacing: 2

            Repeater {
                model: root.logLines
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

        Rectangle { Layout.fillWidth: true; Layout.topMargin: 6; height: 1; color: Theme.border }

        // ----- Messaging platforms ---------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 6

            Repeater {
                model: root.platforms
                delegate: RowLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: 8

                    Rectangle {
                        width: 7; height: 7; radius: 3.5
                        Layout.alignment: Qt.AlignVCenter
                        color: modelData.online ? "#3fbf63" : Theme.danger
                    }
                    QQC.Label {
                        text: modelData.name
                        font.family: FontIcons.display
                        font.pixelSize: 12
                        color: Theme.text
                        Layout.fillWidth: true
                    }
                    QQC.Label {
                        text: modelData.online ? qsTr("connected") : qsTr("offline")
                        font.family: FontIcons.display
                        font.pixelSize: 11
                        color: Theme.textMuted
                    }
                }
            }
        }
    }
}
