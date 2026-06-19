import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import DaemonApp.Theme
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
    }

    EditorController {
        id: editor

        // Live theme palette for the rendered RichText (recolors on theme switch).
        codeBackgroundColor: Theme.codeBackground
        linkColor: Theme.link
        bodyTextColor: Theme.text
        monoFamily: FontIcons.mono

        onDocumentChanged: persistTimer.restart()
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
            sequences: [StandardKey.Cancel]
            onActivated: editor.clearSelection()
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
            anchors.fill: parent
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
    }
}
