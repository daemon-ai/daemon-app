import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// The message composer - the QML port of Hermes' ChatBar
// (apps/desktop/src/app/chat/composer/index.tsx). A docked, full-width bar that
// hosts a status stack (todos + prompt queue) above a bordered input surface.
// The surface holds an attachments row, an optional queue-edit banner, and a
// reflowing menu | input | controls grid that stacks when narrow or multiline.
//
// Owns its own client-side state: per-conversation draft persistence, a sent-
// message history ring, a prompt queue (drained when the turn goes idle), and
// drag-dropped attachment chips. Voice / dictation and a real model/upload
// backend are out of scope (no gateway wired yet).
Rectangle {
    id: root

    // --- Public API ---------------------------------------------------------
    // True while an assistant turn is running (host binds Transcript.busy).
    property bool busy: false
    // The open conversation; drives per-conversation draft/queue/history swap.
    property int conversationId: -1
    property bool composerEnabled: true

    // Emitted to send a turn. `attachmentRefs` is a space-joined @file:/@image:
    // string (empty when none).
    signal submitted(string text, string attachmentRefs)
    // Mid-turn steer (Ctrl+Enter) - text-only nudge without interrupting.
    signal steer(string text)
    // Interrupt the running turn (Esc / Stop button).
    signal cancelRequested()
    // A client-side slash command was picked (e.g. "new", "theme").
    signal commandInvoked(string command)

    // The live draft text (also a test/host seam for setting the input).
    property alias draftText: inputArea.text
    // Number of queued prompts for the active conversation.
    readonly property int queueCount: queueModel.count

    function focusInput() { inputArea.forceActiveFocus(); }

    color: Theme.surface
    implicitHeight: mainColumn.implicitHeight + 2 * Theme.spacing

    // --- Private state ------------------------------------------------------
    // conversationId (as string key) -> draft text.
    property var _drafts: ({})
    // conversationId -> [ { text, refs } ] queued prompts.
    property var _queues: ({})
    // conversationId -> [ sent text ] history ring.
    property var _histories: ({})
    property int _prevConvKey: -1

    // History browse cursor (-1 = not browsing) and the live draft stashed when
    // browsing started.
    property int _browseIndex: -1
    property string _historyDraft: ""

    // Queue edit: index of the entry being edited (-1 = none) and the draft to
    // restore on cancel.
    property int _editingIndex: -1
    property string _preEditDraft: ""

    property bool _draining: false

    readonly property bool stacked: root.width < 380 || inputArea.lineCount > 1
    readonly property bool _hasPayload: inputArea.text.trim().length > 0 || attachmentsModel.count > 0
    readonly property bool _canSteer: root.busy && inputArea.text.trim().length > 0 && attachmentsModel.count === 0

    // --- Models -------------------------------------------------------------
    ListModel { id: queueModel }
    ListModel { id: todoModel }
    ListModel { id: attachmentsModel }

    CompletionProvider { id: completionProvider }

    // --- Helpers ------------------------------------------------------------
    function _key(id) { return id }

    function setText(t) {
        inputArea.text = t;
        inputArea.cursorPosition = t.length;
    }

    function insertAtCursor(t) {
        var pos = inputArea.cursorPosition;
        var s = inputArea.text;
        inputArea.text = s.slice(0, pos) + t + s.slice(pos);
        inputArea.cursorPosition = pos + t.length;
        inputArea.forceActiveFocus();
    }

    function clearComposer() {
        inputArea.clear();
        attachmentsModel.clear();
    }

    function buildRefs() {
        var refs = [];
        for (var i = 0; i < attachmentsModel.count; ++i) {
            var a = attachmentsModel.get(i);
            var prefix = a.kind === "image" ? "@image:"
                       : a.kind === "folder" ? "@folder:"
                       : a.kind === "url" ? "@url:" : "@file:";
            refs.push(prefix + a.name);
        }
        return refs.join(" ");
    }

    function _resetBrowse() {
        _browseIndex = -1;
        _historyDraft = "";
    }

    function _pushHistory(text) {
        var key = _key(conversationId);
        var arr = _histories[key] || [];
        if (arr.length === 0 || arr[arr.length - 1] !== text)
            arr.push(text);
        if (arr.length > 100)
            arr = arr.slice(arr.length - 100);
        _histories[key] = arr;
    }

    function browseUp() {
        var arr = _histories[_key(conversationId)] || [];
        if (arr.length === 0)
            return false;
        if (_browseIndex === -1) {
            _historyDraft = inputArea.text;
            _browseIndex = arr.length - 1;
        } else if (_browseIndex > 0) {
            _browseIndex -= 1;
        } else {
            return true; // at the oldest entry; swallow so the caret doesn't move
        }
        setText(arr[_browseIndex]);
        return true;
    }

    function browseDown() {
        if (_browseIndex === -1)
            return false;
        var arr = _histories[_key(conversationId)] || [];
        if (_browseIndex < arr.length - 1) {
            _browseIndex += 1;
            setText(arr[_browseIndex]);
        } else {
            _browseIndex = -1;
            setText(_historyDraft);
        }
        return true;
    }

    // --- Submit / queue / steer --------------------------------------------
    function submitDraft() {
        if (!composerEnabled)
            return;

        var text = inputArea.text.trim();

        // Saving an in-place queue edit (not a send).
        if (_editingIndex >= 0) {
            if (text.length > 0)
                queueModel.set(_editingIndex, { text: text, refs: buildRefs() });
            exitEdit(false);
            return;
        }

        if (!_hasPayload) {
            // Empty Enter with a non-empty queue drains the next entry.
            if (!busy && queueModel.count > 0)
                _drainNext();
            return;
        }

        var refs = buildRefs();

        if (busy) {
            // Mid-turn: queue the draft instead of sending.
            _enqueue(text, refs);
            clearComposer();
            _resetBrowse();
            return;
        }

        _pushHistory(text);
        clearComposer();
        _resetBrowse();
        root.submitted(text, refs);
    }

    function queueDraft() {
        if (!_hasPayload)
            return;
        _enqueue(inputArea.text.trim(), buildRefs());
        clearComposer();
        _resetBrowse();
    }

    function steerDraft() {
        if (!_canSteer)
            return;
        var text = inputArea.text.trim();
        clearComposer();
        _resetBrowse();
        root.steer(text);
    }

    function _enqueue(text, refs) {
        queueModel.append({ text: text, refs: refs });
    }

    function _drainNext() {
        if (_draining || busy || queueModel.count === 0)
            return;
        _draining = true;
        var entry = queueModel.get(0);
        var text = entry.text;
        var refs = entry.refs;
        queueModel.remove(0);
        if (_editingIndex >= 0)
            _editingIndex -= 1;
        _pushHistory(text);
        root.submitted(text, refs);
        _draining = false;
    }

    // --- Queue row actions --------------------------------------------------
    function sendNowEntry(index) {
        if (index < 0 || index >= queueModel.count)
            return;
        if (busy) {
            // Promote to the head and interrupt; the busy->idle drain sends it.
            if (index !== 0) {
                var e = queueModel.get(index);
                var moved = { text: e.text, refs: e.refs };
                queueModel.remove(index);
                queueModel.insert(0, moved);
            }
            root.cancelRequested();
            return;
        }
        var entry = queueModel.get(index);
        var text = entry.text;
        var refs = entry.refs;
        queueModel.remove(index);
        _pushHistory(text);
        root.submitted(text, refs);
    }

    function deleteEntry(index) {
        if (index < 0 || index >= queueModel.count)
            return;
        if (index === _editingIndex)
            exitEdit(true);
        else if (index < _editingIndex)
            _editingIndex -= 1;
        queueModel.remove(index);
    }

    function beginEdit(index) {
        if (index < 0 || index >= queueModel.count || _editingIndex >= 0)
            return;
        _preEditDraft = inputArea.text;
        _editingIndex = index;
        setText(queueModel.get(index).text);
        inputArea.forceActiveFocus();
    }

    function exitEdit(restore) {
        if (_editingIndex < 0)
            return;
        _editingIndex = -1;
        if (restore)
            setText(_preEditDraft);
        else
            inputArea.clear();
        _preEditDraft = "";
    }

    // --- Completion ---------------------------------------------------------
    property string _triggerKind: ""
    property int _triggerStart: -1

    function refreshTrigger() {
        var pos = inputArea.cursorPosition;
        var before = inputArea.text.slice(0, pos);
        var m = before.match(/(^|\s)([\/@])(\S*)$/);
        if (!m) {
            closeTrigger();
            return;
        }
        var kind = m[2] === "@" ? "mention" : "slash";
        var query = m[3];
        var items = completionProvider.search(kind, query);
        if (items.length === 0) {
            closeTrigger();
            return;
        }
        _triggerKind = kind;
        _triggerStart = pos - (query.length + 1); // include the trigger char
        completion.kind = kind;
        completion.items = items;
        completion.activeIndex = 0;
        if (!completion.visible)
            completion.open();
    }

    function closeTrigger() {
        _triggerKind = "";
        _triggerStart = -1;
        if (completion.visible)
            completion.close();
    }

    function acceptCompletion(item) {
        if (!item) {
            closeTrigger();
            return;
        }
        if (item.action !== undefined && item.action !== "insert") {
            // Strip the typed trigger token, then run the command.
            if (_triggerStart >= 0) {
                var pos = inputArea.cursorPosition;
                var s = inputArea.text;
                inputArea.text = s.slice(0, _triggerStart) + s.slice(pos);
                inputArea.cursorPosition = _triggerStart;
            }
            closeTrigger();
            if (item.action === "clear")
                clearComposer();
            else
                root.commandInvoked(item.action);
            return;
        }
        // Insert: replace the trigger token with the item value.
        var p = inputArea.cursorPosition;
        var text = inputArea.text;
        var start = _triggerStart >= 0 ? _triggerStart : p;
        inputArea.text = text.slice(0, start) + item.value + text.slice(p);
        inputArea.cursorPosition = start + item.value.length;
        closeTrigger();
        inputArea.forceActiveFocus();
    }

    // --- Attachments --------------------------------------------------------
    function addAttachment(name, kind) {
        attachmentsModel.append({ name: name, kind: kind });
    }

    function _basename(path) {
        var s = String(path);
        var slash = Math.max(s.lastIndexOf("/"), s.lastIndexOf("\\"));
        return slash >= 0 ? s.slice(slash + 1) : s;
    }

    // --- Todos (demo seam) --------------------------------------------------
    function setTodos(items) {
        todoModel.clear();
        if (!items)
            return;
        for (var i = 0; i < items.length; ++i)
            todoModel.append({ text: items[i].text, done: items[i].done === true });
    }

    function clearTodos() { todoModel.clear(); }

    // --- Per-conversation state swap ---------------------------------------
    function _stash(key) {
        _drafts[key] = inputArea.text;
        var q = [];
        for (var i = 0; i < queueModel.count; ++i) {
            var e = queueModel.get(i);
            q.push({ text: e.text, refs: e.refs });
        }
        _queues[key] = q;
    }

    function _restore(key) {
        setText(_drafts[key] !== undefined ? _drafts[key] : "");
        queueModel.clear();
        var q = _queues[key] || [];
        for (var i = 0; i < q.length; ++i)
            queueModel.append({ text: q[i].text, refs: q[i].refs });
    }

    onConversationIdChanged: {
        // Editing/browsing state never crosses a conversation boundary.
        _editingIndex = -1;
        _preEditDraft = "";
        _resetBrowse();
        closeTrigger();
        _stash(_key(_prevConvKey));
        _restore(_key(conversationId));
        _prevConvKey = conversationId;
    }

    onBusyChanged: {
        // Flow queued turns whenever the session goes idle.
        if (!busy && queueModel.count > 0)
            Qt.callLater(_drainNext);
    }

    Component.onCompleted: _prevConvKey = conversationId

    // --- Top separator hairline --------------------------------------------
    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: Theme.border
    }

    // --- Layout: status stack + input surface ------------------------------
    ColumnLayout {
        id: mainColumn
        anchors.fill: parent
        anchors.margins: Theme.spacing
        spacing: Theme.spacingSmall

        StatusStack {
            id: statusStack
            Layout.fillWidth: true
            queueModel: queueModel
            todoModel: todoModel
            busy: root.busy
            editingIndex: root._editingIndex
            onSendNow: function(index) { root.sendNowEntry(index); }
            onEditEntry: function(index) { root.beginEdit(index); }
            onDeleteEntry: function(index) { root.deleteEntry(index); }
        }

        Rectangle {
            id: surface
            Layout.fillWidth: true
            implicitHeight: surfaceColumn.implicitHeight + 2 * Theme.spacingSmall
            radius: Theme.radius
            color: Theme.searchBackground
            border.width: 1
            border.color: inputArea.activeFocus ? Theme.searchFocusBorder : Theme.searchBorder

            // Drag-drop: in-app / OS file drops become attachment chips.
            DropArea {
                anchors.fill: parent
                onEntered: function(drag) {
                    if (drag.hasUrls)
                        drag.accept(Qt.CopyAction);
                }
                onDropped: function(drop) {
                    if (!drop.hasUrls)
                        return;
                    for (var i = 0; i < drop.urls.length; ++i) {
                        var name = root._basename(drop.urls[i]);
                        var lower = name.toLowerCase();
                        var isImage = /\.(png|jpe?g|gif|webp|bmp|svg)$/.test(lower);
                        root.addAttachment(name, isImage ? "image" : "file");
                    }
                    drop.accept(Qt.CopyAction);
                    inputArea.forceActiveFocus();
                }
            }

            ColumnLayout {
                id: surfaceColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: Theme.spacingSmall
                spacing: Theme.spacingSmall

                // Queue-edit banner.
                Rectangle {
                    Layout.fillWidth: true
                    visible: root._editingIndex >= 0
                    implicitHeight: 30
                    radius: Theme.radius
                    color: Theme.activeBlockBackground
                    border.width: 1
                    border.color: Theme.activeBlockBorder

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 4
                        spacing: 6

                        QQC.Label {
                            Layout.fillWidth: true
                            text: qsTr("Editing queued message")
                            font.family: FontIcons.display
                            font.pixelSize: 12
                            color: Theme.textMuted
                            elide: Text.ElideRight
                        }
                        Kit.TextButton {
                            text: qsTr("Cancel")
                            font.pixelSize: 12
                            onClicked: root.exitEdit(true)
                        }
                        Kit.TextButton {
                            text: qsTr("Save")
                            font.pixelSize: 12
                            onClicked: root.submitDraft()
                        }
                    }
                }

                // Attachment chips.
                Flow {
                    Layout.fillWidth: true
                    visible: attachmentsModel.count > 0
                    spacing: 6

                    Repeater {
                        model: attachmentsModel
                        delegate: Item {
                            id: chipCell
                            required property int index
                            required property string name
                            required property string kind
                            implicitWidth: chip.implicitWidth
                            implicitHeight: chip.implicitHeight
                            AttachmentChip {
                                id: chip
                                name: chipCell.name
                                kind: chipCell.kind
                                onRemoveRequested: attachmentsModel.remove(chipCell.index)
                            }
                        }
                    }
                }

                // Reflowing control grid: menu | input | controls.
                GridLayout {
                    id: grid
                    Layout.fillWidth: true
                    columns: 3
                    columnSpacing: Theme.spacingSmall
                    rowSpacing: Theme.spacingSmall

                    ComposerMenu {
                        id: menu
                        Layout.row: root.stacked ? 1 : 0
                        Layout.column: 0
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        composerEnabled: root.composerEnabled
                        onRequestFiles: root.addAttachment(qsTr("document.txt"), "file")
                        onRequestFolder: root.addAttachment(qsTr("project/"), "folder")
                        onRequestImages: root.addAttachment(qsTr("screenshot.png"), "image")
                        onRequestUrl: root.addAttachment("example.com", "url")
                        onInsertText: function(text) { root.insertAtCursor(text); }
                    }

                    QQC.ScrollView {
                        id: inputScroll
                        Layout.row: 0
                        Layout.column: root.stacked ? 0 : 1
                        Layout.columnSpan: root.stacked ? 3 : 1
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredHeight: Math.min(Math.max(inputArea.implicitHeight, 26), 140)
                        QQC.ScrollBar.horizontal.policy: QQC.ScrollBar.AlwaysOff

                        QQC.TextArea {
                            id: inputArea
                            placeholderText: qsTr("Send a message...  (Enter to send, Shift+Enter for newline)")
                            placeholderTextColor: Theme.textMuted
                            wrapMode: QQC.TextArea.Wrap
                            color: Theme.text
                            selectionColor: Theme.searchSelection
                            selectedTextColor: Theme.text
                            font.family: FontIcons.display
                            font.pixelSize: 14
                            background: null
                            enabled: root.composerEnabled

                            onTextChanged: root.refreshTrigger()
                            onCursorPositionChanged: root.refreshTrigger()

                            Keys.priority: Keys.BeforeItem
                            Keys.onPressed: function(event) {
                                // Completion popover navigation takes priority.
                                if (completion.visible && completion.items.length > 0) {
                                    var n = completion.items.length;
                                    if (event.key === Qt.Key_Down) {
                                        completion.activeIndex = (completion.activeIndex + 1) % n;
                                        event.accepted = true;
                                        return;
                                    }
                                    if (event.key === Qt.Key_Up) {
                                        completion.activeIndex = (completion.activeIndex - 1 + n) % n;
                                        event.accepted = true;
                                        return;
                                    }
                                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter
                                            || event.key === Qt.Key_Tab) {
                                        root.acceptCompletion(completion.items[completion.activeIndex]);
                                        event.accepted = true;
                                        return;
                                    }
                                    if (event.key === Qt.Key_Escape) {
                                        root.closeTrigger();
                                        event.accepted = true;
                                        return;
                                    }
                                }

                                if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                    if (event.modifiers & Qt.ShiftModifier) {
                                        event.accepted = false; // newline
                                        return;
                                    }
                                    if (event.modifiers & Qt.ControlModifier) {
                                        root.steerDraft();
                                        event.accepted = true;
                                        return;
                                    }
                                    root.submitDraft();
                                    event.accepted = true;
                                    return;
                                }

                                if (event.key === Qt.Key_Up) {
                                    if (inputArea.text.trim().length === 0 || root._browseIndex !== -1) {
                                        if (root.browseUp()) {
                                            event.accepted = true;
                                            return;
                                        }
                                    }
                                }

                                if (event.key === Qt.Key_Down) {
                                    if (root._browseIndex !== -1) {
                                        root.browseDown();
                                        event.accepted = true;
                                        return;
                                    }
                                }

                                if (event.key === Qt.Key_Escape) {
                                    if (root._editingIndex >= 0) {
                                        root.exitEdit(true);
                                        event.accepted = true;
                                        return;
                                    }
                                    if (root.busy) {
                                        root.cancelRequested();
                                        event.accepted = true;
                                        return;
                                    }
                                }
                            }
                        }
                    }

                    // Spacer that pushes the controls to the right in the stacked
                    // layout; collapses (excluded) in the inline layout.
                    Item {
                        Layout.row: 1
                        Layout.column: 1
                        Layout.fillWidth: true
                        visible: root.stacked
                    }

                    ComposerControls {
                        id: controls
                        Layout.row: root.stacked ? 1 : 0
                        Layout.column: 2
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                        busy: root.busy
                        hasPayload: root._hasPayload
                        canSteer: root._canSteer
                        composerEnabled: root.composerEnabled
                        onSend: root.submitDraft()
                        onQueue: root.queueDraft()
                        onStop: root.cancelRequested()
                        onSteer: root.steerDraft()
                    }
                }
            }

            // Completion popover, docked above the surface.
            CompletionPopover {
                id: completion
                y: -height - 6
                x: 0
                width: Math.min(360, surface.width)
                onPicked: function(item) { root.acceptCompletion(item); }
            }
        }
    }
}
