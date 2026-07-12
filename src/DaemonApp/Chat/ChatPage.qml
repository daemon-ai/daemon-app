// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Settings
import DaemonApp.BlockEditor
import DaemonApp.Composer
import DaemonApp.Chat

// A single native chat tab's content: the conversation transcript + composer for
// one open room/DM, backed by a shared ChatConversationController. There is NO
// bespoke editor here — the transcript reuses the existing BlockEditor (via
// MarkdownDocumentView, which renders the controller's oldest-first markdown
// projection) and input reuses the existing Composer. The host (Session.qml) sets
// `transport`/`conversation`; this page owns everything below the tab strip.
// GUI/TUI parity: the TUI binds the very same controllers.
//
// [mirror M2] With the mirror live (`Mirror`/`Outbox` context properties, daemon
// mode), the timeline reads the mirror's ChatWindowModel through the controller
// and sends route through the durable ConvSend outbox lane (enqueue → manual
// drain, §6.8 — no auto-replay): pending intent renders ONLY in the pending strip
// beside the timeline (never a faked transcript row, R2), with per-row
// retry / edit / discard and a lane-level send affordance (§6.5). In mock mode
// both context properties are null and the legacy IChatService path runs
// unchanged (A8's seeder retires that fallback).
Rectangle {
    id: root

    color: Theme.background

    // The conversation this tab shows (the (transport, conv) key). Set by the host
    // when the tab is created; re-assigning rebinds the controller in place.
    property string transport: ""
    property string conversation: ""

    // True while this is the foreground tab (bound by the host). Kept for parity
    // with TranscriptPage; the chat surface has no turn to gate on.
    property bool isActive: false

    // The shared conversation view-model: projects the timeline (mirror window or
    // legacy rows) to markdown. Bound to the app-global Chat + Mirror seams.
    ChatConversationController {
        id: convo
        service: (typeof Chat !== "undefined") ? Chat : null
        mirror: (typeof Mirror !== "undefined") ? Mirror : null
        // Re-render the transcript whenever the authoritative rows change (initial
        // history load, a journal delta / MessagesChanged refresh, or a send
        // round-trip).
        onMarkdownChanged: transcript.loadBytes(convo.markdown)
        // A ConvSend failed on the node/transport (legacy path): non-blocking toast.
        onSendFailed: function(message) {
            sendToast.show(message.length > 0 ? message : qsTr("Couldn't send the message."));
        }
    }

    // The ConvSend outbox lane + per-conversation pending strip lens (§6.4/§8.4).
    ConvSendController {
        id: sendCtl
        outbox: (typeof Outbox !== "undefined") ? Outbox : null
        // A node rejection surfaced through the verb seam (§6.5): non-blocking toast;
        // the strip row keeps the durable retry/edit/discard affordances.
        onSendRejected: function(message) {
            sendToast.show(message.length > 0 ? message : qsTr("Couldn't send the message."));
        }
    }

    function _rebind() {
        if (root.transport !== "" && root.conversation !== "") {
            convo.open(root.transport, root.conversation);
            sendCtl.open(root.transport, root.conversation);
        }
    }
    onTransportChanged: root._rebind()
    onConversationChanged: root._rebind()
    Component.onCompleted: root._rebind()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Demand paging (§4.6): ask the ingestor for older history (mirror mode).
        // v38 answers a cold window with the forward fill; once the reachable tail
        // is held the affordance disappears (end-of-history until BR's backward
        // windows).
        Kit.TextButton {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Theme.spacingSmall
            visible: convo.mirror !== null && convo.canLoadEarlier && !convo.empty
            text: qsTr("Load earlier messages")
            onClicked: convo.loadEarlier()
        }

        // The transcript: the SHARED BlockEditor markdown renderer. The controller
        // owns the projection; this view only renders + reloads it.
        MarkdownDocumentView {
            id: transcript
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentMargin: Theme.spacing
        }

        // Empty-state until the first rows land.
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: convo.empty
            Column {
                anchors.centerIn: parent
                spacing: Theme.spacing
                Kit.Glyph {
                    anchors.horizontalCenter: parent.horizontalCenter
                    glyph: FontIcons.fa_comments
                    font.pointSize: 36 + Theme.pointSizeOffset
                    color: Theme.border
                }
                QQC.Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("No messages yet")
                    color: Theme.textMuted
                    font.family: FontIcons.display
                    font.pixelSize: 16
                }
            }
        }

        // ---- The pending strip (§8.4): the outbox lens BESIDE the timeline. ----
        // Pending ops are visibly distinct from node-confirmed rows (they live here,
        // never in the transcript, R2); rejected entries carry retry/edit/discard
        // (§6.5). Docked above the composer, mirroring the QueuePanel idiom.
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacing
            Layout.rightMargin: Theme.spacing
            Layout.bottomMargin: Theme.spacingSmall
            spacing: 2
            visible: sendCtl.pendingCount > 0

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Kit.Glyph {
                    glyph: FontIcons.fa_clock
                    font.pointSize: 10 + Theme.pointSizeOffset
                    color: Theme.textMuted
                }
                QQC.Label {
                    Layout.fillWidth: true
                    text: qsTr("Unsent (%1)").arg(sendCtl.pendingCount)
                    font.family: FontIcons.display
                    font.pixelSize: Theme.labelSize
                    font.weight: Font.DemiBold
                    font.letterSpacing: Theme.labelTracking
                    font.capitalization: Font.AllUppercase
                    color: Theme.textMuted
                }
                // The §6.8 manual-drain affordance ("review to send"); when a
                // rejection paused the lane the explicit override takes its place.
                Kit.TextButton {
                    visible: !sendCtl.lanePaused
                    text: qsTr("Send")
                    onClicked: sendCtl.drain()
                }
                Kit.TextButton {
                    visible: sendCtl.lanePaused
                    text: qsTr("Send remaining anyway")
                    onClicked: sendCtl.sendRemainingAnyway()
                }
            }

            Repeater {
                model: sendCtl.pending
                delegate: Rectangle {
                    id: opRow
                    // Role access via `model.*`: the `state` role would shadow Item.state as a
                    // required property, so the whole row reads through the model attached object.
                    required property var model
                    readonly property bool opRejected: opRow.model.rejected
                    Layout.fillWidth: true
                    implicitHeight: 32
                    radius: 6
                    color: opRow.opRejected ? Theme.activeBlockBackground
                         : opArea.hovered ? Theme.hover : "transparent"

                    HoverHandler { id: opArea }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 4
                        spacing: 6

                        Kit.Glyph {
                            glyph: opRow.opRejected ? FontIcons.fa_triangle_exclamation
                                 : opRow.model.state === 1 ? FontIcons.fa_arrow_up
                                 : FontIcons.fa_clock
                            font.pointSize: 10 + Theme.pointSizeOffset
                            color: opRow.opRejected ? Theme.danger : Theme.iconMuted
                        }
                        QQC.Label {
                            // Pending-state tag: queued / sending / sent-awaiting /
                            // failed — the visibly-distinct state per row (§6.5).
                            text: opRow.opRejected ? qsTr("Failed")
                                : opRow.model.state === 1 ? qsTr("Sending…")
                                : opRow.model.state === 3 ? qsTr("Sent")
                                : qsTr("Queued")
                            font.family: FontIcons.display
                            font.pixelSize: 11
                            color: opRow.opRejected ? Theme.danger : Theme.textMuted
                        }
                        QQC.Label {
                            Layout.fillWidth: true
                            text: opRow.opRejected && opRow.model.lastError.length > 0
                                ? opRow.model.display + " — " + opRow.model.lastError
                                : opRow.model.display
                            font.family: FontIcons.display
                            font.pixelSize: 12
                            color: Theme.text
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }

                        Kit.IconButton {
                            visible: opRow.opRejected
                            implicitWidth: 26
                            implicitHeight: 26
                            icon: FontIcons.fa_rotate
                            iconColor: Theme.iconMuted
                            iconPointSize: 11
                            tooltipText: qsTr("Retry")
                            onClicked: sendCtl.retry(opRow.model.opId)
                        }
                        Kit.IconButton {
                            visible: opRow.opRejected
                            implicitWidth: 26
                            implicitHeight: 26
                            icon: FontIcons.fa_pen_to_square
                            iconColor: Theme.iconMuted
                            iconPointSize: 11
                            tooltipText: qsTr("Edit")
                            onClicked: composer.setText(sendCtl.takeForEdit(opRow.model.opId))
                        }
                        Kit.IconButton {
                            visible: opRow.opRejected
                            implicitWidth: 26
                            implicitHeight: 26
                            icon: FontIcons.fa_xmark
                            iconColor: Theme.iconMuted
                            iconPointSize: 12
                            tooltipText: qsTr("Discard")
                            onClicked: sendCtl.discard(opRow.model.opId)
                        }
                    }
                }
            }
        }

        // Input: the SHARED Composer. With the outbox lane live, submitting enqueues
        // durably then drains on this explicit user action (§6.8 manual drain — the
        // strip shows anything held back); the confirmed line lands only via the
        // node round-trip (no local echo, R2). Mock/fallback keeps the legacy send.
        Composer {
            id: composer
            Layout.fillWidth: true
            centerContent: UiSettings.centerText
            // A chat conversation has no assistant turn, so the composer is never busy.
            busy: false
            // A chat-scoped id so per-conversation drafts don't collide with sessions.
            sessionId: "chat:" + root.transport + ":" + root.conversation
            onSubmitted: function(text, attachmentRefs) {
                if (sendCtl.laneActive) {
                    sendCtl.send(text);
                    sendCtl.drain();
                } else {
                    convo.send(text);
                }
            }
        }
    }

    Kit.Toast { id: sendToast }
}
