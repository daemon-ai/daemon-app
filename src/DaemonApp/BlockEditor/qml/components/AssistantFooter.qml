import QtQuick

import DaemonApp.Theme

// Assistant message footer (inventory #34): a quiet action row under the last
// block of an assistant message - copy the message, regenerate the reply, and a
// branch picker shown only when more than one branch exists. Hover reveals the
// actions; the row stays unobtrusive otherwise.
Item {
    id: root

    property var editorController: null
    property string messageId: ""
    // Branch picker state (optional; hidden when there is a single branch).
    property int branchIndex: 0
    property int branchCount: 1

    signal regenerateClicked()
    signal copyClicked()

    implicitHeight: row.implicitHeight + Theme.smallSpacing

    HoverHandler { id: hover }

    Row {
        id: row
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 2
        spacing: Theme.smallSpacing
        // Touch devices have no hover, so the footer stays fully visible there.
        opacity: (hover.hovered || Theme.touch) ? 1.0 : 0.55
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
    }

    // A small icon button used by the footer row.
    component FooterButton: Rectangle {
        id: btn
        property string glyph: ""
        property string tip: ""
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
            color: Theme.messageFooterText
        }

        HoverHandler { id: btnHover }
        TapHandler {
            enabled: btn.enabled
            onTapped: btn.activated()
        }
    }
}
