// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.BlockEditor as BE // [waveB:app-v30] D5: DiffBlock + DiffFormat (shared parse)

// Approvals inbox: pending tool-execution requests rendered with the node's honest prompt
// (resolved command / cwd / digest, faithfully — never re-derived), a fingerprint chip where the
// node offered durable permanence, and approve / deny / deny-with-reason / allow-permanently
// actions. The row carries no risk classification: the wire sends none, so the app fabricates none
// (D3/Q4).
Item {
    id: root
    objectName: "approvalsPage"

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
                        // [wave2:integration] C5 origin engine chip (inbox). Self-resolves the
                        // requesting engine from the row's session via the EngineIdentity facade;
                        // renders nothing for native sessions, so it is safe placed unconditionally.
                        Kit.EngineOriginChip {
                            sessionId: delegate.entry.session
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

                    // --- Structured diff detail ([waveB:app-v30] D5) -------------------------
                    // When the node attaches an fs.diff ToolDetail, render the unified diff below
                    // the prompt using the SAME parser (DiffFormat -> be::parseUnifiedDiff) and the
                    // SAME DiffBlock the transcript uses. Unknown kinds / no detail degrade away.
                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: delegate.entry.detailKind === "fs.diff"
                                 && !!(delegate.entry.diff && String(delegate.entry.diff).length > 0)
                        spacing: 4
                        Text {
                            Layout.fillWidth: true
                            visible: !!(delegate.entry.diffPath
                                        && String(delegate.entry.diffPath).length > 0)
                            text: delegate.entry.diffPath ? delegate.entry.diffPath : ""
                            font.family: FontIcons.mono; font.pixelSize: 11; color: Theme.textMuted
                            elide: Text.ElideMiddle
                        }
                        BE.DiffBlock {
                            Layout.fillWidth: true
                            lines: delegate.entry.diff
                                   ? BE.DiffFormat.parse(String(delegate.entry.diff)) : []
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
            }
        }
    }
}
