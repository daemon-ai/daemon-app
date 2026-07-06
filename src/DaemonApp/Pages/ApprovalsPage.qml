// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Approvals inbox: pending tool-execution requests rendered with the node's honest prompt
// (resolved command / cwd / digest, faithfully — never re-derived), a fingerprint chip where the
// node offered durable permanence, and approve / deny / deny-with-reason / allow-permanently
// actions. The row carries no risk classification: the wire sends none, so the app fabricates none
// (D3/Q4).
Item {
    id: root

    PageHeader {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr("Approvals")
        icon: FontIcons.fa_circle_exclamation
    }

    Text {
        anchors.centerIn: parent
        visible: Approvals.count === 0
        text: qsTr("Inbox zero — no pending approvals.")
        font.family: FontIcons.display; font.pixelSize: 14; color: Theme.textMuted
    }

    QQC.ScrollView {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        clip: true
        contentWidth: availableWidth

        ListView {
            model: Approvals.pending
            spacing: 8
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                id: delegate
                required property var entry
                // Deny opens an inline reason field; the card grows to fit.
                property bool denyReasonOpen: false
                width: ListView.view.width
                implicitHeight: body.implicitHeight + 24
                radius: 8
                color: Theme.surface
                border.color: Theme.border
                border.width: 1

                ColumnLayout {
                    id: body
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 12
                    spacing: 6

                    // --- Header: tool + session, plus the C5 origin-chip slot ----------------
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text {
                            text: delegate.entry.tool
                            font.family: FontIcons.display; font.pixelSize: 13
                            font.bold: true; color: Theme.text
                        }
                        // [wave2:app-approvals-safety] C5 origin-chip insertion point (inbox).
                        // Hidden while approvalOriginKind is empty; the app-engines stream wires its
                        // EngineOriginChip here at Integration 2 (keys: approvalOriginKind =
                        // "core"|"foreign", approvalOrigin = display label). Kept empty-by-default.
                        Loader {
                            id: originSlotInbox
                            active: !!(delegate.entry.approvalOriginKind
                                       && String(delegate.entry.approvalOriginKind).length > 0)
                            visible: active
                            sourceComponent: originChipInbox
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: delegate.entry.session
                            font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
                            elide: Text.ElideMiddle
                            Layout.maximumWidth: 220
                        }
                    }

                    // --- Structured prompt: the node's honest text, faithfully (monospace, wrapped)
                    Text {
                        Layout.fillWidth: true
                        text: delegate.entry.prompt && String(delegate.entry.prompt).length > 0
                              ? delegate.entry.prompt : delegate.entry.command
                        font.family: FontIcons.mono; font.pixelSize: 11; color: Theme.text
                        wrapMode: Text.Wrap
                        textFormat: Text.PlainText
                    }

                    // --- Path (when the action targets a file) -------------------------------
                    RowLayout {
                        Layout.fillWidth: true
                        visible: !!(delegate.entry.path && String(delegate.entry.path).length > 0)
                        spacing: 6
                        Text {
                            text: qsTr("Path")
                            font.family: FontIcons.display; font.pixelSize: 10; color: Theme.textMuted
                        }
                        Text {
                            Layout.fillWidth: true
                            text: delegate.entry.path
                            font.family: FontIcons.mono; font.pixelSize: 11; color: Theme.text
                            elide: Text.ElideMiddle
                        }
                    }

                    // --- Fingerprint chip (durable-permanence scope) -------------------------
                    Kit.Chip {
                        visible: !!(delegate.entry.fingerprint
                                    && String(delegate.entry.fingerprint).length > 0)
                        iconGlyph: FontIcons.fa_lock
                        tone: "muted"
                        text: qsTr("fingerprint %1").arg(delegate.entry.shortFingerprint)
                        tooltipText: qsTr("Allowing permanently remembers this exact command:\n%1")
                                     .arg(delegate.entry.fingerprint)
                    }

                    // --- Deny-with-reason input (revealed by "Deny with reason…") ------------
                    Kit.TextField {
                        id: reasonField
                        Layout.fillWidth: true
                        visible: delegate.denyReasonOpen
                        placeholderText: qsTr("Reason the agent will hear (optional)")
                    }

                    // --- Actions -------------------------------------------------------------
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Item { Layout.fillWidth: true }
                        Kit.TextButton {
                            text: delegate.denyReasonOpen ? qsTr("Send deny") : qsTr("Deny")
                            onClicked: {
                                if (delegate.denyReasonOpen) {
                                    Approvals.deny(delegate.entry.id, reasonField.text)
                                } else {
                                    Approvals.deny(delegate.entry.id, "")
                                }
                            }
                        }
                        Kit.TextButton {
                            text: qsTr("Deny with reason…")
                            visible: !delegate.denyReasonOpen
                            onClicked: { delegate.denyReasonOpen = true; reasonField.forceActiveFocus() }
                        }
                        // Wire v28: only shown when the node offered to remember this command
                        // (entry.canAllowPermanent). Sends allow_permanent so the node adds the
                        // command's fingerprint to the session allow-list.
                        Kit.TextButton {
                            text: qsTr("Allow permanently")
                            visible: delegate.entry.canAllowPermanent === true
                                     && !delegate.denyReasonOpen
                            onClicked: Approvals.approve(delegate.entry.id, true)
                        }
                        Kit.TextButton {
                            text: qsTr("Approve"); accentFilled: true
                            visible: !delegate.denyReasonOpen
                            onClicked: Approvals.approve(delegate.entry.id, false)
                        }
                    }
                }

                // [wave2:app-approvals-safety] C5: empty-by-default origin chip. The app-engines
                // stream replaces this component's body with its EngineOriginChip at Integration 2.
                Component {
                    id: originChipInbox
                    Kit.Chip {
                        tone: "muted"
                        iconGlyph: FontIcons.fa_circle_info
                        text: delegate.entry.approvalOrigin
                              ? String(delegate.entry.approvalOrigin) : ""
                    }
                }
            }
        }
    }
}
