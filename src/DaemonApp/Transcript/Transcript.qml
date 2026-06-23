import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.kde.syntaxhighlighting

import DaemonApp.Theme
import DaemonApp.Settings
import DaemonApp.BlockEditor
import DaemonApp.Turn
import DaemonApp.Controls as Kit

// Renders (and edits) the conversation's markdown as a virtualized column of
// blocks via the ported BlockEditor engine. The host calls load() on conversation
// change; user edits are debounced and surfaced through edited(markdown).
Rectangle {
    id: root

    color: Theme.background

    // Kept for compatibility; the renderer is driven by load(), not this binding.
    property string content: ""

    // The shared turn-lifecycle FSM, injected by the host (owned by the
    // ConversationOrchestrator). The transcript is a pure consumer: it reads
    // active/turnState/elapsedMs/errorText and feeds eventsEmitted into the editor.
    property TurnController turn: null

    // True while a simulated assistant turn is running. The composer binds this
    // to drive queue-while-busy and the Stop affordance; a real gateway would
    // feed the same signal from its streaming state.
    readonly property bool busy: !!turn && turn.active

    // Interrupt the running turn (composer Stop button / Esc).
    function stopTurn() {
        if (turn)
            turn.cancel();
    }

    // The markdown currently loaded, so redundant reloads (e.g. open() fires both
    // contentChanged and conversationChanged) don't reset the model twice.
    property string _loadedMarkdown

    // Emitted (debounced) when the user edits a block, carrying the full markdown.
    signal edited(string markdown)

    // Re-run the assistant turn after a rewind (edit / regenerate / restore). The
    // host routes this to ConversationOrchestrator.rerun: the document was already
    // truncated (and, for edit/restore, the user message re-added) by the editor,
    // so this only re-runs the turn without re-appending a user message.
    signal rerunRequested(string text)

    // Slash-command rewind helpers, acting on the live editor's last user message.
    // retryLast restores it (re-run with its own text); editLast truncates it and
    // returns its text so the host can seed the composer; undoLast drops the last
    // exchange. They funnel through the same DocumentStore primitives the inline
    // affordances use, so there is one truncation path.
    function retryLast() {
        const id = editor.lastUserMessageId()
        if (id !== "")
            editor.restoreToMessage(id)
    }
    function editLast() {
        const id = editor.lastUserMessageId()
        return id !== "" ? editor.editFromMessage(id) : ""
    }
    function undoLast() {
        const id = editor.lastUserMessageId()
        if (id !== "")
            editor.undoToMessage(id)
    }

    // Replace the document with `md` without triggering edited() (loads do not
    // funnel through the edit chokepoint, so no echo back to the store). Loads
    // read-first (no auto-focus) so a conversation switch never drops a cursor
    // into the first block or arms a focus callLater that the next reset races.
    function load(md) {
        if (md === _loadedMarkdown)
            return
        // Abort any in-flight simulated turn so its scheduled events can't land
        // in the freshly-loaded conversation.
        if (turn)
            turn.cancel()
        _loadedMarkdown = md
        editor.loadMarkdown(md, false)
        if (UiSettings.showPlainText)
            plainText.text = md
        // Open at the bottom (latest message), like a chat transcript. A reset
        // lands the view at the top and block heights settle asynchronously, so
        // re-pin across a few frames until the layout stops growing.
        editorView.stickToBottom = true
        settleTimer.restart()
    }

    // Turn start (Hermes' runStart) force-locks the view to the bottom regardless
    // of where the post-send reload left it. The orchestrator starts the turn; we
    // only react to it here. Wired to `turn.turnStarted` below.
    function _onTurnStarted() {
        editorView.stickToBottom = true
        editorView.pinToBottom()
    }

    EditorController {
        id: editor

        // Live theme palette for the rendered RichText (recolors on theme switch).
        codeBackgroundColor: Theme.codeBackground
        linkColor: Theme.link
        bodyTextColor: Theme.text
        monoFamily: FontIcons.mono

        // Editor text style + size from the settings menu.
        bodyFontFamily: UiSettings.editorFontFamily
        bodyFontSize: UiSettings.editorFontSize

        onDocumentChanged: persistTimer.restart()

        // Open the shared lightbox when a block (image / generated image) asks
        // to preview an image.
        onImagePreviewRequested: (url, alt) => lightbox.show(url, alt)
    }

    // --- In-transcript find -------------------------------------------------
    // Whether the find bar is showing, and the block row of the active match
    // (emphasised by the delegate; -1 when there is no current match). Reading the
    // notifying currentMatch/matchCount properties makes the binding re-evaluate
    // as the user cycles matches or the document re-collects (currentBlockIndex()
    // is a plain invokable with no change signal of its own).
    property bool searchActive: false
    property int searchMatchRow: {
        editor.search.currentMatch;
        editor.search.matchCount;
        return editor.search.currentBlockIndex();
    }

    // Reveal the find bar (Ctrl+F / menu / /find) and focus the field.
    function openSearch() {
        searchActive = true
        searchField.forceActiveFocus()
        searchField.selectAll()
    }
    // Hide the bar, drop the query + matches, and return focus to the transcript.
    function closeSearch() {
        editor.search.clear()
        searchActive = false
        editorView.forceActiveFocus()
    }

    // Anchor-scroll the matched block into view (same primitive the heading-link
    // navigation uses), escaping the bottom lock so the jump sticks.
    Connections {
        target: editor.search
        function onNavigateTo(blockIndex, charOffset) {
            if (blockIndex < 0)
                return
            editorView.stickToBottom = false
            editorView.currentIndex = blockIndex
            editorView._pinning = true
            editorView.positionViewAtIndex(blockIndex, ListView.Contain)
            Qt.callLater(function() { editorView._pinning = false })
        }
    }

    // Shared modal image lightbox for image blocks and tool image results.
    Lightbox {
        id: lightbox
    }

    // The injected turn (owned by the ConversationOrchestrator) streams a simulated
    // assistant turn (reasoning / tool / text) through the editor's ingest path;
    // the chrome overlay below reads its turnState/elapsedMs/errorText. The FSM
    // lives in the shared C++ TurnController (DaemonApp.Turn), which emits daemon-
    // shaped event maps fed straight into the editor's ingest path.
    Connections {
        target: root.turn
        function onEventsEmitted(events) { editor.ingestEvents(events); }
        function onTurnStarted() { root._onTurnStarted(); }
    }

    // Mock agent host: stands in for the daemon runtime so the interactive
    // blocks (clarify / approval) round-trip end-to-end in the demo. A real host
    // would forward these answers to the gateway and stream the follow-up turn.
    Connections {
        target: editor

        function onClarifyAnswered(blockId, requestId, answers) {
            // `answers` maps each question id to a string (single-select/freeform)
            // or a string list (multi-select); flatten it to a readable summary.
            var parts = []
            for (var key in answers) {
                var value = answers[key]
                parts.push(Array.isArray(value) ? value.join(", ") : value)
            }
            editor.ingestEvents([
                { type: "text", text: "\n\nThanks — proceeding with: " + parts.join("; ") + "\n" },
                { type: "flush" }
            ])
        }

        function onToolApprovalAnswered(blockId, callId, decision, permanent) {
            if (decision === "approved") {
                editor.updateTypedBlock(blockId, {
                    status: "ok",
                    durationMs: 1400,
                    detailKind: "ansi-stream",
                    stdout: "\u001b[32m\u2713\u001b[0m approved — command finished\n"
                })
            }
        }

        // Footer "regenerate": the controller has already dropped the assistant
        // reply; stream a fresh one. A real host would re-send the prior prompt.
        function onRegenerateRequested(messageId) {
            root.rerunRequested("")
        }

        // Inline edit / restore: the controller truncated the doc to the user
        // message and re-added its text; stream the assistant reply for it.
        function onUserMessageEdited(messageId, text) {
            root.rerunRequested(text)
        }

        // An inline message editor opened: escape stick-to-bottom (and stop the
        // settle pass) so the growing edit composer isn't yanked to the bottom.
        // Committing re-runs the turn (which re-locks); cancelling stays escaped.
        function onInlineEditOpened() {
            settleTimer.stop()
            editorView.stickToBottom = false
        }
    }

    // Coalesce a burst of edits into a single persist of the exported markdown.
    Timer {
        id: persistTimer
        interval: 400
        onTriggered: root.edited(editor.exportMarkdown())
    }

    // Settle pass after a conversation load: a fresh model lands at the top and
    // block heights resolve over several frames, so re-pin to the bottom until
    // the content height stops growing (or a frame cap), then stop.
    Timer {
        id: settleTimer
        property int _ticks: 0
        property real _lastHeight: -1
        interval: 16
        repeat: true
        onRunningChanged: if (running) { _ticks = 0; _lastHeight = -1 }
        onTriggered: {
            if (!editorView.stickToBottom) {
                stop()
                return
            }
            editorView.pinToBottom()
            const h = editorView.contentHeight
            _ticks += 1
            // Stop once the layout is stable for a tick, or after ~30 frames.
            if ((h === _lastHeight && _ticks > 1) || _ticks > 30)
                stop()
            _lastHeight = h
        }
    }

    EditorSurface {
        id: editorSurface
        anchors.fill: parent

        Shortcut {
            sequences: [StandardKey.Copy]
            onActivated: editor.copySelectionToClipboard()
        }
        Shortcut {
            sequences: [StandardKey.SelectAll]
            onActivated: editor.selectAll()
        }
        Shortcut {
            // Escape exits distraction-free first (its chrome, incl. the settings
            // menu, is hidden); otherwise it clears the selection as usual. Owning
            // it here avoids an ambiguous Escape overload with a window shortcut.
            sequences: [StandardKey.Cancel]
            onActivated: {
                if (UiSettings.distractionFree)
                    UiSettings.distractionFree = false
                else
                    editor.clearSelection()
            }
        }
        Shortcut {
            sequences: [StandardKey.Undo]
            onActivated: editor.undo()
        }
        Shortcut {
            sequences: [StandardKey.Redo]
            onActivated: editor.redo()
        }

        ListView {
            id: editorView
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            // "Center text": clamp to a readable column centered in the surface;
            // otherwise fill the available width.
            width: UiSettings.centerText ? Math.min(parent.width, 720) : parent.width
            visible: !UiSettings.showPlainText
            clip: true
            model: editor.blockModel
            reuseItems: true
            // Guard against the first layout pass, where the EditorSurface page
            // margins can make this view's height transiently negative (a negative
            // cacheBuffer corrupts reuseItems realization and throws focus errors).
            cacheBuffer: Math.max(0, height * 1.25)
            boundsBehavior: Flickable.DragOverBounds
            flickableDirection: Flickable.VerticalFlick
            spacing: 0

            // --- Stick-to-bottom (mirrors Hermes' use-stick-to-bottom) ---------
            // While locked, content growth pins the view to the bottom; scrolling
            // up past the threshold escapes, scrolling back within it re-locks.
            property bool stickToBottom: true
            // The user message currently open for inline edit (turn-level), or ""
            // when none. Drives the BlockDelegate edit/collapse render per turn.
            property string editingMessageId: ""
            readonly property int bottomThresholdPx: 72
            // Set while we move the view ourselves, so onContentYChanged can tell a
            // programmatic scroll from a user gesture (only the latter toggles lock).
            property bool _pinning: false

            function distanceFromBottom() {
                return contentHeight - (contentY + height)
            }
            function pinToBottom() {
                _pinning = true
                positionViewAtEnd()
                // Release the guard after this turn of the event loop so the
                // contentY settle from positioning isn't read as a user scroll.
                Qt.callLater(function() { editorView._pinning = false })
            }
            // Only a genuine user gesture may clear the lock. Programmatic pins
            // (positionViewAtEnd) and late-resolving block heights also shift
            // contentY, but must never unlock following; otherwise async layout
            // settle reads as the user escaping the bottom. This mirrors
            // use-stick-to-bottom owning scrollTop as the single writer and only
            // escaping on a real scroll-up.
            function _syncLockFromPosition() {
                if (_pinning)
                    return
                if (!draggingVertically && !flickingVertically && !vScroll.pressed)
                    return
                stickToBottom = distanceFromBottom() <= bottomThresholdPx
            }

            onContentYChanged: _syncLockFromPosition()
            // At rest after a gesture, re-evaluate unconditionally so returning to
            // the bottom re-locks (and scrolling away stays escaped).
            onMovementEnded: {
                if (!_pinning)
                    stickToBottom = distanceFromBottom() <= bottomThresholdPx
            }

            // Mirror the delegate's content column width so percent-sized inline
            // images resolve against the same basis the block images use.
            readonly property real blockContentWidth: Math.max(0, width - 2 * Theme.blockPadding)
            onBlockContentWidthChanged: editor.blockModel.contentWidth = blockContentWidth

            // Track async block sizing during a turn: while locked, keep the
            // bottom pinned as delegate heights resolve and the stream grows.
            onContentHeightChanged: {
                if (stickToBottom && !_pinning)
                    pinToBottom()
            }

            // The always-present view owns scroll-to-active: driving currentIndex
            // here closes the virtualization race (row realized before delegate).
            Connections {
                target: editor
                function onActiveBlockRowChanged() {
                    const row = editor.activeBlockRow
                    if (row < 0)
                        return
                    editorView.currentIndex = row
                    Qt.callLater(function() {
                        if (editor.activeBlockRow === row) {
                            // A cursor-driven scroll is programmatic; guard it so
                            // it isn't misread as the user escaping the bottom lock.
                            editorView._pinning = true
                            editorView.positionViewAtIndex(row, ListView.Contain)
                            Qt.callLater(function() { editorView._pinning = false })
                        }
                    })
                }
            }

            Component.onCompleted: {
                editor.blockModel.contentWidth = blockContentWidth
                if (editor.activeBlockRow >= 0) {
                    currentIndex = editor.activeBlockRow
                    Qt.callLater(function() {
                        positionViewAtIndex(editor.activeBlockRow, ListView.Contain)
                    })
                }
            }

            ScrollBar.vertical: ScrollBar {
                id: vScroll
                policy: ScrollBar.AsNeeded
                interactive: true
                width: 10

                contentItem: Rectangle {
                    implicitWidth: 6
                    radius: width / 2
                    color: vScroll.pressed ? Theme.text : Theme.mutedText
                    opacity: vScroll.active ? 0.6 : 0.0
                    Behavior on opacity { NumberAnimation { duration: 150 } }
                }

                background: Rectangle {
                    implicitWidth: 10
                    radius: width / 2
                    color: Theme.surfaceRaised
                    border.color: Theme.border
                    border.width: Theme.hairline
                    opacity: vScroll.active ? 1.0 : 0.0
                    Behavior on opacity { NumberAnimation { duration: 150 } }
                }
            }

            delegate: BlockDelegate {
                width: ListView.view.width
                editorController: editor
                turnRunning: root.busy
                editingMessageId: editorView.editingMessageId
                // Emphasise the block holding the active find match.
                searchMatch: root.searchActive && index === root.searchMatchRow
                onEditRequested: function(id) {
                    editor.notifyInlineEditOpen()
                    editorView.editingMessageId = id
                }
                onEditFinished: editorView.editingMessageId = ""
            }

            // Follow the growing tail while locked: each streamed chunk re-pins
            // the bottom. Escaping (scroll-up) clears stickToBottom and stops it.
            Connections {
                target: editor
                function onStreamContentAppended() {
                    if (editorView.stickToBottom)
                        editorView.pinToBottom()
                }
            }
        }

        // Empty-thread intro (inventory #21): a centered welcome shown when the
        // conversation has no blocks, replacing the bare empty column.
        Column {
            id: emptyIntro
            anchors.centerIn: parent
            width: Math.min(parent.width - Theme.spacingLarge * 2, 420)
            spacing: Theme.smallSpacing
            visible: !UiSettings.showPlainText && editorView.count === 0 && !root.busy

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: FontIcons.fa_comments
                font.family: FontIcons.faSolid
                font.pixelSize: 32
                color: Theme.mutedText
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Start the conversation")
                color: Theme.text
                font.family: FontIcons.display
                font.pixelSize: Theme.bodyFontSize + 3
                font.bold: true
            }
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Send a message below to begin. The agent's reasoning, tool calls, and replies will stream in here.")
                color: Theme.mutedText
                font.family: FontIcons.display
                font.pixelSize: Theme.captionFontSize
                wrapMode: Text.Wrap
            }
        }

        // "Show plain text": a raw-markdown editor that replaces the block view.
        // Content syncs with the block editor whenever the option is toggled.
        ScrollView {
            id: plainScroll
            anchors.fill: parent
            visible: UiSettings.showPlainText
            clip: true

            TextArea {
                id: plainText
                wrapMode: TextArea.Wrap
                background: null
                color: Theme.text
                selectionColor: Theme.selection
                selectedTextColor: Theme.selectionText
                font.family: UiSettings.editorFontFamily !== "" ? UiSettings.editorFontFamily
                                                                : FontIcons.mono
                font.pixelSize: UiSettings.editorFontSize

                // Themed right-click context menu (suppress Qt's default one).
                ContextMenu.menu: null
                TapHandler {
                    acceptedButtons: Qt.RightButton
                    onTapped: plainEditMenu.popup()
                }
                Kit.EditMenu {
                    id: plainEditMenu
                    target: plainText
                }

                onTextChanged: {
                    if (UiSettings.showPlainText && activeFocus)
                        plainPersist.restart()
                }

                // Highlight the raw document as Markdown (the definition also
                // colors fenced code blocks inside it), so "Show plain text"
                // matches the highlighting of the block view. The app theme picks
                // a light/dark highlighting theme so it recolors on theme switch.
                SyntaxHighlighter {
                    textEdit: plainText
                    repository: Repository
                    definition: Repository.definitionForName("Markdown")
                    theme: Theme.isDarkMode
                        ? Repository.defaultTheme(Repository.DarkTheme)
                        : Repository.defaultTheme(Repository.LightTheme)
                }
            }
        }

        // Persist edits made in the plain-text editor (its text is the source of
        // truth while plain-text mode is on, not the block model).
        Timer {
            id: plainPersist
            interval: 400
            onTriggered: root.edited(plainText.text)
        }

        // Keep the two editors in sync across the toggle: capture the block
        // markdown when entering plain text; push edits back on exit.
        Connections {
            target: UiSettings
            function onShowPlainTextChanged() {
                if (UiSettings.showPlainText) {
                    plainText.text = editor.exportMarkdown()
                } else {
                    const md = plainText.text
                    root._loadedMarkdown = md
                    editor.loadMarkdown(md, false)
                    root.edited(md)
                }
            }
        }
    }

    // Streaming chrome (loading + elapsed, stall, error), overlaid on the block
    // view and driven by the turn simulator. Pure overlay: no input handlers.
    ConversationChrome {
        anchors.fill: parent
        visible: !UiSettings.showPlainText
        active: root.busy
        turnState: root.turn ? root.turn.turnState : ""
        elapsedMs: root.turn ? root.turn.elapsedMs : 0
        errorText: root.turn ? root.turn.errorText : ""
    }

    // Floating jump-to-bottom affordance: shown only while the user has scrolled
    // away from the bottom (lock escaped). Clicking re-locks and snaps down. Its
    // presence is the explicit "not following" signal.
    Rectangle {
        id: jumpButton
        visible: !UiSettings.showPlainText && !editorView.stickToBottom
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: Theme.spacing + 12
        anchors.bottomMargin: Theme.spacing
        width: 32
        height: 32
        radius: width / 2
        color: jumpHover.hovered ? Theme.pressed : Theme.toolSurface
        border.width: Theme.hairline
        border.color: Theme.toolBorder
        opacity: visible ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: Theme.motionFast } }

        Text {
            anchors.centerIn: parent
            text: FontIcons.fa_angles_down
            font.family: FontIcons.faSolid
            font.pixelSize: Theme.captionFontSize
            color: Theme.accent
        }

        HoverHandler { id: jumpHover }
        TapHandler {
            onTapped: {
                editorView.stickToBottom = true
                editorView.pinToBottom()
            }
        }
    }

    // Floating find bar (Ctrl+F / Settings "Search transcript" / /find): a one-
    // line query field with a live match counter and prev/next cycling, bound to
    // the editor's TranscriptSearchController. Esc / x close and clear.
    Rectangle {
        id: searchBar
        visible: root.searchActive
        z: 100
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: Theme.smallSpacing
        anchors.rightMargin: Theme.spacing + 12
        width: 360
        height: 40
        radius: Theme.radius
        color: Theme.surfaceRaised
        border.color: Theme.border
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 4
            spacing: 4

            Kit.Glyph {
                glyph: FontIcons.fa_magnifying_glass
                font.pointSize: 11 + Theme.pointSizeOffset
                color: Theme.textMuted
            }

            Kit.TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Find in transcript")
                text: editor.search.query
                onTextEdited: editor.search.query = text
                Keys.onEscapePressed: root.closeSearch()
                Keys.onReturnPressed: function(e) {
                    if (e.modifiers & Qt.ShiftModifier)
                        editor.search.previous()
                    else
                        editor.search.next()
                }
                Keys.onEnterPressed: function(e) {
                    if (e.modifiers & Qt.ShiftModifier)
                        editor.search.previous()
                    else
                        editor.search.next()
                }
            }

            Text {
                Layout.rightMargin: 2
                text: editor.search.matchCount > 0
                      ? (editor.search.currentMatch + 1) + "/" + editor.search.matchCount
                      : (editor.search.query.length > 0 ? qsTr("0/0") : "")
                color: Theme.textMuted
                font.family: FontIcons.display
                font.pixelSize: Theme.captionFontSize
            }

            Kit.IconButton {
                implicitWidth: 26
                implicitHeight: 26
                icon: FontIcons.fa_chevron_up
                iconPointSize: 11
                enabled: editor.search.matchCount > 0
                tooltipText: qsTr("Previous match (Shift+Enter)")
                onClicked: editor.search.previous()
            }
            Kit.IconButton {
                implicitWidth: 26
                implicitHeight: 26
                icon: FontIcons.fa_chevron_down
                iconPointSize: 11
                enabled: editor.search.matchCount > 0
                tooltipText: qsTr("Next match (Enter)")
                onClicked: editor.search.next()
            }
            Kit.IconButton {
                implicitWidth: 26
                implicitHeight: 26
                icon: FontIcons.fa_xmark
                iconPointSize: 12
                tooltipText: qsTr("Close (Esc)")
                onClicked: root.closeSearch()
            }
        }
    }
}
