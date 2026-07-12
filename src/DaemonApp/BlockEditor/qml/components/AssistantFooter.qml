// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick

import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Assistant message footer (inventory #34): a quiet action row under the last
// block of an assistant message - copy the message, regenerate the reply,
// thumbs up/down feedback, and a branch picker shown only when more than one
// branch exists. Hover reveals the actions; the row stays unobtrusive otherwise.
Item {
    id: root

    property var editorController: null
    property string messageId: ""
    // Branch picker state (optional; hidden when there is a single branch).
    property int branchIndex: 0
    property int branchCount: 1

    // Rating constants (mirror IFeedback::kRating*; QML passes plain ints).
    readonly property int ratingNone: 0
    readonly property int ratingUp: 1
    readonly property int ratingDown: -1

    // Live mirror of the submitted rating so the selected glyph survives delegate
    // recycling. ratingFor() has no change signal of its own, so bump _ratingRev
    // on messageRatingChanged to force the binding (which also depends on
    // messageId, so a recycled delegate re-reads the new message's rating).
    property int _ratingRev: 0
    readonly property int rating: {
        root._ratingRev; // re-evaluate when a submission lands
        return root.editorController ? root.editorController.ratingFor(root.messageId) : root.ratingNone
    }
    // Whether the optional-comment affordance is revealed (after a thumb tap).
    property bool commentOpen: false

    signal regenerateClicked()
    signal copyClicked()

    implicitHeight: column.implicitHeight + Theme.smallSpacing

    HoverHandler { id: hover }

    Connections {
        target: root.editorController
        function onMessageRatingChanged(id, r) {
            if (id === root.messageId)
                root._ratingRev++
        }
    }

    Column {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 2
        spacing: 2

        Row {
            id: row
            spacing: Theme.smallSpacing
            // Touch devices have no hover, so the footer stays fully visible there.
            opacity: (hover.hovered || Theme.touch || root.rating !== root.ratingNone) ? 1.0 : 0.55
            Behavior on opacity { NumberAnimation { duration: Theme.motionFast } }

            // --- Branch picker (N34) --------------------------------------------
            Row {
                visible: root.branchCount > 1
                spacing: 2

                FooterButton {
                    glyph: FontIcons.fa_chevron_left
                    enabled: root.branchIndex > 0
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: (root.branchIndex + 1) + "/" + root.branchCount
                    color: Theme.messageFooterText
                    font.family: FontIcons.mono
                    font.pixelSize: Theme.captionFontSize - 2
                }
                FooterButton {
                    glyph: FontIcons.fa_chevron_right
                    enabled: root.branchIndex < root.branchCount - 1
                }
            }

            FooterButton {
                glyph: FontIcons.fa_copy
                tip: qsTr("Copy message")
                onActivated: {
                    if (root.editorController)
                        root.editorController.copyMessageToClipboard(root.messageId)
                    root.copyClicked()
                }
            }

            FooterButton {
                glyph: FontIcons.fa_rotate
                tip: qsTr("Regenerate")
                onActivated: {
                    if (root.editorController)
                        root.editorController.requestRegenerate(root.messageId)
                    root.regenerateClicked()
                }
            }

            // --- Thumbs feedback -------------------------------------------------
            FooterButton {
                glyph: FontIcons.fa_thumbs_up
                tip: qsTr("Good response")
                selected: root.rating === root.ratingUp
                onActivated: root.rate(root.ratingUp)
            }
            FooterButton {
                glyph: FontIcons.fa_thumbs_down
                tip: qsTr("Bad response")
                selected: root.rating === root.ratingDown
                onActivated: root.rate(root.ratingDown)
            }
        }

        // Optional free-text comment + response-content consent, revealed after a
        // thumb tap. Submitting re-sends the same rating with the comment; the seam
        // records both. The checkbox is the per-event opt-in to attach the rated
        // response text to the exported feedback (default off / privacy-first).
        Column {
            id: commentRow
            visible: root.commentOpen
            spacing: Theme.smallSpacing

            Row {
                spacing: Theme.smallSpacing

                Kit.TextField {
                    id: commentField
                    width: 240
                    underline: true
                    placeholderText: qsTr("Tell us more (optional)")
                    onAccepted: root.submitComment()
                }
                Kit.IconButton {
                    icon: FontIcons.fa_paper_plane
                    iconPointSize: 12
                    tooltipText: qsTr("Send feedback")
                    onClicked: root.submitComment()
                }
            }

            Kit.CheckBox {
                id: includeContent
                text: qsTr("Include the response text")
                checked: false
            }
        }
    }

    // Record a thumb rating immediately and reveal the comment affordance. The
    // initial submit carries no comment and no content (the opt-in lives in the
    // revealed row); a follow-up submitComment() re-sends with the user's choices.
    function rate(value) {
        if (!root.editorController)
            return
        includeContent.checked = false
        root.editorController.submitMessageFeedback(root.messageId, value, "", false)
        root.commentOpen = true
    }

    // Re-submit the current rating with the typed comment + content consent, then
    // collapse the row.
    function submitComment() {
        if (root.editorController && root.rating !== root.ratingNone)
            root.editorController.submitMessageFeedback(root.messageId, root.rating,
                                                        commentField.text, includeContent.checked)
        root.commentOpen = false
        commentField.text = ""
        includeContent.checked = false
    }

    // A small icon button used by the footer row.
    component FooterButton: Rectangle {
        id: btn
        property string glyph: ""
        property string tip: ""
        // Accent-highlight when this is the active choice (e.g. selected thumb).
        property bool selected: false
        signal activated()

        width: 22
        height: 22
        radius: Theme.radius
        color: btnHover.hovered ? Theme.messageFooterHover : Theme.transparent
        opacity: enabled ? 1.0 : 0.4

        Text {
            anchors.centerIn: parent
            text: btn.glyph
            font.family: FontIcons.faSolid
            font.pixelSize: Theme.captionFontSize - 2
            color: btn.selected ? Theme.accent : Theme.messageFooterText
        }

        Accessible.role: Accessible.Button
        Accessible.name: btn.tip
        Accessible.onPressAction: btn.activated()

        HoverHandler { id: btnHover }
        TapHandler {
            enabled: btn.enabled
            onTapped: btn.activated()
        }

        Kit.Tooltip {
            text: btn.tip
            visible: btn.tip.length > 0 && btnHover.hovered
            x: Math.round((btn.width - implicitWidth) / 2)
            y: btn.height + 4
        }
    }
}
