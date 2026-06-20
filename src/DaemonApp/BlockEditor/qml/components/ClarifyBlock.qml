import QtQuick

import DaemonApp.Theme

// Interactive clarify panel (Hermes inventory #7): a question with optional
// multiple-choice buttons plus a freeform answer field and a skip action. Once
// answered it collapses to a compact resolved row (the agent host then drives
// the underlying tool to completion). Answers are routed to the host through
// editorController.answerClarify.
Item {
    id: root

    property var toolData: ({})
    property var editorController: null
    property var blockId: undefined

    readonly property string question: (toolData && toolData.question) ? String(toolData.question) : qsTr("The agent needs your input.")
    readonly property string requestId: (toolData && toolData.requestId) ? String(toolData.requestId) : ""
    readonly property var choices: (toolData && toolData.choices) ? toolData.choices : []
    readonly property bool answered: !!(toolData && toolData.answered)
    readonly property string answer: (toolData && toolData.answer) ? String(toolData.answer) : ""

    function submit(value) {
        const text = String(value).trim()
        if (text.length === 0)
            return
        if (root.editorController && root.blockId !== undefined)
            root.editorController.answerClarify(Number(root.blockId), root.requestId, text)
    }

    implicitHeight: answered ? resolved.implicitHeight : panel.implicitHeight

    // Compact resolved row once the user has answered.
    Row {
        id: resolved
        visible: root.answered
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.smallSpacing

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: FontIcons.fa_circle_check
            font.family: FontIcons.faSolid
            font.pixelSize: Theme.captionFontSize
            color: Theme.statusOk
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("You answered:")
            color: Theme.mutedText
            font.pixelSize: Theme.captionFontSize
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(implicitWidth, resolved.width / 2)
            text: root.answer
            color: Theme.text
            font.pixelSize: Theme.captionFontSize
            font.bold: true
            elide: Text.ElideRight
        }
    }

    Column {
        id: panel
        visible: !root.answered
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.smallSpacing

        Text {
            width: parent.width
            text: root.question
            color: Theme.text
            font.pixelSize: Theme.bodyFontSize - 1
            wrapMode: Text.Wrap
        }

        // Multiple-choice buttons.
        Repeater {
            model: root.choices
            delegate: Rectangle {
                required property var modelData
                width: panel.width
                implicitHeight: choiceText.implicitHeight + Theme.smallSpacing * 2
                radius: Theme.radiusSmall
                color: choiceHover.hovered ? Theme.pressed : Theme.toolHeader
                border.width: Theme.hairline
                border.color: Theme.toolBorder

                Text {
                    id: choiceText
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: Theme.smallSpacing
                    text: String(parent.modelData)
                    color: Theme.text
                    font.pixelSize: Theme.captionFontSize
                    wrapMode: Text.Wrap
                }

                HoverHandler { id: choiceHover }
                TapHandler { onTapped: root.submit(parent.modelData) }
            }
        }

        // Freeform answer field + submit.
        Rectangle {
            width: parent.width
            implicitHeight: freeRow.implicitHeight + Theme.smallSpacing * 2
            radius: Theme.radiusSmall
            color: Theme.background
            border.width: Theme.hairline
            border.color: freeInput.activeFocus ? Theme.searchFocusBorder : Theme.toolBorder

            Row {
                id: freeRow
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: Theme.smallSpacing
                spacing: Theme.smallSpacing

                TextInput {
                    id: freeInput
                    width: parent.width - submitGlyph.width - parent.spacing
                    anchors.verticalCenter: parent.verticalCenter
                    color: Theme.text
                    font.pixelSize: Theme.captionFontSize
                    selectionColor: Theme.selection
                    selectedTextColor: Theme.selectionText
                    clip: true
                    onAccepted: root.submit(text)

                    Text {
                        anchors.fill: parent
                        visible: freeInput.text.length === 0 && !freeInput.activeFocus
                        text: qsTr("Type a reply…")
                        color: Theme.mutedText
                        font: freeInput.font
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Text {
                    id: submitGlyph
                    anchors.verticalCenter: parent.verticalCenter
                    text: FontIcons.fa_paper_plane
                    font.family: FontIcons.faSolid
                    font.pixelSize: Theme.captionFontSize
                    color: freeInput.text.length > 0 ? Theme.accent : Theme.mutedText

                    TapHandler { onTapped: root.submit(freeInput.text) }
                }
            }
        }

        // Skip action.
        Text {
            text: qsTr("Skip")
            color: skipHover.hovered ? Theme.text : Theme.mutedText
            font.pixelSize: Theme.captionFontSize - 1
            font.underline: skipHover.hovered

            HoverHandler { id: skipHover }
            TapHandler {
                onTapped: {
                    if (root.editorController && root.blockId !== undefined)
                        root.editorController.answerClarify(Number(root.blockId), root.requestId, qsTr("(skipped)"))
                }
            }
        }
    }
}
