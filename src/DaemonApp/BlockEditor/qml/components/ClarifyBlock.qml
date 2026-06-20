import QtQuick

import DaemonApp.Theme

// Interactive clarify panel (Hermes inventory #7). Supports one or more
// questions, each of which may be single-select, multi-select, and/or accept a
// freeform reply. The whole form is answered with a single submit: answers are
// collected into a { questionId: value } map (value is a string for
// single-select/freeform, or a string list for multi-select) and routed to the
// host through editorController.answerClarify. Once answered it collapses to a
// compact resolved summary; the agent host then drives the underlying tool to
// completion.
Item {
    id: root

    property var toolData: ({})
    property var editorController: null
    property var blockId: undefined

    readonly property string requestId: (toolData && toolData.requestId) ? String(toolData.requestId) : ""
    readonly property bool answered: !!(toolData && toolData.answered)
    readonly property var answers: (toolData && toolData.answers) ? toolData.answers : ({})

    // Normalize to a list of question objects. A legacy single-question clarify
    // (top-level question/choices) is mapped to one question with id "q" that
    // also allows a freeform reply, preserving the original behaviour.
    readonly property var questions: {
        if (toolData && toolData.questions && toolData.questions.length > 0)
            return toolData.questions
        const prompt = (toolData && toolData.question) ? String(toolData.question) : qsTr("The agent needs your input.")
        return [{
            "id": "q",
            "prompt": prompt,
            "choices": (toolData && toolData.choices) ? toolData.choices : [],
            "multiSelect": false,
            "allowFreeform": true
        }]
    }

    // Working state, keyed by question id. `selections` holds the chosen
    // choice(s); `freeform` holds typed text. Both are reassigned wholesale so
    // bindings that read them re-evaluate.
    property var selections: ({})
    property var freeform: ({})

    function questionId(q, index) {
        return (q && q.id !== undefined) ? String(q.id) : ("q" + index)
    }

    function isSelected(qid, choice) {
        const value = selections[qid]
        if (value === undefined)
            return false
        if (Array.isArray(value))
            return value.indexOf(choice) >= 0
        return value === choice
    }

    function toggleChoice(qid, choice, multi) {
        const next = Object.assign({}, selections)
        if (multi) {
            const current = Array.isArray(next[qid]) ? next[qid].slice() : []
            const at = current.indexOf(choice)
            if (at >= 0)
                current.splice(at, 1)
            else
                current.push(choice)
            next[qid] = current
        } else {
            // Tapping the active single-select choice clears it.
            next[qid] = (next[qid] === choice) ? undefined : choice
        }
        selections = next
    }

    function setFreeform(qid, text) {
        const next = Object.assign({}, freeform)
        next[qid] = text
        freeform = next
    }

    // Assemble the answers map. Freeform text augments a multi-select list and
    // fills in single-select questions that have no choice picked.
    function collectAnswers() {
        const out = ({})
        for (let i = 0; i < questions.length; ++i) {
            const q = questions[i]
            const qid = questionId(q, i)
            let value = selections[qid]
            const typed = freeform[qid] !== undefined ? String(freeform[qid]).trim() : ""
            if (q.allowFreeform && typed.length > 0) {
                if (q.multiSelect) {
                    const list = Array.isArray(value) ? value.slice() : []
                    list.push(typed)
                    value = list
                } else if (value === undefined) {
                    value = typed
                }
            }
            if (value !== undefined && !(Array.isArray(value) && value.length === 0))
                out[qid] = value
        }
        return out
    }

    function submit() {
        const collected = collectAnswers()
        if (Object.keys(collected).length === 0)
            return
        if (root.editorController && root.blockId !== undefined)
            root.editorController.answerClarify(Number(root.blockId), root.requestId, collected)
    }

    function skip() {
        const out = ({})
        for (let i = 0; i < questions.length; ++i)
            out[questionId(questions[i], i)] = qsTr("(skipped)")
        if (root.editorController && root.blockId !== undefined)
            root.editorController.answerClarify(Number(root.blockId), root.requestId, out)
    }

    function answerDisplay(qid) {
        const value = answers ? answers[qid] : undefined
        if (value === undefined)
            return ""
        if (Array.isArray(value))
            return value.join(", ")
        return String(value)
    }

    implicitHeight: answered ? resolved.implicitHeight : panel.implicitHeight

    // Compact resolved summary once the user has answered: one row per question.
    Column {
        id: resolved
        visible: root.answered
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.smallSpacing / 2

        Repeater {
            model: root.questions
            delegate: Row {
                id: resolvedRow
                required property var modelData
                required property int index
                readonly property string qid: root.questionId(modelData, index)
                width: resolved.width
                spacing: Theme.smallSpacing
                visible: root.answerDisplay(qid).length > 0

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: FontIcons.fa_circle_check
                    font.family: FontIcons.faSolid
                    font.pixelSize: Theme.captionFontSize
                    color: Theme.statusOk
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.min(implicitWidth, resolvedRow.width * 0.4)
                    text: (resolvedRow.modelData.prompt ? String(resolvedRow.modelData.prompt) : qsTr("Answer")) + ":"
                    color: Theme.mutedText
                    font.pixelSize: Theme.captionFontSize
                    elide: Text.ElideRight
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.min(implicitWidth, resolvedRow.width * 0.5)
                    text: root.answerDisplay(resolvedRow.qid)
                    color: Theme.text
                    font.pixelSize: Theme.captionFontSize
                    font.bold: true
                    elide: Text.ElideRight
                }
            }
        }
    }

    Column {
        id: panel
        visible: !root.answered
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.spacing

        // One section per question: prompt, choice chips, and freeform field.
        Repeater {
            model: root.questions
            delegate: Column {
                id: questionBlock
                required property var modelData
                required property int index
                readonly property string qid: root.questionId(modelData, index)
                readonly property var choices: modelData.choices ? modelData.choices : []
                readonly property bool multi: !!modelData.multiSelect
                readonly property bool allowFreeform: modelData.allowFreeform === undefined
                    ? choices.length === 0
                    : !!modelData.allowFreeform
                width: panel.width
                spacing: Theme.smallSpacing

                Text {
                    width: parent.width
                    text: modelData.prompt ? String(modelData.prompt) : qsTr("The agent needs your input.")
                    color: Theme.text
                    font.pixelSize: Theme.bodyFontSize - 1
                    wrapMode: Text.Wrap
                }

                Text {
                    visible: questionBlock.multi
                    text: qsTr("Select all that apply")
                    color: Theme.mutedText
                    font.pixelSize: Theme.captionFontSize - 1
                }

                // Choice chips (single- or multi-select).
                Flow {
                    width: parent.width
                    spacing: Theme.smallSpacing
                    visible: questionBlock.choices.length > 0

                    Repeater {
                        model: questionBlock.choices
                        delegate: Rectangle {
                            id: chip
                            required property var modelData
                            readonly property bool selected: root.isSelected(questionBlock.qid, modelData)
                            implicitWidth: chipRow.implicitWidth + Theme.spacing * 2
                            implicitHeight: chipRow.implicitHeight + Theme.smallSpacing * 2
                            radius: Theme.radiusSmall
                            color: chip.selected ? (chipHover.hovered ? Qt.darker(Theme.accent, 1.1) : Theme.accent)
                                : (chipHover.hovered ? Theme.pressed : Theme.toolHeader)
                            border.width: Theme.hairline
                            border.color: chip.selected ? Theme.accent : Theme.toolBorder

                            Row {
                                id: chipRow
                                anchors.centerIn: parent
                                spacing: Theme.smallSpacing

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: chip.selected
                                    text: FontIcons.check
                                    font.family: FontIcons.faSolid
                                    font.pixelSize: Theme.captionFontSize - 2
                                    color: Theme.background
                                }
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: String(chip.modelData)
                                    color: chip.selected ? Theme.background : Theme.text
                                    font.pixelSize: Theme.captionFontSize
                                    font.bold: chip.selected
                                }
                            }

                            HoverHandler { id: chipHover }
                            TapHandler {
                                onTapped: root.toggleChoice(questionBlock.qid, chip.modelData, questionBlock.multi)
                            }
                        }
                    }
                }

                // Optional freeform reply for this question.
                Rectangle {
                    width: parent.width
                    visible: questionBlock.allowFreeform
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
                            width: parent.width
                            anchors.verticalCenter: parent.verticalCenter
                            color: Theme.text
                            font.pixelSize: Theme.captionFontSize
                            selectionColor: Theme.selection
                            selectedTextColor: Theme.selectionText
                            clip: true
                            onTextChanged: root.setFreeform(questionBlock.qid, text)
                            onAccepted: root.submit()

                            Text {
                                anchors.fill: parent
                                visible: freeInput.text.length === 0 && !freeInput.activeFocus
                                text: questionBlock.choices.length > 0 ? qsTr("Or type a reply…") : qsTr("Type a reply…")
                                color: Theme.mutedText
                                font: freeInput.font
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }
            }
        }

        // Footer: submit + skip.
        Row {
            width: parent.width
            spacing: Theme.spacing

            Rectangle {
                id: submitButton
                readonly property bool ready: Object.keys(root.collectAnswers()).length > 0
                implicitWidth: submitRow.implicitWidth + Theme.spacing * 2
                implicitHeight: submitRow.implicitHeight + Theme.smallSpacing * 2
                radius: Theme.radiusSmall
                color: submitButton.ready
                    ? (submitHover.hovered ? Qt.darker(Theme.accent, 1.1) : Theme.accent)
                    : Theme.toolHeader
                border.width: Theme.hairline
                border.color: submitButton.ready ? Theme.accent : Theme.toolBorder
                opacity: submitButton.ready ? 1.0 : 0.6

                Row {
                    id: submitRow
                    anchors.centerIn: parent
                    spacing: Theme.smallSpacing

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: FontIcons.fa_paper_plane
                        font.family: FontIcons.faSolid
                        font.pixelSize: Theme.captionFontSize
                        color: submitButton.ready ? Theme.background : Theme.mutedText
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Submit")
                        color: submitButton.ready ? Theme.background : Theme.mutedText
                        font.pixelSize: Theme.captionFontSize
                        font.bold: true
                    }
                }

                HoverHandler { id: submitHover; enabled: submitButton.ready }
                TapHandler {
                    enabled: submitButton.ready
                    onTapped: root.submit()
                }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Skip")
                color: skipHover.hovered ? Theme.text : Theme.mutedText
                font.pixelSize: Theme.captionFontSize - 1
                font.underline: skipHover.hovered

                HoverHandler { id: skipHover }
                TapHandler { onTapped: root.skip() }
            }
        }
    }
}
