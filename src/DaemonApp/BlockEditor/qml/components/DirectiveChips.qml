// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick

import DaemonApp.Theme

// User attachment row + directive chips (inventory #22). Parses a user message's
// raw text for @-mentions (@file: / @folder: / @url: / @image: / @tool: / @line:
// / @terminal: / @session:) and renders a compact chip per mention, with an
// image-directive chip showing a small thumbnail when the source resolves.
Flow {
    id: root

    property string markdown: ""
    spacing: Theme.smallSpacing

    // Parsed mentions: [{ type, label, id }]. Rebuilt whenever the text changes.
    readonly property var mentions: {
        const out = []
        const re = /@(file|folder|url|image|tool|line|terminal|session):([^\s]+)/g
        let m
        while ((m = re.exec(markdown)) !== null) {
            const type = m[1]
            const id = m[2]
            // Label is the trailing path/name segment, kept short.
            const segs = id.split("/")
            let label = segs[segs.length - 1] || id
            if (label.length > 28)
                label = label.substring(0, 27) + "\u2026"
            out.push({ type: type, label: label, id: id })
        }
        return out
    }

    visible: mentions.length > 0

    function glyphFor(type) {
        switch (type) {
        case "file": return FontIcons.fa_code
        case "folder": return FontIcons.fa_folder
        case "url": return FontIcons.fa_link
        case "image": return FontIcons.fa_image
        case "tool": return FontIcons.fa_wrench
        case "line": return FontIcons.fa_indent
        case "terminal": return FontIcons.fa_terminal
        case "session": return FontIcons.fa_comments
        }
        return FontIcons.fa_tag
    }

    Repeater {
        model: root.mentions

        delegate: Rectangle {
            id: chip
            required property var modelData
            readonly property bool isImage: modelData.type === "image"

            radius: Theme.radius
            color: Theme.toolHeader
            border.width: Theme.hairline
            border.color: Theme.toolBorder
            implicitHeight: Math.max(chipRow.implicitHeight + 4, thumb.visible ? thumb.height + 4 : 0)
            implicitWidth: chipRow.implicitWidth + Theme.smallSpacing * 2

            Row {
                id: chipRow
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: Theme.smallSpacing
                spacing: 4

                Image {
                    id: thumb
                    anchors.verticalCenter: parent.verticalCenter
                    visible: chip.isImage && status === Image.Ready
                    source: chip.isImage ? chip.modelData.id : ""
                    sourceSize.height: 18
                    fillMode: Image.PreserveAspectFit
                    height: 18
                    width: visible ? Math.min(implicitWidth, 28) : 0
                    asynchronous: true
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: !thumb.visible
                    text: root.glyphFor(chip.modelData.type)
                    font.family: FontIcons.faSolid
                    font.pixelSize: Theme.captionFontSize - 2
                    color: Theme.mutedText
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: chip.modelData.label
                    color: Theme.text
                    font.family: FontIcons.mono
                    font.pixelSize: Theme.captionFontSize - 1
                }
            }
        }
    }
}
