// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick

import DaemonApp.Theme

// Centered system notice (inventory #24): a quiet, full-width-but-narrow chrome
// strip for steer notes, slash-command status, and generic system text. The
// variant is detected from a leading "steer:" / "slash:" prefix on the raw
// markdown (mirroring Hermes' SystemMessage), otherwise it renders as generic
// centered text. Notices are passive (no cross-block selection / link routing).
Item {
    id: root

    property string markdown: ""
    property var editorController: null

    readonly property string _raw: markdown.trim()
    readonly property bool isSteer: _raw.startsWith("steer:")
    readonly property bool isSlash: _raw.startsWith("slash:")

    // steer: a labeled steering note. slash: first line is the command, the rest
    // is its output. generic: the text verbatim.
    readonly property string steerBody: isSteer ? _raw.substring(6).trim() : ""
    readonly property string slashRest: isSlash ? _raw.substring(6).trim() : ""
    readonly property string slashCommand: {
        if (!isSlash)
            return ""
        const nl = slashRest.indexOf("\n")
        return nl < 0 ? slashRest : slashRest.substring(0, nl)
    }
    readonly property string slashOutput: {
        if (!isSlash)
            return ""
        const nl = slashRest.indexOf("\n")
        return nl < 0 ? "" : slashRest.substring(nl + 1).trim()
    }

    readonly property string bodyFamily: (editorController && editorController.bodyFontFamily !== "")
        ? editorController.bodyFontFamily : FontIcons.display

    implicitHeight: pill.implicitHeight

    Rectangle {
        id: pill
        // Centered and narrower than the column so it reads as chrome, not content.
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width, 560)
        radius: Theme.radiusSmall
        color: Theme.systemNoticeSurface
        border.width: Theme.hairline
        border.color: Theme.systemNoticeBorder
        implicitHeight: column.implicitHeight + Theme.smallSpacing * 2

        Column {
            id: column
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.smallSpacing
            spacing: 2

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Theme.smallSpacing
                visible: root.isSteer || root.isSlash

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.isSlash ? FontIcons.fa_terminal : FontIcons.fa_circle_info
                    font.family: FontIcons.faSolid
                    font.pixelSize: Theme.captionFontSize - 1
                    color: Theme.systemNoticeText
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.isSlash ? root.slashCommand
                                       : (root.isSteer ? qsTr("Steering note") : "")
                    color: Theme.systemNoticeText
                    font.family: root.bodyFamily
                    font.pixelSize: Theme.captionFontSize
                    font.bold: true
                }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: root.isSteer ? root.steerBody
                                   : (root.isSlash ? root.slashOutput : root._raw)
                visible: text.length > 0
                color: Theme.systemNoticeText
                font.family: root.bodyFamily
                font.pixelSize: Theme.captionFontSize
                wrapMode: Text.Wrap
                renderType: Text.QtRendering
            }
        }
    }
}
