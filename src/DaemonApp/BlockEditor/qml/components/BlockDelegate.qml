import QtQuick
import QtQuick.Controls

import DaemonApp.Theme

Item {
    id: root

    required property int index
    required property var blockId
    required property int blockType
    required property string blockTypeName
    required property string markdown
    required property string displayMarkup
    required property real heightHint
    required property int projectionRevision
    required property int indent
    required property var tableData
    required property var mermaidData
    required property var codeData
    required property var imageData
    required property var mathData
    required property var toolData
    required property var reasoningData
    required property var contentData
    required property string messageRole
    required property string messageId
    required property bool messageFirst
    required property bool messageLast
    required property var editorController
    // True while an assistant turn is streaming. The footer is suppressed during
    // a turn so it doesn't hop between blocks as the tail grows (each inserted
    // block would otherwise relocate ~40px of footer right at the pinned bottom).
    property bool turnRunning: false

    // BlockType enum values for list items (BulletListItem=2, OrderedListItem=3,
    // TaskListItem=4); list policy lives in the store/controller, the delegate
    // only routes keys and presents indent/spacing.
    readonly property bool isListItem: blockType >= 2 && blockType <= 4
    readonly property int indentDepth: Math.floor(indent / 2)
    readonly property int indentStep: 20
    readonly property int verticalPadding: isListItem ? 2 : Theme.blockPadding

    // BlockType::Table enum index (mirrors the numeric isListItem convention).
    // hasTable gates the structured grid render; a degenerate/in-progress edit
    // yields empty tableData and falls back to raw text.
    readonly property bool isTable: blockType === 7
    readonly property bool hasTable: isTable && tableData && tableData.columns > 0

    // A code fence whose language is mermaid renders as a native diagram when
    // passive; activating the block reveals the raw fenced source for editing.
    readonly property bool hasMermaid: !!(mermaidData
        && mermaidData.language === "mermaid"
        && mermaidData.source
        && mermaidData.source.length > 0)

    // BlockType::Image enum index (9). hasImage gates the native Image renderer;
    // a mid-edit block with no resolvable url falls back to raw text.
    readonly property bool hasImage: blockType === 9
        && !!(imageData
        && imageData.url
        && imageData.url.length > 0)

    // Math renders as a centered formula image when passive: a ```math fence or
    // a whole-block $$...$$. Activating the block reveals the raw markdown for
    // editing, exactly like mermaid. buildMathData yields the source for both.
    readonly property bool hasMath: !!(mathData
        && mathData.source
        && mathData.source.length > 0)

    // BlockType::CodeFence enum index (6). A non-mermaid code fence renders as a
    // syntax-highlighted code card when passive (mermaid fences are gated above
    // and route to MermaidBlock); activating the block reveals the raw source.
    readonly property bool hasCode: blockType === 6
        && !hasMermaid
        && !hasMath
        && !!(codeData
        && codeData.code
        && codeData.code.length > 0)

    // Agent transcript blocks. BlockType enum indices: Reasoning=13, ToolCall=14,
    // Content=15 (appended after the markdown types). Each renders as a passive
    // structured card; activating the block reveals its raw fenced markdown.
    readonly property bool hasReasoning: blockType === 13
    readonly property bool hasTool: blockType === 14
    readonly property bool hasContent: blockType === 15

    // Message/role layer (Strategy C). A user block renders in a glass bubble; a
    // system block renders as a centered notice (a "process:"-prefixed system
    // message becomes a collapsible process notification); assistant/un-roled
    // blocks render as before, with a footer after the last block of the run.
    readonly property bool isUser: messageRole === "user"
    readonly property bool isSystem: messageRole === "system"
    readonly property bool isAssistant: messageRole === "assistant"
    readonly property bool isProcessNotice: isSystem && markdown.trim().startsWith("process:")
    // First block of a turn gets extra top spacing so turns read as grouped.
    readonly property int turnGap: messageFirst ? Theme.contentSpacing : 0

    property bool isPooled: false
    property bool isActive: editorController.activeBlockId === Number(blockId)
    property var selectionSpan: editorController.selectionSpanForBlock(Number(blockId), index, passiveText.length)

    // Item-space rectangles painted behind the glyphs for the current selection.
    // C++ returns per-line rects in document-layout space (document margin
    // included); we add the TextEdit leftPadding/topPadding to reach item space.
    // Active blocks edit raw markdown, so they paint against activeEditor with
    // raw offsets; passive blocks paint against passiveText with visual offsets.
    // Structured tables are opaque to cross-renderer selection (no text doc).
    // Re-evaluates when the span, content width, or wrapped height changes.
    property var selectionRects: {
        if (!selectionSpan || selectionSpan.kind <= 0)
            return []
        // Passive structured tables self-paint per cell (see TableBlock.qml); an
        // active table edits as raw markdown, so it uses the active raw path below.
        if (!root.isActive && (root.hasTable || root.hasImage))
            return []
        const fullBlock = selectionSpan.kind === 1
        const textItem = root.isActive ? activeEditor : passiveText
        if (!textItem.textDocument)
            return []
        // Touch geometry-affecting properties so the binding recomputes on re-wrap.
        const _w = textItem.width
        const _h = textItem.contentHeight
        const start = fullBlock ? 0 : (root.isActive ? selectionSpan.rawStart : selectionSpan.start)
        const end = fullBlock ? textItem.length : (root.isActive ? selectionSpan.rawEnd : selectionSpan.end)
        const rects = root.editorController.activeTextController.lineRectsForRange(textItem.textDocument, start, end)
        const out = []
        for (let i = 0; i < rects.length; ++i) {
            const r = rects[i]
            out.push({ x: r.x + textItem.leftPadding, y: r.y + textItem.topPadding, width: r.width, height: r.height })
        }
        return out
    }
    property real lastReportedHeight: 0
    property point pendingActivationPoint: Qt.point(-1, -1)
    property bool highlightRefreshPending: false
    property int markdownHeadingLevel: {
        const match = markdown.match(/^(#{1,6})\s+/)
        return match ? match[1].length : 0
    }

    width: ListView.view ? ListView.view.width : implicitWidth
    implicitHeight: Math.max(contentColumn.implicitHeight + verticalPadding * 2 + turnGap, 1)
    height: implicitHeight

    ListView.onPooled: {
        isPooled = true
        activeEditor.focus = false
        // activeEditor.visible no longer toggles (the container owns visibility),
        // so detach the layout controller here when the delegate is recycled.
        editorController.activeTextController.detach(activeEditor.textDocument)
    }

    ListView.onReused: {
        isPooled = false
        if (root.isActive)
            root.queueInitialActiveSetup()
    }

    Component.onCompleted: {
        if (root.isActive)
            root.queueInitialActiveSetup()
    }

    onIsActiveChanged: {
        if (isActive) {
            // Backup only: Main.qml drives currentIndex from controller.activeBlockRow.
            // This covers the already-realized case; it is not the primary path.
            if (root.ListView.view)
                root.ListView.view.currentIndex = index
            root.queueInitialActiveSetup()
        } else {
            highlightRefreshPending = false
            initialActiveSetupTimer.stop()
            highlightRefreshTimer.stop()
            editorController.activeTextController.detach(activeEditor.textDocument)
        }
    }

    Timer {
        id: initialActiveSetupTimer
        interval: 0
        repeat: false

        onTriggered: {
            if (!root.isActive || root.isPooled || !activeEditor.visible || !activeEditor.textDocument)
                return
            editorController.activeTextController.attach(activeEditor.textDocument)
            activeEditor.forceActiveFocus()
            activeEditor.cursorPosition = Math.max(0, Math.min(editorController.activeCursorOffset, activeEditor.text.length))
            root.queueActiveHighlighting()
            Qt.callLater(root.ensureCaretVisible)
        }
    }

    Timer {
        id: highlightRefreshTimer
        interval: 0
        repeat: false

        onTriggered: {
            highlightRefreshPending = false
            if (!root.isActive || root.isPooled || !activeEditor.visible || !activeEditor.textDocument)
                return
            editorController.activeTextController.attach(activeEditor.textDocument)
            editorController.activeTextController.applyMarkdownFormatting(activeEditor.text, blockType, root.markdownHeadingLevel)
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusSmall
        color: root.isActive ? Theme.activeBlockBackground : Theme.transparent
        border.width: root.isActive ? 1 : 0
        border.color: Theme.activeBlockBorder
    }

    Column {
        id: contentColumn
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: root.verticalPadding + root.turnGap
        anchors.leftMargin: Theme.blockPadding
        anchors.rightMargin: Theme.blockPadding
        spacing: Theme.smallSpacing

        Text {
            id: typeLabel
            visible: root.isActive
            text: blockTypeName
            color: Theme.mutedText
            font.pixelSize: Theme.captionFontSize
        }

        Item {
            id: passiveContainer
            visible: !root.isActive && !root.hasTable && !root.hasMermaid && !root.hasImage && !root.hasCode && !root.hasMath
                     && !root.hasReasoning && !root.hasTool && !root.hasContent && !root.isUser && !root.isSystem
            width: parent.width
            implicitHeight: passiveText.implicitHeight
            height: implicitHeight

            // Selection highlight, declared first so it paints behind the glyphs
            // (siblings drawn later, like passiveText, render on top).
            Repeater {
                model: root.selectionRects
                delegate: Rectangle {
                    required property var modelData
                    x: modelData.x
                    y: modelData.y
                    width: modelData.width
                    height: modelData.height
                    color: Theme.selection
                    opacity: 0.35
                }
            }

            TextEdit {
                id: passiveText
                width: parent.width
                // Read-only TextEdit (not Text) so we get positionAt hit-testing
                // and a QTextDocument for selection geometry. Native selection is
                // disabled; cross-block selection is painted via root.selectionRects.
                readOnly: true
                selectByMouse: false
                activeFocusOnPress: false
                // Show nesting via a left margin instead of raw spaces (the projector
                // strips leading whitespace). The active editor keeps the raw spaces.
                leftPadding: root.indentDepth * root.indentStep
                text: displayMarkup
                textFormat: TextEdit.RichText
                wrapMode: TextEdit.Wrap
                color: Theme.text
                font.family: editorController.bodyFontFamily !== "" ? editorController.bodyFontFamily
                                                                    : FontIcons.display
                font.pixelSize: editorController.bodyFontSize
                renderType: Text.QtRendering
            }

            // Passive click + cross-block drag selection. preventStealing keeps the
            // drag from being hijacked by the enclosing ListView/Flickable (so the
            // gesture selects instead of scrolling, and the delegate is not recycled
            // mid-drag). The active block has its own activeSelect MouseArea, so this
            // is disabled there; tables are opaque (no passive text to hit-test).
            MouseArea {
                id: passiveSelect
                anchors.fill: parent
                enabled: !root.isActive && !root.hasTable && !root.hasMermaid && !root.hasImage && !root.hasMath
                         && !root.hasReasoning && !root.hasTool && !root.hasContent && !root.isUser && !root.isSystem
                acceptedButtons: Qt.LeftButton
                preventStealing: true
                hoverEnabled: true
                cursorShape: hoverLink !== "" ? Qt.PointingHandCursor : Qt.IBeamCursor

                property bool dragging: false
                property int pressOffset: 0
                property real pressXLocal: 0
                property real pressYLocal: 0
                property int clickStreak: 0
                property string hoverLink: ""
                property string pressLink: ""

                function linkAtMouse(mouse) {
                    const p = mapToItem(passiveText, mouse.x, mouse.y)
                    return passiveText.linkAt(p.x, p.y)
                }

                Timer {
                    id: passiveClickTimer
                    interval: 350
                    onTriggered: passiveSelect.clickStreak = 0
                }

                onExited: hoverLink = ""
                onPressed: mouse => {
                    dragging = false
                    pressXLocal = mouse.x
                    pressYLocal = mouse.y
                    pressLink = linkAtMouse(mouse)
                    pressOffset = root.selectionOffsetAt(passiveText, mapToItem(null, mouse.x, mouse.y))
                }
                onPositionChanged: mouse => {
                    if (!pressed) {
                        hoverLink = linkAtMouse(mouse)
                        return
                    }
                    if (!dragging) {
                        if (Math.abs(mouse.x - pressXLocal) < 3 && Math.abs(mouse.y - pressYLocal) < 3)
                            return
                        dragging = true
                        root.editorController.beginSelection(root.index, pressOffset)
                    }
                    root.extendSelectionToScenePoint(mapToItem(null, mouse.x, mouse.y))
                }
                onReleased: mouse => {
                    if (dragging)
                        return
                    // Open a link only on a stationary click whose press and release
                    // land on the same anchor, so a selection gesture that starts on
                    // a link never opens it.
                    if (pressLink !== "" && linkAtMouse(mouse) === pressLink) {
                        root.activateLink(pressLink)
                        return
                    }
                    clickStreak = passiveClickTimer.running ? clickStreak + 1 : 1
                    passiveClickTimer.restart()
                    root.resolveSelectionClick(false, pressOffset, mouse, Qt.point(mapToItem(root, mouse.x, mouse.y).x, mapToItem(root, mouse.x, mouse.y).y), clickStreak)
                }
            }
        }

        Item {
            id: tableContainer
            visible: !root.isActive && root.hasTable
            width: parent.width
            implicitHeight: tableLoader.implicitHeight
            height: implicitHeight

            Loader {
                id: tableLoader
                anchors.left: parent.left
                anchors.right: parent.right
                active: tableContainer.visible
                sourceComponent: tableComponent
            }

            // Passive table click + cross-block drag selection. Mirrors passiveSelect:
            // a drag seeds/extends the document selection from per-cell raw offsets;
            // a plain click clears the selection and activates the table for raw editing.
            MouseArea {
                id: tableSelect
                anchors.fill: parent
                enabled: !root.isActive && root.hasTable
                acceptedButtons: Qt.LeftButton
                preventStealing: true
                hoverEnabled: true
                cursorShape: hoverLink !== "" ? Qt.PointingHandCursor : Qt.IBeamCursor

                property bool dragging: false
                property var pressHit: null
                property real pressXLocal: 0
                property real pressYLocal: 0
                property string hoverLink: ""
                property string pressLink: ""

                function linkAtMouse(mouse) {
                    const hit = tableLoader.item
                        ? tableLoader.item.cellHitAt(mapToItem(null, mouse.x, mouse.y))
                        : null
                    return (hit && hit.link) ? hit.link : ""
                }

                onExited: hoverLink = ""
                onPressed: mouse => {
                    dragging = false
                    pressXLocal = mouse.x
                    pressYLocal = mouse.y
                    pressHit = tableLoader.item
                        ? tableLoader.item.cellHitAt(mapToItem(null, mouse.x, mouse.y))
                        : null
                    pressLink = (pressHit && pressHit.link) ? pressHit.link : ""
                }
                onPositionChanged: mouse => {
                    if (!pressed) {
                        hoverLink = linkAtMouse(mouse)
                        return
                    }
                    if (!dragging) {
                        if (Math.abs(mouse.x - pressXLocal) < 3 && Math.abs(mouse.y - pressYLocal) < 3)
                            return
                        if (!pressHit)
                            return
                        dragging = true
                        root.editorController.beginSelectionAtRaw(root.index,
                            root.editorController.tableRawOffsetForCell(Number(blockId), pressHit.rowIndex, pressHit.col, pressHit.inCellVisual))
                    }
                    root.extendSelectionToScenePoint(mapToItem(null, mouse.x, mouse.y))
                }
                onReleased: mouse => {
                    if (dragging)
                        return
                    // Same-link stationary click opens the URL instead of activating.
                    if (pressLink !== "" && linkAtMouse(mouse) === pressLink) {
                        root.activateLink(pressLink)
                        return
                    }
                    root.editorController.clearSelection()
                    root.pendingActivationPoint = Qt.point(mapToItem(root, mouse.x, mouse.y).x, mapToItem(root, mouse.x, mouse.y).y)
                    root.editorController.activateBlockAt(root.index, 0)
                }
            }
        }

        Component {
            id: tableComponent
            TableBlock {
                width: tableLoader.width
                tableData: root.tableData
                editorController: root.editorController
                blockId: root.blockId
            }
        }

        Item {
            id: mermaidContainer
            visible: !root.isActive && root.hasMermaid
            width: parent.width
            implicitHeight: mermaidLoader.implicitHeight
            height: implicitHeight

            Loader {
                id: mermaidLoader
                anchors.left: parent.left
                anchors.right: parent.right
                active: mermaidContainer.visible
                sourceComponent: mermaidComponent
            }
        }

        Component {
            id: mermaidComponent
            MermaidBlock {
                width: mermaidLoader.width
                mermaidData: root.mermaidData
                editorController: root.editorController
                blockId: root.blockId
            }
        }

        Item {
            id: mathContainer
            visible: !root.isActive && root.hasMath
            width: parent.width
            implicitHeight: mathLoader.implicitHeight
            height: implicitHeight

            Loader {
                id: mathLoader
                anchors.left: parent.left
                anchors.right: parent.right
                active: mathContainer.visible
                sourceComponent: mathComponent
            }
        }

        Component {
            id: mathComponent
            MathBlock {
                width: mathLoader.width
                mathData: root.mathData
                editorController: root.editorController
                blockId: root.blockId
            }
        }

        Item {
            id: imageContainer
            visible: !root.isActive && root.hasImage
            width: parent.width
            implicitHeight: imageLoader.implicitHeight
            height: implicitHeight

            Loader {
                id: imageLoader
                anchors.left: parent.left
                anchors.right: parent.right
                active: imageContainer.visible
                sourceComponent: imageComponent
            }
        }

        Component {
            id: imageComponent
            ImageBlock {
                width: imageLoader.width
                imageData: root.imageData
                editorController: root.editorController
                blockId: root.blockId
            }
        }

        Item {
            id: codeContainer
            visible: !root.isActive && root.hasCode
            width: parent.width
            implicitHeight: codeLoader.implicitHeight
            height: implicitHeight

            Loader {
                id: codeLoader
                anchors.left: parent.left
                anchors.right: parent.right
                active: codeContainer.visible
                sourceComponent: codeComponent
            }
        }

        Component {
            id: codeComponent
            CodeBlock {
                width: codeLoader.width
                codeData: root.codeData
                editorController: root.editorController
                blockId: root.blockId
            }
        }

        Item {
            id: reasoningContainer
            visible: !root.isActive && root.hasReasoning
            width: parent.width
            implicitHeight: reasoningLoader.implicitHeight
            height: implicitHeight

            Loader {
                id: reasoningLoader
                anchors.left: parent.left
                anchors.right: parent.right
                active: reasoningContainer.visible
                sourceComponent: reasoningComponent
            }
        }

        Component {
            id: reasoningComponent
            ReasoningBlock {
                width: reasoningLoader.width
                reasoningData: root.reasoningData
                editorController: root.editorController
                blockId: root.blockId
            }
        }

        Item {
            id: toolContainer
            visible: !root.isActive && root.hasTool
            width: parent.width
            implicitHeight: toolLoader.implicitHeight
            height: implicitHeight

            Loader {
                id: toolLoader
                anchors.left: parent.left
                anchors.right: parent.right
                active: toolContainer.visible
                sourceComponent: toolComponent
            }
        }

        Component {
            id: toolComponent
            ToolCallBlock {
                width: toolLoader.width
                toolData: root.toolData
                editorController: root.editorController
                blockId: root.blockId
            }
        }

        Item {
            id: contentContainer
            visible: !root.isActive && root.hasContent
            width: parent.width
            implicitHeight: contentLoader.implicitHeight
            height: implicitHeight

            Loader {
                id: contentLoader
                anchors.left: parent.left
                anchors.right: parent.right
                active: contentContainer.visible
                sourceComponent: contentComponent
            }
        }

        Component {
            id: contentComponent
            ContentBlock {
                width: contentLoader.width
                contentData: root.contentData
                editorController: root.editorController
                blockId: root.blockId
            }
        }

        // --- Message/role layer render paths --------------------------------
        // User message: a glass bubble with the simplified text, directive chips,
        // and an inline edit composer (the passive selection path is disabled for
        // user/system rows above, so these own their own interaction).
        Loader {
            id: userLoader
            width: parent.width
            active: root.isUser && !root.isActive
            visible: active
            sourceComponent: Component {
                UserMessageBubble {
                    markdown: root.markdown
                    displayMarkup: root.displayMarkup
                    editorController: root.editorController
                    messageId: root.messageId
                }
            }
        }

        Loader {
            id: systemLoader
            width: parent.width
            active: root.isSystem && !root.isProcessNotice && !root.isActive
            visible: active
            sourceComponent: Component {
                SystemMessage {
                    markdown: root.markdown
                    editorController: root.editorController
                }
            }
        }

        Loader {
            id: processLoader
            width: parent.width
            active: root.isProcessNotice && !root.isActive
            visible: active
            sourceComponent: Component {
                ProcessNotice {
                    markdown: root.markdown
                    editorController: root.editorController
                }
            }
        }

        Item {
            id: activeContainer
            visible: root.isActive
            width: parent.width
            implicitHeight: activeEditor.implicitHeight
            height: implicitHeight

            // Selection highlight, declared first so it paints behind the active
            // raw glyphs (the caret/text stay visible on top). Built from raw
            // offsets against activeEditor's own QTextDocument.
            Repeater {
                model: root.selectionRects
                delegate: Rectangle {
                    required property var modelData
                    x: modelData.x
                    y: modelData.y
                    width: modelData.width
                    height: modelData.height
                    color: Theme.selection
                    opacity: 0.35
                }
            }

            TextEdit {
                id: activeEditor
                width: parent.width
                text: markdown
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.Wrap
                color: Theme.text
                selectedTextColor: Theme.selectionText
                selectionColor: Theme.selection
                font.family: editorController.bodyFontFamily !== "" ? editorController.bodyFontFamily
                                                                    : FontIcons.display
                font.pixelSize: editorController.bodyFontSize
                // Mouse selection is owned by the unified activeSelect MouseArea so a
                // drag can start here and extend into passive blocks; native keyboard
                // selection (Shift+Arrow) is preserved.
                activeFocusOnPress: false
                persistentSelection: true
                selectByMouse: false

                // Mirror the native (keyboard) selection into the controller so copy
                // can fall back to it when there is no document selection.
                onSelectionStartChanged: {
                    if (root.isActive && activeFocus && !root.isPooled)
                        editorController.setActiveNativeSelection(selectionStart, selectionEnd)
                }
                onSelectionEndChanged: {
                    if (root.isActive && activeFocus && !root.isPooled)
                        editorController.setActiveNativeSelection(selectionStart, selectionEnd)
                }

                onTextChanged: {
                    // Skip echoes from the `text: markdown` binding being re-applied
                    // during model resets after structural edits; only forward genuine
                    // user edits to the model.
                    if (root.isActive && activeFocus && !root.isPooled && text !== markdown) {
                        editorController.replaceBlockMarkdown(Number(blockId), text)
                        root.queueActiveHighlighting()
                    }
                }

                onActiveFocusChanged: {
                    if (activeFocus)
                        root.queueActiveHighlighting()
                }

                onCursorPositionChanged: {
                    if (root.isActive && activeFocus && !root.isPooled)
                        editorController.updateActiveCursorOffset(cursorPosition)
                }

                // Keep the caret on screen as it moves (arrow keys, typing) and
                // as the editor grows (wrapping / appended lines at the bottom).
                onCursorRectangleChanged: {
                    if (root.isActive)
                        Qt.callLater(root.ensureCaretVisible)
                }

                onContentHeightChanged: {
                    if (root.isActive)
                        Qt.callLater(root.ensureCaretVisible)
                }

                Keys.onPressed: event => {
                const commandModifier = (event.modifiers & Qt.ControlModifier) || (event.modifiers & Qt.MetaModifier)
                const shiftModifier = event.modifiers & Qt.ShiftModifier

                if (commandModifier && event.key === Qt.Key_A) {
                    editorController.selectAll()
                    event.accepted = true
                } else if (commandModifier && event.key === Qt.Key_Z) {
                    if (shiftModifier)
                        editorController.redo()
                    else
                        editorController.undo()
                    event.accepted = true
                } else if (commandModifier && event.key === Qt.Key_BracketRight) {
                    if (root.isListItem)
                        editorController.indentListItem(Number(blockId))
                    event.accepted = true
                } else if (commandModifier && event.key === Qt.Key_BracketLeft) {
                    if (root.isListItem)
                        editorController.outdentListItem(Number(blockId))
                    event.accepted = true
                } else if (event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab) {
                    const isOutdent = shiftModifier || event.key === Qt.Key_Backtab
                    if (root.isListItem && isOutdent) {
                        editorController.outdentListItem(Number(blockId))
                        event.accepted = true
                    } else if (root.isListItem) {
                        // Tab indents only at the start of a line; elsewhere it
                        // inserts a literal tab like a normal editor.
                        const before = text.substring(0, cursorPosition)
                        const atLineStart = (cursorPosition === 0) || before.charAt(before.length - 1) === "\n"
                        if (atLineStart)
                            editorController.indentListItem(Number(blockId))
                        else
                            activeEditor.insert(cursorPosition, "\t")
                        event.accepted = true
                    } else {
                        if (!isOutdent)
                            activeEditor.insert(cursorPosition, "\t")
                        event.accepted = true
                    }
                } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    // Tables edit as raw multi-line source like paragraphs: a plain
                    // Enter adds a soft newline (a new row line) and a double-Enter on
                    // an empty trailing line escapes into a new block.
                    const isParagraphLike = (blockTypeName === "paragraph" || blockTypeName === "quote" || root.isTable)
                    if (!isParagraphLike) {
                        // Headings, lists, and code fences keep their existing
                        // Enter semantics (split / continue marker / literal newline).
                        editorController.splitBlock(Number(blockId), cursorPosition)
                        event.accepted = true
                    } else if (shiftModifier) {
                        // Force a soft newline: let the TextEdit insert "\n" itself.
                        event.accepted = false
                    } else {
                        const before = text.substring(0, cursorPosition)
                        const after = text.substring(cursorPosition)
                        const lineStart = before.lastIndexOf("\n") + 1
                        const currentLine = before.substring(lineStart)
                        const atLineEnd = (after.length === 0 || after.charAt(0) === "\n")
                        const onEmptyLine = (currentLine.length === 0) && atLineEnd
                        const trailingDoubleSpace = atLineEnd && /  $/.test(currentLine)
                        if (onEmptyLine || trailingDoubleSpace) {
                            // Blank line (double-Enter) or trailing double-space ->
                            // promote the soft break to a real block split.
                            editorController.splitParagraph(Number(blockId), cursorPosition)
                            event.accepted = true
                        } else {
                            // Soft newline within the paragraph; TextEdit inserts "\n"
                            // and onTextChanged forwards it to the model.
                            event.accepted = false
                        }
                    }
                } else if (event.key === Qt.Key_Backspace && cursorPosition === 0) {
                    Qt.callLater(() => editorController.backspaceAtStart(Number(blockId)))
                    event.accepted = true
                } else if (event.key === Qt.Key_Delete && text.length === 0) {
                    Qt.callLater(() => editorController.deleteBlock(Number(blockId)))
                    event.accepted = true
                } else if (event.key === Qt.Key_Up && editorController.activeTextController.isOnFirstVisualLine(cursorPosition)) {
                    const visualX = editorController.activeTextController.cursorVisualX(cursorPosition)
                    Qt.callLater(() => editorController.moveActiveBlock(-1, EditorController.LastVisualLineAtX, visualX))
                    event.accepted = true
                } else if (event.key === Qt.Key_Down && editorController.activeTextController.isOnLastVisualLine(cursorPosition)) {
                    const visualX = editorController.activeTextController.cursorVisualX(cursorPosition)
                    Qt.callLater(() => editorController.moveActiveBlock(1, EditorController.FirstVisualLineAtX, visualX))
                    event.accepted = true
                } else if (commandModifier && event.key === Qt.Key_C) {
                    // Route copy through the controller so the active editor and the
                    // document selection share one (unified raw markdown) clipboard path.
                    editorController.copySelectionToClipboard()
                    event.accepted = true
                } else if ((commandModifier && event.key === Qt.Key_V)
                           || (shiftModifier && event.key === Qt.Key_Insert)) {
                    // Intercept paste before native TextEdit insertion: parse the
                    // clipboard markdown into blocks instead of dumping it all into
                    // this one block. selectionStart === selectionEnd === cursorPosition
                    // when there is no selection, so this also replaces a selection.
                    editorController.pasteFromClipboard(Number(blockId), activeEditor.selectionStart, activeEditor.selectionEnd)
                    event.accepted = true
                }
                }
            }

            // Mouse selection/click for the active (raw) editor. Mirrors passiveSelect
            // but seeds with raw offsets so a drag can start here and extend into
            // passive blocks; preventStealing keeps the gesture from scrolling.
            MouseArea {
                id: activeSelect
                anchors.fill: activeEditor
                enabled: root.isActive
                acceptedButtons: Qt.LeftButton
                preventStealing: true
                cursorShape: Qt.IBeamCursor

                property bool dragging: false
                property int pressOffset: 0
                property real pressXLocal: 0
                property real pressYLocal: 0
                property int clickStreak: 0

                Timer {
                    id: activeClickTimer
                    interval: 350
                    onTriggered: activeSelect.clickStreak = 0
                }

                onPressed: mouse => {
                    dragging = false
                    pressXLocal = mouse.x
                    pressYLocal = mouse.y
                    pressOffset = root.selectionOffsetAt(activeEditor, mapToItem(null, mouse.x, mouse.y))
                }
                onPositionChanged: mouse => {
                    if (!pressed)
                        return
                    if (!dragging) {
                        if (Math.abs(mouse.x - pressXLocal) < 3 && Math.abs(mouse.y - pressYLocal) < 3)
                            return
                        dragging = true
                        root.editorController.beginSelectionAtRaw(root.index, pressOffset)
                    }
                    root.extendSelectionToScenePoint(mapToItem(null, mouse.x, mouse.y))
                }
                onReleased: mouse => {
                    if (dragging)
                        return
                    clickStreak = activeClickTimer.running ? clickStreak + 1 : 1
                    activeClickTimer.restart()
                    root.resolveSelectionClick(true, pressOffset, mouse, Qt.point(mapToItem(root, mouse.x, mouse.y).x, mapToItem(root, mouse.x, mouse.y).y), clickStreak)
                }
            }
        }

        // Assistant message footer (copy / regenerate / branch) shown once, on
        // the last block of an assistant message and only when not editing.
        Loader {
            id: footerLoader
            width: parent.width
            active: root.isAssistant && root.messageLast && !root.isActive && !root.turnRunning
            visible: active
            sourceComponent: Component {
                AssistantFooter {
                    editorController: root.editorController
                    messageId: root.messageId
                }
            }
        }
    }

    onHeightChanged: {
        if (!isPooled && Math.abs(height - lastReportedHeight) >= 1) {
            lastReportedHeight = height
            editorController.reportBlockHeight(Number(blockId), height)
        }
    }

    Connections {
        target: editorController
        enabled: !root.isPooled
        function onEditorFocusRequested(requestedBlockId, placement, cursorOffset, visualX) {
            if (Number(requestedBlockId) !== Number(blockId))
                return

            if (root.pendingActivationPoint.x >= 0)
                Qt.callLater(function() {
                    root.placeCursorFromPendingPoint()
                })
            else
                root.restoreActiveEditorFocus(placement, cursorOffset, visualX)
        }
        function onSelectionRevisionChanged() {
            root.selectionSpan = editorController.selectionSpanForBlock(Number(blockId), index, passiveText.length)
        }
    }

    // Within-block caret follow only. Cross-block scroll-to-active is owned by
    // the ListView in Main.qml (via controller.activeBlockRow), so this is a
    // slim, stateless nudge: move contentY only when the caret of an already-
    // realized active editor is off-screen. Its targets are block interiors,
    // not the document's extreme edges, so it does not trigger the Flickable
    // returnToBounds fixup that previously caused self-scroll.
    function ensureCaretVisible() {
        if (!root.isActive || root.isPooled)
            return
        const view = root.ListView.view
        if (!view)
            return
        const caretTop = activeEditor.mapToItem(view.contentItem, 0, activeEditor.cursorRectangle.y).y
        const caretBottom = caretTop + activeEditor.cursorRectangle.height
        const margin = Theme.smallSpacing
        // Use originY (non-zero with some ListView layouts) for correct bounds.
        const minContentY = view.originY
        const maxContentY = Math.max(minContentY, view.originY + view.contentHeight - view.height)
        if (caretTop < view.contentY + margin)
            view.contentY = Math.max(minContentY, caretTop - margin)
        else if (caretBottom > view.contentY + view.height - margin)
            view.contentY = Math.min(maxContentY, caretBottom - view.height + margin)
    }

    function restoreActiveEditorFocus(placement, cursorOffset, visualX) {
        if (!root.isActive || root.isPooled)
            return

        Qt.callLater(function() {
            if (!root.isActive || root.isPooled)
                return
            editorController.activeTextController.attach(activeEditor.textDocument)
            activeEditor.forceActiveFocus()
            activeEditor.cursorPosition = Math.max(0, Math.min(root.resolveFocusCursor(placement, cursorOffset, visualX), activeEditor.text.length))
            root.queueActiveHighlighting()
            Qt.callLater(root.ensureCaretVisible)
        })
    }

    function placeCursorFromPendingPoint() {
        if (!root.isActive || root.isPooled || root.pendingActivationPoint.x < 0)
            return

        const pointInEditor = root.mapToItem(activeEditor, root.pendingActivationPoint.x, root.pendingActivationPoint.y)
        editorController.activeTextController.attach(activeEditor.textDocument)
        const cursorOffset = activeEditor.positionAt(pointInEditor.x, pointInEditor.y)
        root.pendingActivationPoint = Qt.point(-1, -1)
        activeEditor.forceActiveFocus()
        activeEditor.cursorPosition = Math.max(0, Math.min(cursorOffset, activeEditor.text.length))
        editorController.updateActiveCursorOffset(activeEditor.cursorPosition)
        root.queueActiveHighlighting()
        Qt.callLater(root.ensureCaretVisible)
    }

    function resolveFocusCursor(placement, cursorOffset, visualX) {
        if (placement === EditorController.Start)
            return 0
        if (placement === EditorController.End)
            return activeEditor.text.length
        if (placement === EditorController.FirstVisualLineAtX)
            return editorController.activeTextController.cursorAtFirstVisualLine(visualX)
        if (placement === EditorController.LastVisualLineAtX)
            return editorController.activeTextController.cursorAtLastVisualLine(visualX)
        return cursorOffset
    }

    // Clamped hit-test: map a scene point into a text item and return the text
    // offset (raw for activeEditor, visual for passiveText). Clamping keeps points
    // dragged outside the glyph rect (padding/empty space) from jumping wildly.
    function selectionOffsetAt(textItem, scenePoint) {
        const local = textItem.mapFromItem(null, scenePoint.x, scenePoint.y)
        const x = Math.max(0, Math.min(local.x, textItem.width))
        const y = Math.max(0, Math.min(local.y, textItem.height))
        return textItem.positionAt(x, y)
    }

    // Extend the document selection head to a scene point landing in THIS delegate.
    // The active row resolves raw offsets, passive rows visual offsets, so a drag
    // mixing both renderers stays correct in either direction. Tables are opaque.
    function extendSelectionHere(scenePoint) {
        // Passive table: resolve the scene point to a cell and extend in raw offsets.
        // (An active table falls through to the active raw editor path below.)
        if (!root.isActive && root.hasTable) {
            const item = tableLoader.item
            if (!item)
                return
            const h = item.cellHitAt(scenePoint)
            if (h)
                root.editorController.updateSelectionAtRaw(root.index,
                    root.editorController.tableRawOffsetForCell(Number(blockId), h.rowIndex, h.col, h.inCellVisual))
            return
        }
        const textItem = root.isActive ? activeEditor : passiveText
        const offset = root.selectionOffsetAt(textItem, scenePoint)
        if (root.isActive)
            root.editorController.updateSelectionAtRaw(root.index, offset)
        else
            root.editorController.updateSelection(root.index, offset)
    }

    // Owner of the drag grab routes each move to whichever delegate is under the
    // cursor (it may be active or passive, above or below the origin).
    // A clicked link: an in-document anchor (#slug) scrolls the list to the
    // matching heading; anything else opens externally. Falls back to external
    // open when the fragment does not resolve to a heading.
    function activateLink(href) {
        if (href.startsWith("#")) {
            const targetRow = root.editorController.rowForAnchor(href)
            if (targetRow >= 0) {
                const view = root.ListView.view
                if (view)
                    view.positionViewAtIndex(targetRow, ListView.Beginning)
                return
            }
        }
        Qt.openUrlExternally(href)
    }

    function extendSelectionToScenePoint(scenePos) {
        const view = root.ListView.view
        if (!view)
            return
        const viewPoint = view.mapFromItem(null, scenePos.x, scenePos.y)
        const contentY = viewPoint.y + view.contentY
        const row = root.editorController.rowAtContentY(contentY)
        if (row < 0)
            return
        const item = view.itemAtIndex(row)
        if (!item)
            return
        item.extendSelectionHere(scenePos)
    }

    // Resolve a non-drag press: Shift extends from the active caret; a 2nd/3rd
    // click selects the word/line; a plain click clears the selection and places
    // the caret (activating a passive block or repositioning within the active one).
    function resolveSelectionClick(isRawRow, offset, mouse, rootPoint, streak) {
        if (mouse.modifiers & Qt.ShiftModifier) {
            root.editorController.extendSelectionFromActive(root.index, offset, isRawRow)
            return
        }
        if (streak >= 3) {
            if (isRawRow)
                root.editorController.selectLineAtRaw(root.index, offset)
            else
                root.editorController.selectLine(root.index, offset)
            return
        }
        if (streak === 2) {
            if (isRawRow)
                root.editorController.selectWordAtRaw(root.index, offset)
            else
                root.editorController.selectWord(root.index, offset)
            return
        }
        root.editorController.clearSelection()
        root.pendingActivationPoint = rootPoint
        if (isRawRow)
            root.placeCursorFromPendingPoint()
        else
            root.editorController.activateBlockAt(root.index, 0)
    }

    function queueInitialActiveSetup() {
        initialActiveSetupTimer.restart()
    }

    function queueActiveHighlighting() {
        if (highlightRefreshPending)
            return
        highlightRefreshPending = true
        highlightRefreshTimer.restart()
    }
}
