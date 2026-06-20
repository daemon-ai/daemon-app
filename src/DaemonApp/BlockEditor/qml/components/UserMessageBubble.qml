import QtQuick
import QtQuick.Controls

import DaemonApp.Theme

// User message bubble (inventory #20-22): a glass bubble holding the user's text
// (simplified markdown render), a directive/attachment chip row, and a hover
// edit affordance. Clicking edit swaps the body for an inline composer
// (inventory #21) seeded with the message text; saving truncates the document at
// this message and re-runs the turn via EditorController.editUserMessage.
Item {
    id: root

    property string markdown: ""
    property string displayMarkup: ""
    property var editorController: null
    property string messageId: ""
    // Branch picker state (optional; the row hides when there is one branch).
    property int branchIndex: 0
    property int branchCount: 1

    property bool editing: false

    readonly property string bodyFamily: (editorController && editorController.bodyFontFamily !== "")
        ? editorController.bodyFontFamily : FontIcons.display

    implicitHeight: bubble.implicitHeight

    function beginEdit() {
        if (!editorController)
            return
        // Escape stick-to-bottom before the composer grows, so the view doesn't
        // yank the editor to the bottom (mirrors Hermes' beginEditHold).
        editorController.notifyInlineEditOpen()
        editArea.text = editorController.messageText(messageId)
        editing = true
        editArea.forceActiveFocus()
    }
    function commitEdit() {
        const next = editArea.text
        editing = false
        if (editorController)
            editorController.editUserMessage(messageId, next)
    }
    function cancelEdit() {
        editing = false
    }

    HoverHandler { id: hover }

    Rectangle {
        id: bubble
        anchors.left: parent.left
        anchors.right: parent.right
        color: Theme.bubbleUser
        border.width: Theme.hairline
        border.color: Theme.bubbleUserBorder
        radius: Theme.radiusSmall
        implicitHeight: column.implicitHeight + Theme.smallSpacing * 2

        Column {
            id: column
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.smallSpacing
            spacing: Theme.smallSpacing

            // --- Read view ---------------------------------------------------
            Text {
                id: body
                visible: !root.editing
                width: parent.width
                text: root.displayMarkup
                textFormat: Text.RichText
                color: Theme.bubbleUserText
                font.family: root.bodyFamily
                font.pixelSize: Theme.bodyFontSize
                wrapMode: Text.Wrap
                renderType: Text.QtRendering
                onLinkActivated: link => Qt.openUrlExternally(link)
            }

            DirectiveChips {
                width: parent.width
                visible: !root.editing
                markdown: root.markdown
            }

            // --- Inline edit composer (N16) ---------------------------------
            Rectangle {
                width: parent.width
                visible: root.editing
                radius: Theme.radius
                color: Theme.background
                border.width: Theme.hairline
                border.color: Theme.searchFocusBorder
                implicitHeight: editArea.implicitHeight + Theme.smallSpacing * 2

                TextArea {
                    id: editArea
                    anchors.fill: parent
                    anchors.margins: Theme.smallSpacing
                    wrapMode: TextArea.Wrap
                    background: null
                    color: Theme.text
                    selectionColor: Theme.selection
                    selectedTextColor: Theme.selectionText
                    font.family: root.bodyFamily
                    font.pixelSize: Theme.bodyFontSize

                    Keys.onPressed: event => {
                        const cmd = (event.modifiers & Qt.ControlModifier) || (event.modifiers & Qt.MetaModifier)
                        if (event.key === Qt.Key_Escape) {
                            root.cancelEdit()
                            event.accepted = true
                        } else if (cmd && (event.key === Qt.Key_Return || event.key === Qt.Key_Enter)) {
                            root.commitEdit()
                            event.accepted = true
                        }
                    }
                }
            }
        }
    }

    // Hover affordance: edit pencil (read view) / save + cancel (edit view).
    Row {
        anchors.right: bubble.right
        anchors.top: bubble.top
        anchors.margins: 4
        spacing: 2
        visible: root.editing || hover.hovered

        ActionButton {
            visible: !root.editing
            glyph: FontIcons.fa_pen_to_square
            onActivated: root.beginEdit()
        }
        ActionButton {
            visible: root.editing
            glyph: FontIcons.check
            onActivated: root.commitEdit()
        }
        ActionButton {
            visible: root.editing
            glyph: FontIcons.fa_xmark
            onActivated: root.cancelEdit()
        }
    }

    component ActionButton: Rectangle {
        id: ab
        property string glyph: ""
        signal activated()
        width: 22
        height: 22
        radius: Theme.radius
        color: abHover.hovered ? Theme.messageFooterHover : Theme.transparent

        Text {
            anchors.centerIn: parent
            text: ab.glyph
            font.family: FontIcons.faSolid
            font.pixelSize: Theme.captionFontSize - 2
            color: Theme.messageFooterText
        }

        HoverHandler { id: abHover }
        TapHandler { onTapped: ab.activated() }
    }
}
