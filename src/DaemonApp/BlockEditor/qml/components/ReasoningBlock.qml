// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick

import DaemonApp.Theme

// Passive render for a Reasoning block: a collapsible disclosure with a quiet
// "Reasoning" header (chevron + brain glyph + duration) and a markdown body. The
// body auto-opens while the model is still thinking (status === "running") and
// can be toggled by clicking the header. buildReasoningData supplies the scalar
// fields plus the projected `displayMarkup` for the chain-of-thought text.
Item {
    id: root

    property var reasoningData: ({})
    property var editorController: null
    property var blockId: undefined

    readonly property string status: (reasoningData && reasoningData.status) ? reasoningData.status : "complete"
    readonly property string durationLabel: (reasoningData && reasoningData.durationLabel) ? reasoningData.durationLabel : ""
    readonly property string bodyMarkup: (reasoningData && reasoningData.displayMarkup) ? reasoningData.displayMarkup : ""
    readonly property bool running: status === "running"

    // Auto-open while thinking; settle to collapsed once complete unless the user
    // re-opens it. `expanded` is user-togglable via the header.
    property bool expanded: running
    onRunningChanged: if (running) expanded = true

    implicitHeight: card.implicitHeight

    Rectangle {
        id: card
        anchors.left: parent.left
        anchors.right: parent.right
        color: Theme.reasoningSurface
        border.width: Theme.hairline
        border.color: Theme.toolBorder
        radius: Theme.radiusSmall
        implicitHeight: layout.implicitHeight + Theme.smallSpacing * 2

        Column {
            id: layout
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.smallSpacing
            spacing: Theme.smallSpacing

            Item {
                id: header
                width: parent.width
                implicitHeight: headerRow.implicitHeight
                height: implicitHeight

                Row {
                    id: headerRow
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Theme.smallSpacing

                    Text {
                        id: chevron
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.expanded ? FontIcons.fa_chevron_down : FontIcons.fa_chevron_right
                        font.family: FontIcons.faSolid
                        font.pixelSize: Theme.captionFontSize - 2
                        color: Theme.mutedText
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: FontIcons.fa_brain
                        font.family: FontIcons.faSolid
                        font.pixelSize: Theme.captionFontSize
                        color: root.running ? Theme.statusRunning : Theme.mutedText

                        RotationAnimation on rotation {
                            running: root.running
                            from: 0; to: 360
                            duration: 2600
                            loops: Animation.Infinite
                        }
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.running ? qsTr("Reasoning…") : qsTr("Reasoning")
                        color: Theme.reasoningText
                        font.pixelSize: Theme.captionFontSize
                        font.italic: true
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: root.durationLabel.length > 0
                        text: root.durationLabel
                        color: Theme.mutedText
                        font.pixelSize: Theme.captionFontSize - 1
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.expanded = !root.expanded
                }
            }

            Text {
                id: bodyText
                visible: root.expanded && root.bodyMarkup.length > 0
                width: parent.width
                text: root.bodyMarkup
                textFormat: Text.RichText
                color: Theme.reasoningText
                font.family: (root.editorController && root.editorController.bodyFontFamily !== "")
                             ? root.editorController.bodyFontFamily : FontIcons.display
                font.pixelSize: Theme.bodyFontSize
                wrapMode: Text.Wrap
                renderType: Text.QtRendering
            }
        }
    }
}
