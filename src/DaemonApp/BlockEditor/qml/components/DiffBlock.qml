// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick

import DaemonApp.Theme

// Renders a unified diff. The C++ side (be::parseUnifiedDiff, surfaced as
// editorController.parseDiff) types each line (add/del/hunk/meta/context); this
// paints a per-line wash and color, monospace, GitHub-style.
Item {
    id: root

    property var lines: []

    implicitHeight: col.implicitHeight

    Column {
        id: col
        anchors.left: parent.left
        anchors.right: parent.right

        Repeater {
            model: root.lines
            delegate: Rectangle {
                required property var modelData
                width: col.width
                implicitHeight: lineText.implicitHeight
                height: implicitHeight
                color: modelData.kind === "add" ? Theme.diffAddBackground
                     : modelData.kind === "del" ? Theme.diffDelBackground
                     : Theme.transparent

                Text {
                    id: lineText
                    anchors.left: parent.left
                    anchors.right: parent.right
                    leftPadding: Theme.smallSpacing
                    rightPadding: Theme.smallSpacing
                    text: modelData.text.length > 0 ? modelData.text : " "
                    textFormat: Text.PlainText
                    color: modelData.kind === "add" ? Theme.diffAddText
                         : modelData.kind === "del" ? Theme.diffDelText
                         : modelData.kind === "hunk" ? Theme.diffHunkText
                         : modelData.kind === "meta" ? Theme.mutedText
                         : Theme.text
                    font.family: FontIcons.mono
                    font.pixelSize: Theme.bodyFontSize - 1
                    wrapMode: Text.NoWrap
                }
            }
        }
    }
}
