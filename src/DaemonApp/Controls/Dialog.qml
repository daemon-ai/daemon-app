// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme

// Themed modal dialog - the kit's replacement for QQC.Dialog + standardButtons,
// which render in the bland Basic style. Provides a token-driven frame, a titled
// header hairline, and a footer built from Kit.TextButton (primary accentFilled,
// cancel flat). Set `destructive` to tint the primary action with the danger
// token. Consumers set `title`, `contentItem`, and handle onAccepted/onRejected
// exactly as before, minus standardButtons.
QQC.Dialog {
    id: root

    // Footer button labels and behavior.
    property string acceptText: qsTr("OK")
    property string rejectText: qsTr("Cancel")
    property bool showCancel: true
    // Tints the primary button with the danger token (delete / clear / reset).
    property bool destructive: false

    modal: true
    // Center over the window overlay by default.
    parent: QQC.Overlay.overlay
    anchors.centerIn: parent

    // Content insets; the header/footer manage their own margins.
    padding: 16

    background: Rectangle {
        color: Theme.background
        border.color: Theme.border
        border.width: 1
        radius: Theme.radius
    }

    header: Item {
        visible: root.title.length > 0
        implicitHeight: root.title.length > 0 ? 46 : 0

        QQC.Label {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            verticalAlignment: Text.AlignVCenter
            text: root.title
            font.family: FontIcons.display
            font.pixelSize: 15
            font.weight: Font.DemiBold
            color: Theme.text
            elide: Text.ElideRight
        }

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: Theme.border
        }
    }

    footer: Item {
        implicitHeight: 56

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: Theme.border
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 8

            Item { Layout.fillWidth: true }

            TextButton {
                text: root.rejectText
                visible: root.showCancel
                onClicked: root.reject()
            }
            TextButton {
                text: root.acceptText
                accentFilled: true
                fillColor: root.destructive ? Theme.danger : Theme.accent
                onClicked: root.accept()
            }
        }
    }
}
