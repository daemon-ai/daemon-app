import QtQuick

import DaemonApp.Theme

// Pending-approval bar for dangerous tools (Hermes inventory #3): shown inside a
// ToolCall card while a terminal / execute_code call is awaiting the user's
// decision. Approve / Deny (and optionally Allow permanently) route back to the
// host via editorController.answerToolApproval, which clears the gate.
Item {
    id: root

    property var toolData: ({})
    property var editorController: null
    property var blockId: undefined

    readonly property string callId: (toolData && toolData.callId) ? String(toolData.callId) : ""
    readonly property string command: (toolData && toolData.approvalCommand) ? String(toolData.approvalCommand)
                                     : (toolData && toolData.argsSummary) ? String(toolData.argsSummary) : ""
    readonly property bool allowPermanent: !!(toolData && toolData.allowPermanent)

    function decide(decision, permanent) {
        if (root.editorController && root.blockId !== undefined)
            root.editorController.answerToolApproval(Number(root.blockId), root.callId, decision, permanent === true)
    }

    implicitHeight: bar.implicitHeight

    Rectangle {
        id: bar
        anchors.left: parent.left
        anchors.right: parent.right
        radius: Theme.radiusSmall
        color: Theme.toolHeader
        border.width: Theme.hairline
        border.color: Theme.warning
        implicitHeight: layout.implicitHeight + Theme.smallSpacing * 2

        Column {
            id: layout
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.smallSpacing
            spacing: Theme.smallSpacing

            Row {
                width: parent.width
                spacing: Theme.smallSpacing

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: FontIcons.fa_circle_exclamation
                    font.family: FontIcons.faSolid
                    font.pixelSize: Theme.captionFontSize
                    color: Theme.warning
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Approve this action?")
                    color: Theme.text
                    font.pixelSize: Theme.captionFontSize
                    font.bold: true
                }
            }

            Text {
                visible: root.command.length > 0
                width: parent.width
                text: root.command
                color: Theme.mutedText
                font.family: FontIcons.mono
                font.pixelSize: Theme.captionFontSize - 1
                wrapMode: Text.Wrap
            }

            Row {
                spacing: Theme.smallSpacing

                component ApprovalButton: Rectangle {
                    id: btn
                    property string label: ""
                    property color accentColor: Theme.accent
                    property bool strong: false
                    signal clicked()

                    implicitWidth: btnText.implicitWidth + Theme.spacing * 2
                    implicitHeight: btnText.implicitHeight + Theme.smallSpacing * 1.5
                    radius: Theme.radiusSmall
                    color: btn.strong ? (btnHover.hovered ? Qt.darker(btn.accentColor, 1.1) : btn.accentColor)
                                      : (btnHover.hovered ? Theme.pressed : Theme.background)
                    border.width: Theme.hairline
                    border.color: btn.strong ? btn.accentColor : Theme.toolBorder

                    Text {
                        id: btnText
                        anchors.centerIn: parent
                        text: btn.label
                        color: btn.strong ? Theme.background : Theme.text
                        font.pixelSize: Theme.captionFontSize
                        font.bold: btn.strong
                    }

                    HoverHandler { id: btnHover }
                    TapHandler { onTapped: btn.clicked() }
                }

                ApprovalButton {
                    label: qsTr("Approve")
                    accentColor: Theme.statusOk
                    strong: true
                    onClicked: root.decide("approved", false)
                }
                ApprovalButton {
                    label: qsTr("Deny")
                    accentColor: Theme.danger
                    onClicked: root.decide("denied", false)
                }
                ApprovalButton {
                    visible: root.allowPermanent
                    label: qsTr("Allow permanently")
                    onClicked: root.decide("approved", true)
                }
            }
        }
    }
}
