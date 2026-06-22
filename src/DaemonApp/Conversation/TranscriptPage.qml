import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Settings
import DaemonApp.Transcript
import DaemonApp.Composer

// A single transcript tab's content: the Transcript + Composer for one open
// conversation, backed by its OWN ConversationController + ConversationOrchestrator
// so each tab keeps independent streaming/scroll state while it sits in the
// background of the pane's StackLayout. The tab host (Conversation.qml) sets
// `conversationId`; this page owns everything below the tab strip.
Rectangle {
    id: root

    color: Theme.background

    // The conversation this tab shows. Set once by the host when the tab is
    // created; re-assigning re-opens (kept for completeness, tabs are 1:1).
    property int conversationId: -1

    // No task model yet, so the header pie stays hidden.
    property int totalTasks: 0
    property int doneTasks: 0

    // A front-end-only slash command asked to open settings (e.g. "/theme").
    signal openSettingsRequested()

    // The tab title resolved from the conversation content (first heading/line).
    // The host binds this to TabModel.setTitle so the chip reflects the thread.
    signal titleResolved(string title)

    // The user committed to this conversation (submitted a turn or edited the
    // transcript); the host pins the tab so a preview tab becomes permanent.
    signal committed()

    // Exposed so the pane's settings popup can act on this tab's conversation.
    readonly property alias conversationController: controller

    // The chip label is the conversation's canonical title (the same string the
    // list shows), not the first line of the markdown content.
    function _resolveTitle() {
        if (conversationId < 0)
            return;
        const t = ConversationStore.title(conversationId);
        root.titleResolved((t && t.length > 0) ? t : qsTr("Conversation"));
    }

    ConversationController {
        id: controller
        store: ConversationStore
    }

    // The shared submit pipeline: owns the turn (injected into the transcript) and
    // the status-stack todos (rendered by the composer).
    ConversationOrchestrator {
        id: orchestrator
        conversation: controller
        onCommandRequested: function(command) {
            if (command === "theme")
                root.openSettingsRequested();
            else if (command === "distraction")
                UiSettings.distractionFree = true;
        }
        // Rewind slash commands act on the live editor (the GUI's source of truth),
        // then re-run via the same rerun seam the inline affordances use.
        onRetryRequested: transcript.retryLast()
        onEditRequested: {
            const text = transcript.editLast();
            if (text !== "") {
                composer.setText(text);
                composer.forceActiveFocus();
            }
        }
        onUndoRequested: transcript.undoLast()
    }

    // Open the bound conversation on realize and whenever it changes.
    onConversationIdChanged: {
        if (conversationId >= 0)
            controller.open(conversationId);
        root._resolveTitle();
    }
    Component.onCompleted: {
        if (conversationId >= 0)
            controller.open(conversationId);
        root._resolveTitle();
    }

    // Keep the tab title in sync with the conversation (e.g. a future rename).
    Connections {
        target: controller
        function onConversationChanged() { root._resolveTitle(); }
        function onContentChanged() { root._resolveTitle(); }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: controller.hasConversation

        Transcript {
            id: transcript
            Layout.fillWidth: true
            Layout.fillHeight: true
            // The orchestrator owns the turn; the transcript is a consumer.
            turn: orchestrator.turn
            onEdited: function(markdown) {
                controller.updateContent(markdown);
                root.committed();
            }
            // Edit / regenerate / restore truncated the document; re-run the turn.
            onRerunRequested: function(text) {
                orchestrator.rerun(text);
                root.committed();
            }

            Component.onCompleted: load(controller.content)

            Connections {
                target: controller
                function onConversationChanged() { transcript.load(controller.content); }
                function onContentChanged() { transcript.load(controller.content); }
            }
        }

        Composer {
            id: composer
            Layout.fillWidth: true
            centerContent: UiSettings.centerText
            busy: transcript.busy
            conversationId: controller.currentId
            todosModel: orchestrator.todos

            onSubmitted: function(text, attachmentRefs) {
                orchestrator.submit(text, attachmentRefs);
                root.committed();
            }
            onSteer: function(text) { orchestrator.steer(text); }
            onCancelRequested: orchestrator.cancel()
            onCommandInvoked: function(command) { orchestrator.invokeCommand(command); }
        }
    }

    // Empty state: shown until the controller has a conversation.
    Column {
        anchors.centerIn: parent
        visible: !controller.hasConversation
        spacing: Theme.spacing

        Kit.Glyph {
            anchors.horizontalCenter: parent.horizontalCenter
            glyph: FontIcons.fa_comments
            font.pointSize: 36 + Theme.pointSizeOffset
            color: Theme.border
        }

        QQC.Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Select a conversation")
            color: Theme.textMuted
            font.family: FontIcons.display
            font.pixelSize: 16
        }
    }

    // Distraction-free hides the chrome, so keep an always-visible escape hatch.
    Kit.IconButton {
        visible: UiSettings.distractionFree
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 10
        z: 10
        icon: FontIcons.fa_xmark
        iconColor: Theme.iconMuted
        backgroundRadius: width / 2
        tooltipText: qsTr("Exit distraction-free (Esc)")
        onClicked: UiSettings.distractionFree = false
    }
}
