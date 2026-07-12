// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick

import DaemonApp.Theme

// Process notification (inventory #25): a centered notice with a terminal glyph,
// a headline, and a collapsible <details>-style body for the captured output.
// Detected from a system message whose raw text starts with "process:"; the
// first line after the prefix is the headline, the remainder is the detail.
Item {
    id: root

    property string markdown: ""
    property var editorController: null

    readonly property string _raw: markdown.trim()
    readonly property string _body: _raw.startsWith("process:") ? _raw.substring(8).trim() : _raw
    readonly property string headline: {
        const nl = _body.indexOf("\n")
        return nl < 0 ? _body : _body.substring(0, nl)
    }
    readonly property string detail: {
        const nl = _body.indexOf("\n")
        return nl < 0 ? "" : _body.substring(nl + 1).trim()
    }

    property bool expanded: false

    implicitHeight: pill.implicitHeight

    Rectangle {
        id: pill
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width, 560)
        radius: Theme.radiusSmall
        color: Theme.processNoticeSurface
        border.width: Theme.hairline
        border.color: Theme.processNoticeBorder
        implicitHeight: column.implicitHeight + Theme.smallSpacing * 2

        Column {
            id: column
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.smallSpacing
            spacing: Theme.smallSpacing

            Item {
                width: parent.width
                implicitHeight: headerRow.implicitHeight

                Row {
                    id: headerRow
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: Theme.smallSpacing

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: FontIcons.fa_terminal
                        font.family: FontIcons.faSolid
                        font.pixelSize: Theme.captionFontSize
                        color: Theme.processNoticeIcon
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.headline
                        color: Theme.systemNoticeText
                        font.family: (root.editorController && root.editorController.bodyFontFamily !== "")
                            ? root.editorController.bodyFontFamily : FontIcons.display
                        font.pixelSize: Theme.captionFontSize
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: root.detail.length > 0
                        text: root.expanded ? FontIcons.fa_chevron_down : FontIcons.fa_chevron_right
                        font.family: FontIcons.faSolid
                        font.pixelSize: Theme.captionFontSize - 3
                        color: Theme.systemNoticeText
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: root.detail.length > 0
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.expanded = !root.expanded

                    // Disclosure toggle for the notice detail.
                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Toggle details")
                    Accessible.onPressAction: root.expanded = !root.expanded
                }
            }

            Text {
                width: parent.width
                visible: root.expanded && root.detail.length > 0
                text: root.detail
                color: Theme.systemNoticeText
                font.family: FontIcons.mono
                font.pixelSize: Theme.captionFontSize - 1
                wrapMode: Text.Wrap
                renderType: Text.QtRendering
            }
        }
    }
}
