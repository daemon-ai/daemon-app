// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Settings
import DaemonApp.Transcript
import DaemonApp.Composer

// A single transcript tab's content: the Transcript + Composer for one open
// session, backed by its OWN SessionController + SessionOrchestrator
// so each tab keeps independent streaming/scroll state while it sits in the
// background of the pane's StackLayout. The tab host (Session.qml) sets
// `sessionId`; this page owns everything below the tab strip.
Rectangle {
    id: root

    color: Theme.background

    // The session this tab shows. Set once by the host when the tab is
    // created; re-assigning re-opens (kept for completeness, tabs are 1:1).
    property string sessionId: ""

    // True while this is the foreground tab (bound by the host). Only the active
    // tab feeds the shared footer status model, so a background tab can keep
    // streaming into its own document without hijacking the footer.
    property bool isActive: false

    // No task model yet, so the header pie stays hidden.
    property int totalTasks: 0
    property int doneTasks: 0

    // [wave2:app-delegation] F1: a read-only child-transcript drill-in (opened from a delegation
    // card / Fleet row). The transcript still streams + renders, but the composer is suppressed —
    // operator steer/cancel stays on the Fleet rows (wave-1), not in the viewer.
    property bool readOnly: false

    // A front-end-only slash command asked to open settings (e.g. "/theme").
    signal openSettingsRequested()

    // A command this page can't satisfy locally (e.g. "help", "title", "save");
    // the host (Main.qml) routes it to a window-level action.
    signal commandForwarded(string command)

    // The tab title resolved from the session content (first heading/line).
    // The host binds this to TabModel.setTitle so the chip reflects the thread.
    signal titleResolved(string title)

    // The user committed to this session (submitted a turn or edited the
    // transcript); the host pins the tab so a preview tab becomes permanent.
    signal committed()

    // Exposed so the pane's settings popup can act on this tab's session.
    readonly property alias sessionController: controller

    // Route a palette/slash command into this tab's orchestrator (the same entry
    // point the composer uses), so the command palette reaches the foreground tab.
    function invokeCommand(command) {
        orchestrator.invokeCommand(command);
    }

    // Simulated context compaction: drop the live context fill to a fraction (the
    // daemon will replace this with a real summarize + reset). Visible immediately
    // in the footer status bar.
    function _compressContext() {
        Status.setContextUsed(Math.max(1500, Math.round(Status.contextUsed * 0.2)));
    }

    // The chip label is the session's canonical title (the same string the
    // list shows), not the first line of the markdown content.
    function _resolveTitle() {
        if (sessionId === "")
            return;
        const t = SessionStore.title(sessionId);
        root.titleResolved((t && t.length > 0) ? t : qsTr("Session"));
    }

    SessionController {
        id: controller
        store: SessionStore
    }

    // The shared submit pipeline: owns the turn (injected into the transcript) and
    // the status-stack todos (rendered by the composer).
    SessionOrchestrator {
        id: orchestrator
        session: controller
        // Daemon Submit/Subscribe engine vs mock simulator, chosen by the app graph.
        turnEngines: TurnEngines
        // Per-session profile drives the turn (#6b): the orchestrator reads THIS session's profile
        // override and resolves it (via Profiles) to the id the node resolves for Submit.profile.
        sessionSettings: SessionSettings
        profileStore: Profiles
        // Route the orchestrator's front-end commands. Session-scoped ones act
        // on this tab's composer/session/status here; window-level ones (help,
        // title, save) bubble up via commandForwarded.
        onCommandRequested: function(command) {
            switch (command) {
            case "theme": root.openSettingsRequested(); break;
            case "distraction": UiSettings.distractionFree = true; break;
            case "model": composer.openModelPicker(); break;
            case "reasoning": composer.session.cycleReasoningEffort(); break;
            case "fast": composer.session.toggleFastMode(); break;
            case "verbose": composer.session.toggleVerbose(); break;
            case "usage": break; // usage is live in the footer status bar
            case "compress": root._compressContext(); break;
            case "find": transcript.openSearch(); break;
            case "clear": clearConfirmDialog.open(); break;
            default: root.commandForwarded(command);
            }
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

    // Open the bound session on realize and whenever it changes.
    onSessionIdChanged: {
        if (sessionId !== "")
            controller.open(sessionId);
        root._resolveTitle();
    }
    Component.onCompleted: {
        if (sessionId !== "")
            controller.open(sessionId);
        root._resolveTitle();
    }

    // Keep the tab title in sync with the session (e.g. a future rename).
    Connections {
        target: controller
        function onSessionChanged() { root._resolveTitle(); }
        function onContentChanged() { root._resolveTitle(); }
    }

    // Re-resolve on every store change too: the node auto-titles a session from its first user
    // message and the roster refetch lands that title in the cache WITHOUT a content change, so
    // the tab chip would otherwise keep its stale "Session" fallback.
    Connections {
        target: SessionStore
        function onChanged() { root._resolveTitle(); }
    }

    // Feed the shared footer status model from this tab's live turn while it is
    // the active tab. usage/context/rateLimit events ride the same stream the
    // transcript ingests; StatusBarModel filters them by type.
    Connections {
        target: orchestrator.turn
        enabled: root.isActive
        function onTurnStarted() {
            Status.setBusy(true);
            Status.setTurnStartedAt(Date.now());
        }
        function onTurnFinished() {
            Status.setBusy(false);
            // Turn-done desktop notification (opt-in): App.notifyGate no-ops while
            // the window is on-screen and active, so this only alerts when hidden.
            if (AppSettings.value("notify/turnDone", false))
                App.notifyGate(SessionStore.title(root.sessionId),
                               qsTr("The turn finished."));
        }
        function onEventsEmitted(events) { Status.applyTurnEvents(events); }
    }

    // Mirror this tab's live subagent counts into the footer Agents item while it
    // is the active tab (the TUI feeds the same counts from updateSubagents()).
    Connections {
        target: orchestrator.subagents
        enabled: root.isActive
        function onCountChanged() {
            Status.agentsRunning = orchestrator.subagents.runningCount;
            Status.agentsFailed = orchestrator.subagents.failedCount;
        }
    }

    // The turn paused for a masked host input (sudo password / secret). Raise the
    // masked prompt; the answer resumes the turn, cancel aborts it. Answered by the
    // mock turn now; the daemon's HostRequestKind replaces this responder later.
    Connections {
        target: orchestrator.turn
        function onHostRequested(kind, prompt) {
            maskedPrompt.kind = kind;
            maskedPrompt.promptText = prompt;
            maskedField.text = "";
            maskedPrompt.open();
            maskedField.forceActiveFocus();
        }
        // A gate (approval/clarify/host) raises an OS notification when the window
        // is hidden so the user knows the turn is waiting on them. App.notifyGate
        // no-ops when the window is on-screen and active (the inline gate is shown).
        function onAwaitingInput(kind) {
            // Honor the "notify when a turn needs my input" preference.
            if (!AppSettings.value("notify/gates", true))
                return;
            const what = kind === "approval" ? qsTr("needs your approval")
                                             : qsTr("needs a credential");
            App.notifyGate(SessionStore.title(root.sessionId),
                           qsTr("The turn %1.").arg(what));
        }
    }

    // On (re)activation, resync the footer to THIS tab's live turn state so the
    // running indicator/timer reflect the foreground session.
    onIsActiveChanged: {
        if (!isActive)
            return;
        Status.setBusy(orchestrator.turn.active);
        if (orchestrator.turn.active)
            Status.setTurnStartedAt(Date.now() - orchestrator.turn.elapsedMs);
        Status.agentsRunning = orchestrator.subagents.runningCount;
        Status.agentsFailed = orchestrator.subagents.failedCount;
    }

    // Ctrl+F / Cmd+F opens the transcript find bar for this tab.
    Shortcut {
        sequences: [StandardKey.Find]
        onActivated: transcript.openSearch()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: controller.hasSession

        Transcript {
            id: transcript
            Layout.fillWidth: true
            Layout.fillHeight: true
            // The session whose SessionLogEntry sequence the transcript renders.
            sessionId: root.sessionId
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
                function onSessionChanged() { transcript.load(controller.content); }
                function onContentChanged() {
                    // While THIS tab's turn streams, the live document is authoritative and a
                    // store-driven reload would cancel the stream and replace it with the cache
                    // projection (the daemon roster refetch emits changed() mid-turn once the
                    // user's Command echo lands in the block cache). Session switches above
                    // still reload (and cancel) unconditionally.
                    if (orchestrator.turn && orchestrator.turn.active)
                        return;
                    transcript.load(controller.content);
                }
            }
        }

        // CON-8 (A7): the send-time missing-provider nudge. When a turn fails with the node's
        // "no model provider configured" error, surface an actionable deep link back into the
        // wizard's provider step (FirstRun.reenterProvider re-opens the gate at `inference`)
        // instead of leaving a dead chat. The failed message stays in the composer history for
        // retry after the fix.
        Rectangle {
            visible: orchestrator.turn && orchestrator.turn.errorText
                     && orchestrator.turn.errorText.indexOf("no model provider configured") !== -1
            Layout.fillWidth: true
            implicitHeight: nudgeRow.implicitHeight + 16
            radius: 8
            color: Theme.surface
            border.color: Theme.border
            border.width: 1
            RowLayout {
                id: nudgeRow
                anchors.fill: parent
                anchors.margins: 8
                spacing: 10
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Set up a provider to send your first message.")
                    font.family: FontIcons.display; font.pixelSize: 12; color: Theme.text
                    wrapMode: Text.WordWrap
                }
                Kit.TextButton {
                    text: qsTr("Set up provider")
                    accentFilled: true
                    onClicked: if (typeof FirstRun !== "undefined" && FirstRun)
                                   FirstRun.reenterProvider()
                }
            }
        }

        Composer {
            id: composer
            Layout.fillWidth: true
            visible: !root.readOnly
            centerContent: UiSettings.centerText
            busy: transcript.busy
            sessionId: controller.currentId
            todosModel: orchestrator.todos
            subagentsModel: orchestrator.subagents

            onSubmitted: function(text, attachmentRefs) {
                orchestrator.submit(text, attachmentRefs);
                root.committed();
            }
            onSteer: function(text) { orchestrator.steer(text); }
            onCancelRequested: orchestrator.interrupt()
            onCommandInvoked: function(command) { orchestrator.invokeCommand(command); }
        }

        // [wave2:app-delegation] F1: read-only drill-in footer (in place of the composer). Steering
        // a child stays on the Fleet rows, not here.
        Rectangle {
            Layout.fillWidth: true
            visible: root.readOnly
            implicitHeight: readOnlyRow.implicitHeight + 16
            color: Theme.surface
            border.color: Theme.border
            RowLayout {
                id: readOnlyRow
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8
                Kit.Glyph { glyph: FontIcons.fa_eye; color: Theme.textMuted; font.pointSize: 11 }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Read-only view of a delegated child. Steer or cancel it from the Fleet page.")
                    font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    // Empty state: shown until the controller has a session.
    Column {
        anchors.centerIn: parent
        visible: !controller.hasSession
        spacing: Theme.spacing

        Kit.Glyph {
            anchors.horizontalCenter: parent.horizontalCenter
            glyph: FontIcons.fa_comments
            font.pointSize: 36 + Theme.pointSizeOffset
            color: Theme.border
        }

        QQC.Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Select a session")
            color: Theme.textMuted
            font.family: FontIcons.display
            font.pixelSize: 16
        }
    }

    // --- HITL surfaces ------------------------------------------------------
    // Destructive-confirm before clearing the session (/clear).
    Kit.Dialog {
        id: clearConfirmDialog
        title: qsTr("Clear session")
        width: 360
        acceptText: qsTr("Clear")
        destructive: true
        onAccepted: controller.updateContent("")
        contentItem: QQC.Label {
            text: qsTr("Remove all messages from this session? This cannot be undone.")
            wrapMode: Text.WordWrap
            color: Theme.text
            font.family: FontIcons.display
            font.pixelSize: 13
        }
    }

    // Masked host-input prompt (sudo password / secret). A UI shell answered by
    // the mock turn via resume(); the daemon's host channel replaces it later.
    Kit.Dialog {
        id: maskedPrompt
        property string kind: "password"
        property string promptText: ""
        title: kind === "secret" ? qsTr("Secret required") : qsTr("Password required")
        closePolicy: QQC.Popup.NoAutoClose
        width: 420
        acceptText: qsTr("Submit")

        // Forward the typed secret to the parked Input host-request (empty requestId
        // resolves to the engine's pending gate); the mock maps respondInput onto resume().
        onAccepted: orchestrator.turn.respondInput("", maskedField.text)
        onRejected: orchestrator.cancel()

        contentItem: ColumnLayout {
            spacing: 8
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Kit.Glyph {
                    glyph: maskedPrompt.kind === "secret" ? FontIcons.fa_key : FontIcons.fa_lock
                    font.pointSize: 13 + Theme.pointSizeOffset
                    color: Theme.accent
                }
                QQC.Label {
                    Layout.fillWidth: true
                    text: maskedPrompt.promptText
                    wrapMode: Text.WordWrap
                    color: Theme.text
                    font.family: FontIcons.display
                    font.pixelSize: 13
                }
            }
            Kit.TextField {
                id: maskedField
                Layout.fillWidth: true
                underline: true
                echoMode: TextInput.Password
                placeholderText: maskedPrompt.kind === "secret" ? qsTr("Paste secret\u2026")
                                                                : qsTr("Password\u2026")
                onAccepted: maskedPrompt.accept()
            }
            QQC.Label {
                text: qsTr("Never stored - forwarded to the host for this step only.")
                color: Theme.textMuted
                font.family: FontIcons.display
                font.pixelSize: 10
            }
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
