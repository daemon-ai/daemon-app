// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick

import DaemonApp.Theme

// Passive render for a ToolCall block: a card with a status-aware header (status
// glyph, tone glyph, title, args subtitle, duration, copy) over a collapsible
// body that routes to a sub-renderer by detailKind (ansi-stream -> AnsiText,
// diff -> DiffBlock, search-results -> SearchResultsList, image -> ImageBlock,
// else plain mono text). The tone -> glyph map mirrors Hermes' ChainToolFallback
// (generic for now). buildToolData supplies the computed view fields.
Item {
    id: root

    property var toolData: ({})
    property var editorController: null
    property var blockId: undefined

    readonly property string title: (toolData && toolData.title) ? toolData.title : qsTr("Tool")
    readonly property string subtitle: (toolData && toolData.subtitle) ? toolData.subtitle : ""
    readonly property string status: (toolData && toolData.status) ? toolData.status : "ok"
    readonly property string tone: (toolData && toolData.tone) ? toolData.tone : "tool"
    readonly property string durationLabel: (toolData && toolData.durationLabel) ? toolData.durationLabel : ""
    readonly property string detailKind: (toolData && toolData.detailKind) ? toolData.detailKind : ""
    readonly property string variant: (toolData && toolData.variant) ? toolData.variant : "generic"
    readonly property bool awaitingApproval: !!(toolData && toolData.awaitingApproval)
    readonly property bool running: status === "running"

    property bool expanded: true

    readonly property string statusGlyph: running ? FontIcons.fa_spinner
                                         : status === "error" ? FontIcons.fa_circle_exclamation
                                         : FontIcons.fa_circle_check
    readonly property color statusColor: running ? Theme.statusRunning
                                        : status === "error" ? Theme.statusError
                                        : Theme.statusOk

    function toneGlyph(t) {
        switch (t) {
        case "terminal": return FontIcons.fa_terminal
        case "search":
        case "web": return FontIcons.fa_globe
        case "edit": return FontIcons.fa_pen_to_square
        case "code": return FontIcons.fa_code
        case "image": return FontIcons.fa_image
        case "agent": return FontIcons.fa_circle_question
        default: return FontIcons.fa_wrench
        }
    }

    // Detail payload resolution per kind.
    readonly property string ansiText: {
        if (!toolData)
            return ""
        const out = toolData.stdout ? String(toolData.stdout) : ""
        const err = toolData.stderr ? String(toolData.stderr) : ""
        if (out.length > 0 && err.length > 0)
            return out + "\n" + err
        if (out.length > 0)
            return out
        if (err.length > 0)
            return err
        return toolData.body ? String(toolData.body) : ""
    }
    // The clarify variant always shows its interactive panel; otherwise a detail
    // body appears only when a sub-renderer has a payload to draw.
    readonly property bool hasDetail: variant === "clarify"
        || !!(detailKind.length > 0
            && (ansiText.length > 0
                || (toolData && toolData.diff)
                || (toolData && toolData.hits)
                || (toolData && toolData.imageUrl)
                || (toolData && toolData.body)))

    implicitHeight: card.implicitHeight

    Rectangle {
        id: card
        anchors.left: parent.left
        anchors.right: parent.right
        color: Theme.toolSurface
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
                implicitHeight: Math.max(headerRow.implicitHeight, copyButton.implicitHeight)
                height: implicitHeight

                Row {
                    id: headerRow
                    anchors.left: parent.left
                    anchors.right: copyButton.left
                    anchors.rightMargin: Theme.smallSpacing
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Theme.smallSpacing

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.expanded ? FontIcons.fa_chevron_down : FontIcons.fa_chevron_right
                        font.family: FontIcons.faSolid
                        font.pixelSize: Theme.captionFontSize - 2
                        color: Theme.mutedText
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.statusGlyph
                        font.family: FontIcons.faSolid
                        font.pixelSize: Theme.captionFontSize
                        color: root.statusColor

                        RotationAnimation on rotation {
                            running: root.running
                            from: 0; to: 360
                            duration: 900
                            loops: Animation.Infinite
                        }
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.toneGlyph(root.tone)
                        font.family: FontIcons.faSolid
                        font.pixelSize: Theme.captionFontSize
                        color: Theme.mutedText
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.title
                        color: Theme.text
                        font.pixelSize: Theme.bodyFontSize - 1
                        font.bold: true
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: root.subtitle.length > 0
                        width: Math.min(implicitWidth, headerRow.width / 2)
                        text: root.subtitle
                        color: Theme.mutedText
                        font.family: FontIcons.mono
                        font.pixelSize: Theme.captionFontSize
                        elide: Text.ElideRight
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: root.durationLabel.length > 0
                        text: root.durationLabel
                        color: Theme.mutedText
                        font.pixelSize: Theme.captionFontSize - 1
                    }
                }

                Text {
                    id: copyButton
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: FontIcons.fa_copy
                    font.family: FontIcons.faSolid
                    font.pixelSize: Theme.captionFontSize
                    color: copyHover.hovered ? Theme.text : Theme.mutedText

                    HoverHandler { id: copyHover }
                    TapHandler {
                        onTapped: {
                            const payload = root.subtitle.length > 0 ? root.subtitle : root.ansiText
                            // Route through the controller so the wasm insecure-context fallback +
                            // toast apply (desktop still lands the same text on the clipboard). Fall
                            // back to the off-screen TextEdit only if no controller is wired.
                            if (root.editorController) {
                                root.editorController.copyText(payload)
                            } else {
                                clip.text = payload
                                clip.selectAll()
                                clip.copy()
                            }
                        }
                    }
                }

                // Header click (outside the copy button) toggles the body.
                MouseArea {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: copyButton.left
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.expanded = !root.expanded
                }
            }

            // Pending-approval bar for dangerous tools, above the body. Shown
            // whenever the tool is awaiting a decision, independent of expansion.
            Loader {
                id: approvalLoader
                width: parent.width
                active: root.awaitingApproval
                visible: active
                sourceComponent: approvalComponent
            }

            Loader {
                id: detailLoader
                width: parent.width
                active: root.expanded && root.hasDetail
                visible: active
                sourceComponent: {
                    if (root.variant === "clarify")
                        return clarifyComponent
                    switch (root.detailKind) {
                    case "diff": return diffComponent
                    case "search-results": return searchComponent
                    case "generated-image": return generatedImageComponent
                    case "image": return imageComponent
                    case "ansi-stream":
                    case "pty": return ansiComponent
                    default: return textComponent
                    }
                }
            }
        }
    }

    // Off-screen helper that owns clipboard access for the copy button.
    TextEdit {
        id: clip
        visible: false
        width: 0
        height: 0
    }

    Component {
        id: ansiComponent
        AnsiText {
            width: detailLoader.width
            spans: root.editorController ? root.editorController.ansiSpans(root.ansiText) : []
        }
    }

    Component {
        id: diffComponent
        DiffBlock {
            width: detailLoader.width
            lines: root.editorController ? root.editorController.parseDiff(root.toolData.diff ? String(root.toolData.diff) : "") : []
        }
    }

    Component {
        id: searchComponent
        SearchResultsList {
            width: detailLoader.width
            hits: (root.toolData && root.toolData.hits) ? root.toolData.hits : []
        }
    }

    Component {
        id: imageComponent
        ImageBlock {
            width: detailLoader.width
            imageData: ({
                url: (root.toolData && root.toolData.imageUrl) ? root.toolData.imageUrl : "",
                source: (root.toolData && root.toolData.imageUrl) ? root.toolData.imageUrl : "",
                isRemote: true
            })
            editorController: root.editorController
            blockId: root.blockId
        }
    }

    Component {
        id: textComponent
        Text {
            width: detailLoader.width
            text: (root.toolData && root.toolData.body) ? String(root.toolData.body) : root.ansiText
            textFormat: Text.PlainText
            color: Theme.text
            font.family: FontIcons.mono
            font.pixelSize: Theme.bodyFontSize - 1
            wrapMode: Text.Wrap
        }
    }

    Component {
        id: generatedImageComponent
        GeneratedImageBlock {
            width: detailLoader.width
            toolData: root.toolData
            editorController: root.editorController
            blockId: root.blockId
        }
    }

    Component {
        id: clarifyComponent
        ClarifyBlock {
            width: detailLoader.width
            toolData: root.toolData
            editorController: root.editorController
            blockId: root.blockId
        }
    }

    Component {
        id: approvalComponent
        ToolApprovalBar {
            width: approvalLoader.width
            toolData: root.toolData
            editorController: root.editorController
            blockId: root.blockId
        }
    }
}
