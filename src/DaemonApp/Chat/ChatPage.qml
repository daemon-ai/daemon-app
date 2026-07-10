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
// one open room/DM, backed by a shared ChatConversationController over the node's
// IChatService. There is NO bespoke editor here — the transcript reuses the
// existing BlockEditor (via MarkdownDocumentView, which renders the controller's
// oldest-first markdown projection) and input reuses the existing Composer. The
// host (Session.qml) sets `transport`/`conversation`; this page owns everything
// below the tab strip. GUI/TUI parity: the TUI binds the very same controller.
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

    // The shared conversation view-model: projects ConvHistory rows to markdown,
    // forwards ConvSend, applies MessagesChanged refreshes. Bound to the app-global
    // Chat seam (mock or daemon-backed).
    ChatConversationController {
        id: convo
        service: (typeof Chat !== "undefined") ? Chat : null
        // Re-render the transcript whenever the authoritative rows change (initial
        // history load, a MessagesChanged refresh, or a send round-trip).
        onMarkdownChanged: transcript.loadBytes(convo.markdown)
        // A ConvSend failed on the node/transport: non-blocking toast (house pattern).
        onSendFailed: function(message) {
            sendToast.show(message.length > 0 ? message : qsTr("Couldn't send the message."));
        }
    }

    function _rebind() {
        if (root.transport !== "" && root.conversation !== "")
            convo.open(root.transport, root.conversation);
    }
    onTransportChanged: root._rebind()
    onConversationChanged: root._rebind()
    Component.onCompleted: root._rebind()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

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

        // Input: the SHARED Composer. Submitting sends via ConvSend (no local echo —
        // the node's MessagesChanged brings the line back authoritatively).
        Composer {
            id: composer
            Layout.fillWidth: true
            centerContent: UiSettings.centerText
            // A chat conversation has no assistant turn, so the composer is never busy.
            busy: false
            // A chat-scoped id so per-conversation drafts don't collide with sessions.
            sessionId: "chat:" + root.transport + ":" + root.conversation
            onSubmitted: function(text, attachmentRefs) { convo.send(text); }
        }
    }

    Kit.Toast { id: sendToast }
}
