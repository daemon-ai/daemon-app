// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick

import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Pending-approval bar for dangerous tools (Hermes inventory #3): shown inside a
// ToolCall card while a terminal / execute_code call is awaiting the user's
// decision. Renders the node's honest prompt faithfully (monospace, wrapped) plus
// Approve / Deny / Deny-with-reason / Allow-permanently, routing back to the host
// via editorController.answerToolApproval (which clears the gate). Deny-with-reason
// (wire v29) threads the operator explanation to the model via Approved.reason.
Item {
    id: root

    property var toolData: ({})
    property var editorController: null
    property var blockId: undefined

    // Deny opens an inline reason field; the bar grows to fit.
    property bool denyReasonOpen: false

    readonly property string callId: (toolData && toolData.callId) ? String(toolData.callId) : ""
    readonly property string command: (toolData && toolData.approvalCommand) ? String(toolData.approvalCommand)
                                     : (toolData && toolData.argsSummary) ? String(toolData.argsSummary) : ""
    readonly property bool allowPermanent: !!(toolData && toolData.allowPermanent)
    // [wave2:app-approvals-safety] C5: origin attribution keys (empty today; the app-engines stream
    // populates approvalOriginKind = "core"|"foreign" + approvalOrigin at Integration 2).
    readonly property string originKind: (toolData && toolData.approvalOriginKind) ? String(toolData.approvalOriginKind) : ""
    readonly property string originLabel: (toolData && toolData.approvalOrigin) ? String(toolData.approvalOrigin) : ""

    function decide(decision, permanent, reason) {
        if (root.editorController && root.blockId !== undefined)
            root.editorController.answerToolApproval(Number(root.blockId), root.callId, decision,
                                                     permanent === true, reason ? reason : "")
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

            // [wave2:app-approvals-safety] C5 origin-chip insertion point (inline card). Hidden
            // while approvalOriginKind is empty; the app-engines stream wires its EngineOriginChip
            // into this Loader at Integration 2. Empty-by-default, stable, tagged.
            Loader {
                id: originSlot
                width: parent.width
                active: root.originKind.length > 0
                visible: active
                sourceComponent: originChipComponent
            }

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

            // The node's honest prompt (resolved command / cwd / digest), rendered faithfully as
            // monospace, wrapped text — never re-derived or squashed to one line.
            Text {
                visible: root.command.length > 0
                width: parent.width
                text: root.command
                color: Theme.mutedText
                font.family: FontIcons.mono
                font.pixelSize: Theme.captionFontSize - 1
                wrapMode: Text.Wrap
                textFormat: Text.PlainText
            }

            // Deny-with-reason input (revealed by "Deny with reason…").
            Kit.TextField {
                id: reasonField
                width: parent.width
                visible: root.denyReasonOpen
                placeholderText: qsTr("Reason the agent will hear (optional)")
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
                    visible: !root.denyReasonOpen
                    label: qsTr("Approve")
                    accentColor: Theme.statusOk
                    strong: true
                    onClicked: root.decide("approved", false, "")
                }
                ApprovalButton {
                    label: root.denyReasonOpen ? qsTr("Send deny") : qsTr("Deny")
                    accentColor: Theme.danger
                    onClicked: {
                        if (root.denyReasonOpen)
                            root.decide("denied", false, reasonField.text)
                        else
                            root.decide("denied", false, "")
                    }
                }
                ApprovalButton {
                    visible: !root.denyReasonOpen
                    label: qsTr("Deny with reason…")
                    onClicked: { root.denyReasonOpen = true; reasonField.forceActiveFocus() }
                }
                ApprovalButton {
                    visible: root.allowPermanent && !root.denyReasonOpen
                    label: qsTr("Allow permanently")
                    onClicked: root.decide("approved", true, "")
                }
            }
        }
    }

    // [wave2:app-approvals-safety] C5: empty-by-default origin chip. The app-engines stream replaces
    // this component's body with its EngineOriginChip at Integration 2.
    Component {
        id: originChipComponent
        Kit.Chip {
            tone: "muted"
            iconGlyph: FontIcons.fa_circle_info
            text: root.originLabel
        }
    }
}
