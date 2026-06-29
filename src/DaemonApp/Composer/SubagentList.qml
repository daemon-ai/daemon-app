// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme

// Live subagent/delegation rows docked above the composer - the QML stand-in for
// Hermes' composer status-stack subagent rows. Backed by a SubagentModel of
// { subId, title, status, detail } owned by the orchestrator and fed by the
// turn's subagent.* events; with no agent backend the demo turn drives it.
ColumnLayout {
    id: root

    property var model: null

    spacing: 2

    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 4
        Layout.rightMargin: 4
        Layout.bottomMargin: 2
        spacing: 6

        Text {
            text: FontIcons.fa_sitemap
            font.family: FontIcons.faSolid
            font.pixelSize: 10
            color: Theme.textMuted
        }
        QQC.Label {
            Layout.fillWidth: true
            text: qsTr("Subagents")
            font.family: FontIcons.display
            font.pixelSize: Theme.labelSize
            font.weight: Font.DemiBold
            font.letterSpacing: Theme.labelTracking
            font.capitalization: Font.AllUppercase
            color: Theme.textMuted
        }
    }

    Repeater {
        model: root.model
        delegate: RowLayout {
            id: rowRoot
            required property int index
            required property string title
            required property string status
            required property string detail
            Layout.fillWidth: true
            Layout.leftMargin: 6
            Layout.rightMargin: 6
            spacing: 8

            readonly property bool running: rowRoot.status === "running"
            readonly property bool errored: rowRoot.status === "error"

            Text {
                Layout.alignment: Qt.AlignVCenter
                text: rowRoot.errored ? FontIcons.fa_robot
                    : (rowRoot.running ? FontIcons.fa_spinner : FontIcons.fa_circle_check)
                font.family: FontIcons.faSolid
                font.pixelSize: 12
                color: rowRoot.errored ? Theme.statusError
                    : (rowRoot.running ? Theme.accent : Theme.statusOk)

                // Spin while the subagent is in flight.
                RotationAnimation on rotation {
                    running: rowRoot.running
                    from: 0
                    to: 360
                    loops: Animation.Infinite
                    duration: 900
                }
            }
            QQC.Label {
                Layout.topMargin: 1
                Layout.bottomMargin: 1
                text: rowRoot.title
                font.family: FontIcons.display
                font.pixelSize: 12
                font.weight: Font.Medium
                color: Theme.text
            }
            QQC.Label {
                Layout.fillWidth: true
                Layout.topMargin: 1
                Layout.bottomMargin: 1
                text: rowRoot.detail
                font.family: FontIcons.display
                font.pixelSize: 11
                color: Theme.textMuted
                elide: Text.ElideRight
            }
        }
    }
}
