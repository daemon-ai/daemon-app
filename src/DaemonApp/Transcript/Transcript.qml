import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.kde.syntaxhighlighting

import DaemonApp.Theme
import DaemonApp.Settings
import DaemonApp.BlockEditor

// Renders (and edits) the conversation's markdown as a virtualized column of
// blocks via the ported BlockEditor engine. The host calls load() on conversation
// change; user edits are debounced and surfaced through edited(markdown).
Rectangle {
    id: root

    color: Theme.background

    // Kept for compatibility; the renderer is driven by load(), not this binding.
    property string content: ""

    // The markdown currently loaded, so redundant reloads (e.g. open() fires both
    // contentChanged and conversationChanged) don't reset the model twice.
    property string _loadedMarkdown

    // Emitted (debounced) when the user edits a block, carrying the full markdown.
    signal edited(string markdown)

    // Replace the document with `md` without triggering edited() (loads do not
    // funnel through the edit chokepoint, so no echo back to the store). Loads
    // read-first (no auto-focus) so a conversation switch never drops a cursor
    // into the first block or arms a focus callLater that the next reset races.
    function load(md) {
        if (md === _loadedMarkdown)
            return
        _loadedMarkdown = md
        editor.loadMarkdown(md, false)
        if (UiSettings.showPlainText)
            plainText.text = md
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

    // Shared modal image lightbox for image blocks and tool image results.
    Lightbox {
        id: lightbox
    }

    // Mock agent host: stands in for the daemon runtime so the interactive
    // blocks (clarify / approval) round-trip end-to-end in the demo. A real host
    // would forward these answers to the gateway and stream the follow-up turn.
    Connections {
        target: editor

        function onClarifyAnswered(blockId, requestId, answer) {
            editor.ingestEvents([
                { type: "text", text: "\n\nThanks — proceeding with: " + answer + "\n" },
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
    }

    // Coalesce a burst of edits into a single persist of the exported markdown.
    Timer {
        id: persistTimer
        interval: 400
        onTriggered: root.edited(editor.exportMarkdown())
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

            // Mirror the delegate's content column width so percent-sized inline
            // images resolve against the same basis the block images use.
            readonly property real blockContentWidth: Math.max(0, width - 2 * Theme.blockPadding)
            onBlockContentWidthChanged: editor.blockModel.contentWidth = blockContentWidth

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
                        if (editor.activeBlockRow === row)
                            editorView.positionViewAtIndex(row, ListView.Contain)
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
            }

            // Follow the growing tail while streaming, but only when the user is
            // already parked near the bottom.
            Connections {
                target: editor
                function onStreamContentAppended() {
                    if (!editor.streaming && !editorView.atYEnd)
                        return
                    const distanceFromBottom = editorView.contentHeight - (editorView.contentY + editorView.height)
                    if (editorView.atYEnd || distanceFromBottom < editorView.height * 0.5)
                        Qt.callLater(editorView.positionViewAtEnd)
                }
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
}
