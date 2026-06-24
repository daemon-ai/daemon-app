import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Dialogs
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.ComposerSession
import DaemonApp.Files

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

    // When true, the composer's inner content (status stack + input surface) is
    // clamped to contentMaxWidth and centered, so it lines up with the
    // transcript's centered column. The docked bar background still spans the
    // full width. Defaults keep the content full-width (host opts in).
    property bool centerContent: false
    property real contentMaxWidth: 720

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
    readonly property int queueCount: controller.queueCount
    // The status-stack todo list (a TodoListModel owned by the host/orchestrator).
    property var todosModel: null
    // The live subagent rows (a SubagentModel owned by the host/orchestrator).
    property var subagentsModel: null

    // The shared session controller, exposed so the host can drive model/mode
    // commands (palette + slash) without re-plumbing each setter.
    readonly property alias session: controller

    function focusInput() { inputArea.forceActiveFocus(); }
    // Open the model picker overlay (the same overlay the model pill opens), so
    // "/model" and the command palette can raise it.
    function openModelPicker() { controls.openModelPicker(); }

    color: Theme.surface
    implicitHeight: mainColumn.implicitHeight + 2 * Theme.spacing

    // --- Session controller (shared C++ FSM) --------------------------------
    // Owns draft/queue/history/attachments + the submit/queue/steer/cancel/drain
    // state machine; this QML keeps only the visual concerns (field + caret,
    // completion popover, chips, layout, key routing). It is bound from the public
    // API above, and its outbound signals are relayed to this composer's own
    // signals so the host (and tests) keep their existing connections.
    ComposerSessionController {
        id: controller
        busy: root.busy
        enabled: root.composerEnabled
        conversationId: root.conversationId
        // Single source of truth for the model list + active model: the shared
        // Models-hub catalog (installed models + active id), so the composer
        // picker, the Models hub, and Settings -> Model default all agree.
        modelSource: ModelCatalog
        onSubmitted: function(text, refs) { root.submitted(text, refs); }
        onSteer: function(text) { root.steer(text); }
        onCancelRequested: root.cancelRequested()
        onCommandInvoked: function(command) { root.commandInvoked(command); }
        // Programmatic draft changes (history recall, conversation swap, clear,
        // queue edit) replace the field text with the caret at the end.
        onDraftReset: function(text) { root.setText(text); }
        // A completion accept lands the caret at a specific offset (emitted right
        // after draftReset so this overrides the caret-to-end above).
        onCursorRequested: function(pos) { inputArea.cursorPosition = pos; }
    }

    // --- Local attachment pickers -------------------------------------------
    // FileDialog/FolderDialog-backed pickers (same dialect as the transcript
    // export dialog). The picked path is added as an attachment chip and folded
    // into the submitted @file:/@image:/@folder: refs.
    // Daemon-served file picker (browses the node's workspace through the Fs
    // seam, not the local disk): the @file attach path. Picked files are added as
    // chips by their root-relative path.
    FilePicker {
        id: workspaceFilePicker
        onPicked: (rootId, path) => root.addAttachment(path, "file")
    }
    // Daemon-served folder picker: the same FileExplorer in folder-select mode
    // (replaces the unthemed native FolderDialog). Picked dirs attach as @folder:.
    FilePicker {
        id: workspaceFolderPicker
        selectDirs: true
        onPicked: (rootId, path) => root.addAttachment(path + "/", "folder")
    }
    FileDialog {
        id: imagePicker
        title: qsTr("Attach image")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Images (*.png *.jpg *.jpeg *.gif *.webp *.bmp *.svg)"),
                      qsTr("All files (*)")]
        onAccepted: root.addAttachment(root._baseName(selectedFile), "image")
    }

    // Stack on touch so the finger-sized menu/controls never crowd the input on
    // narrow phone widths; also stack when very narrow or multiline.
    readonly property bool stacked: Theme.touch || root.width < 380 || inputArea.lineCount > 1

    // --- Helpers (view-side) ------------------------------------------------
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

    // --- Intents (delegated to the shared controller) -----------------------
    function submitDraft() { controller.submit(); }
    function queueDraft() { controller.enqueueDraft(); }
    function steerDraft() { controller.steerDraft(); }
    function sendNowEntry(index) { controller.sendNowEntry(index); }
    function deleteEntry(index) { controller.deleteEntry(index); }
    function beginEdit(index) { controller.beginEdit(index); inputArea.forceActiveFocus(); }
    function exitEdit(restore) { controller.exitEdit(restore); }
    // History recall (returns true to swallow the key).
    function browseUp() { return controller.browseUp(); }
    function browseDown() { return controller.browseDown(); }

    // --- Completion ---------------------------------------------------------
    // The trigger detection, list filtering, navigation, and accept transforms all
    // live in the shared controller now. The view only pushes text/caret in
    // (refreshTrigger), routes keys to the controller, and renders the popover.
    function refreshTrigger() {
        controller.refreshTrigger(inputArea.text, inputArea.cursorPosition);
    }

    // --- Attachments --------------------------------------------------------
    // The basename of a file: URL (the chip label + the @file:/@image: ref body).
    function _baseName(u) {
        const s = String(u);
        const i = s.lastIndexOf("/");
        return i >= 0 ? s.slice(i + 1) : s;
    }
    function addAttachment(name, kind) {
        controller.addAttachment(name, kind);
    }

    function _basename(path) {
        var s = String(path);
        var slash = Math.max(s.lastIndexOf("/"), s.lastIndexOf("\\"));
        return slash >= 0 ? s.slice(slash + 1) : s;
    }

    // Per-conversation draft/queue swap, history, the idle drain, and dismissing
    // the completion popover on conversation change all live in the controller now.

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
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: Theme.spacing
        anchors.bottomMargin: Theme.spacing
        anchors.horizontalCenter: parent.horizontalCenter
        // Full-width (minus margins) by default; clamp + center the content to
        // contentMaxWidth when the host enables centering. The bar background and
        // top hairline still span the full width.
        width: root.centerContent
            ? Math.min(parent.width - 2 * Theme.spacing, root.contentMaxWidth)
            : parent.width - 2 * Theme.spacing
        spacing: Theme.spacingSmall

        StatusStack {
            id: statusStack
            Layout.fillWidth: true
            queueModel: controller.queue
            todoModel: root.todosModel
            subagentModel: root.subagentsModel
            busy: root.busy
            editingIndex: controller.editingIndex
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
                    visible: controller.editingIndex >= 0
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

                // Reverse-search prompt: readline's "(reverse-i-search)`query':",
                // shown while Ctrl+R search is active (red when the query fails).
                Rectangle {
                    Layout.fillWidth: true
                    visible: controller.reverseSearching
                    implicitHeight: 30
                    radius: Theme.radius
                    color: Theme.activeBlockBackground
                    border.width: 1
                    border.color: controller.reverseSearchFound ? Theme.activeBlockBorder
                                                                 : Theme.danger

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        spacing: 6

                        QQC.Label {
                            Layout.fillWidth: true
                            text: (controller.reverseSearchFound
                                       ? qsTr("(reverse-i-search)`")
                                       : qsTr("(failed reverse-i-search)`"))
                                  + controller.reverseSearchQuery + qsTr("':")
                            font.family: FontIcons.display
                            font.pixelSize: 12
                            color: controller.reverseSearchFound ? Theme.accent : Theme.danger
                            elide: Text.ElideRight
                        }
                        QQC.Label {
                            text: qsTr("Enter accept \u00b7 Ctrl+R next \u00b7 Esc cancel")
                            font.pixelSize: 11
                            color: Theme.textMuted
                        }
                    }
                }

                // Attachment chips.
                Flow {
                    Layout.fillWidth: true
                    visible: controller.attachments.count > 0
                    spacing: 6

                    Repeater {
                        model: controller.attachments
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
                                onRemoveRequested: controller.attachments.removeAt(chipCell.index)
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
                        // Real local pickers: the chosen path becomes a @file:/
                        // @image:/@folder: ref on submit (the actual upload/read
                        // stays daemon-coupled). URL has no native picker.
                        onRequestFiles: workspaceFilePicker.open()
                        onRequestFolder: workspaceFolderPicker.open()
                        onRequestImages: imagePicker.open()
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

                            // Themed right-click context menu (suppress Qt's
                            // default Basic-styled one and open the kit menu).
                            QQC.ContextMenu.menu: null
                            TapHandler {
                                acceptedButtons: Qt.RightButton
                                onTapped: composerEditMenu.popup()
                            }
                            Kit.EditMenu {
                                id: composerEditMenu
                                target: inputArea
                            }

                            // Push live edits into the controller (it guards
                            // against echoing the value back), then refresh the
                            // completion trigger. The controller is the source of
                            // truth for the draft; this field is its view.
                            onTextChanged: {
                                controller.draft = inputArea.text;
                                root.refreshTrigger();
                            }
                            onCursorPositionChanged: root.refreshTrigger()

                            Keys.priority: Keys.BeforeItem
                            Keys.onPressed: function(event) {
                                // Reverse incremental history search (Ctrl+R) owns
                                // all keys while active; the matched entry previews
                                // in this field via the controller's draftReset.
                                if (controller.reverseSearching) {
                                    if (event.key === Qt.Key_Backspace) {
                                        controller.reverseSearchBackspace();
                                        event.accepted = true;
                                        return;
                                    }
                                    if ((event.modifiers & Qt.ControlModifier)
                                            && event.key === Qt.Key_R) {
                                        controller.reverseSearchNext();
                                        event.accepted = true;
                                        return;
                                    }
                                    if (((event.modifiers & Qt.ControlModifier)
                                            && event.key === Qt.Key_G)
                                            || event.key === Qt.Key_Escape) {
                                        controller.reverseSearchCancel();
                                        event.accepted = true;
                                        return;
                                    }
                                    if (event.key === Qt.Key_Return
                                            || event.key === Qt.Key_Enter) {
                                        controller.reverseSearchAccept();
                                        event.accepted = true;
                                        return;
                                    }
                                    if (event.text.length === 1 && event.text >= " "
                                            && !(event.modifiers
                                                & (Qt.ControlModifier | Qt.AltModifier))) {
                                        controller.reverseSearchType(event.text);
                                        event.accepted = true;
                                        return;
                                    }
                                    // Any other key accepts the match, then falls
                                    // through to normal handling below.
                                    controller.reverseSearchAccept();
                                }

                                // Ctrl+R enters reverse incremental search.
                                if ((event.modifiers & Qt.ControlModifier)
                                        && event.key === Qt.Key_R) {
                                    controller.reverseSearchStart();
                                    event.accepted = true;
                                    return;
                                }

                                // Completion popover navigation takes priority.
                                if (controller.completionActive) {
                                    if (event.key === Qt.Key_Down) {
                                        controller.moveActive(1);
                                        event.accepted = true;
                                        return;
                                    }
                                    if (event.key === Qt.Key_Up) {
                                        controller.moveActive(-1);
                                        event.accepted = true;
                                        return;
                                    }
                                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter
                                            || event.key === Qt.Key_Tab) {
                                        controller.acceptActive();
                                        event.accepted = true;
                                        return;
                                    }
                                    if (event.key === Qt.Key_Escape) {
                                        controller.closeTrigger();
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
                                    if (inputArea.text.trim().length === 0 || controller.browsing()) {
                                        if (root.browseUp()) {
                                            event.accepted = true;
                                            return;
                                        }
                                    }
                                }

                                if (event.key === Qt.Key_Down) {
                                    if (controller.browsing()) {
                                        root.browseDown();
                                        event.accepted = true;
                                        return;
                                    }
                                }

                                if (event.key === Qt.Key_Escape) {
                                    if (controller.editingIndex >= 0) {
                                        root.exitEdit(true);
                                        event.accepted = true;
                                        return;
                                    }
                                    if (root.busy) {
                                        controller.cancel();
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
                        primaryAction: controller.primaryAction
                        primaryActionEnabled: controller.primaryActionEnabled
                        canSteer: controller.canSteer
                        composerEnabled: root.composerEnabled
                        session: controller
                        modelList: controller.models
                        currentModelIndex: controller.currentModelIndex
                        onSend: root.submitDraft()
                        onQueue: root.queueDraft()
                        onStop: controller.cancel()
                        onSteer: root.steerDraft()
                        onModelSelected: function(index) { controller.selectModel(index); }
                    }
                }
            }

            // Completion popover, docked above the surface. Driven entirely by the
            // shared controller's completion state.
            CompletionPopover {
                id: completion
                y: -height - 6
                x: 0
                width: Math.min(360, surface.width)
                visible: controller.completionActive
                items: controller.completionItems
                activeIndex: controller.completionActiveIndex
                kind: controller.completionKind
                onPicked: function(index) { controller.accept(index); }
                onHovered: function(index) { controller.setActiveIndex(index); }
            }
        }
    }
}
