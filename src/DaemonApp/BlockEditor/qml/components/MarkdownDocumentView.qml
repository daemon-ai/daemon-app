import QtQuick
import QtQuick.Controls

import DaemonApp.Theme
import DaemonApp.Settings
import DaemonApp.BlockEditor
import DaemonApp.Editor

// Shared markdown document host. Markdown is rendered with the BlockEditor by
// default everywhere; the global "Show Raw Markdown" setting switches the same
// document to the native source editor.
Rectangle {
    id: root

    property bool isCurrent: true
    property string fileName: "document.md"
    property int contentMargin: 0
    property bool modified: false
    property bool _loading: false
    property var _bytes: null

    color: Theme.background

    function loadBytes(bytes) {
        _loading = true;
        _bytes = bytes;
        editor.loadMarkdownBytes(bytes);
        if (UiSettings.showRawMarkdown)
            _loadSource(bytes);
        modified = false;
        _loading = false;
    }

    function textBytes() {
        if (UiSettings.showRawMarkdown && sourceLoader.item)
            return sourceLoader.item.controller.textBytes();
        return editor.exportMarkdownBytes();
    }

    function markSaved(revision) {
        modified = false;
        if (sourceLoader.item)
            sourceLoader.item.controller.markSaved(revision || "");
    }

    function _loadSource(bytes) {
        if (sourceLoader.item)
            sourceLoader.item.controller.loadBytes(bytes, fileName, "");
    }

    function _syncForMode() {
        if (!isCurrent)
            return;
        _loading = true;
        if (UiSettings.showRawMarkdown) {
            _loadSource(editor.exportMarkdownBytes());
        } else if (sourceLoader.item) {
            editor.loadMarkdownBytes(sourceLoader.item.controller.textBytes());
        } else if (_bytes !== null) {
            editor.loadMarkdownBytes(_bytes);
        }
        _loading = false;
    }

    EditorController {
        id: editor
        codeBackgroundColor: Theme.codeBackground
        linkColor: Theme.link
        bodyTextColor: Theme.text
        monoFamily: FontIcons.mono
        bodyFontFamily: UiSettings.editorFontFamily
        bodyFontSize: UiSettings.editorFontSize
        onDocumentChanged: {
            if (!root._loading && !root.modified)
                root.modified = true;
        }
        onImagePreviewRequested: (url, alt) => lightbox.show(url, alt)
    }

    Lightbox { id: lightbox }

    Loader {
        id: sourceLoader
        anchors.fill: parent
        active: root.isCurrent && root._bytes !== null && UiSettings.showRawMarkdown
        visible: active
        sourceComponent: sourceComponent
        onLoaded: root._syncForMode()
    }

    Component {
        id: sourceComponent
        FocusScope {
            property alias controller: sourceCtl
            focus: true
            CodeEditorController { id: sourceCtl }
            CodeEditor {
                anchors.fill: parent
                controller: sourceCtl
                focus: true
            }
            Connections {
                target: sourceCtl
                function onModifiedChanged() {
                    if (!root._loading && sourceCtl.modified)
                        root.modified = true;
                }
            }
        }
    }

    ListView {
        id: blocks
        anchors.fill: parent
        anchors.margins: root.contentMargin
        visible: root.isCurrent && root._bytes !== null && !UiSettings.showRawMarkdown
        clip: true
        model: editor.blockModel
        reuseItems: true
        cacheBuffer: Math.max(0, height * 1.25)
        boundsBehavior: Flickable.DragOverBounds
        flickableDirection: Flickable.VerticalFlick
        spacing: 0

        readonly property real blockContentWidth: Math.max(0, width - 2 * Theme.blockPadding)
        onBlockContentWidthChanged: editor.blockModel.contentWidth = blockContentWidth
        Component.onCompleted: editor.blockModel.contentWidth = blockContentWidth

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            width: 10
        }

        delegate: BlockDelegate {
            width: ListView.view.width
            editorController: editor
        }
    }

    Connections {
        target: UiSettings
        function onShowRawMarkdownChanged() {
            root._syncForMode();
        }
    }
}
