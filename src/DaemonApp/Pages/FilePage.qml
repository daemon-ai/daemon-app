import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import DaemonApp.Theme
import DaemonApp.Editor
import DaemonApp.BlockEditor

// Hosts an open file as a tab: reads bytes through the Fs seam and routes by type
// - markdown -> the shared MarkdownDocumentView (rendered by default, raw source
// when "Show Raw Markdown" is enabled); other text -> the QML-native code editor;
// image/binary -> a placeholder. Saves with optimistic concurrency
// (the held revision) and reloads on external change when not dirty. Dirty/title
// are mirrored back to the TabModel by the Session host.
Item {
    id: root

    property string rootId: ""
    property string path: ""
    property var service: (typeof Fs !== "undefined") ? Fs : null
    property bool isCurrent: true

    signal dirtyChanged(bool dirty)
    signal titleResolved(string title)

    readonly property bool isImage: /\.(png|jpe?g|gif|webp|bmp|svg|ico)$/i.test(path)
    readonly property bool isMarkdown: /\.(md|markdown|mdown|mkd)$/i.test(path)
    property bool binary: false
    property bool truncated: false
    property bool loaded: false
    property bool saving: false
    property string errorText: ""
    property string statusText: ""
    // The fs content revision held as the optimistic-write base (markdown path;
    // the code editor tracks its own revision internally).
    property string _revision: ""
    // The most recent bytes from the Fs read, applied to whichever editor the
    // Loader builds (so content survives the active editor being (re)created).
    property var _pendingBytes: null

    function baseName(p) { const i = p.lastIndexOf('/'); return i >= 0 ? p.substring(i + 1) : p; }
    function parentDir(p) { const i = p.lastIndexOf('/'); return i >= 0 ? p.substring(0, i) : ""; }

    function reload() {
        if (!service || path === "")
            return;
        loaded = false;
        service.read(rootId, path);
    }
    function save() {
        if (!service || !loaded || binary || isImage || truncated)
            return;
        saving = true;
        errorText = "";
        statusText = qsTr("Saving\u2026");
        if (isMarkdown) {
            if (mdLoader.item)
                service.write(rootId, path, mdLoader.item.textBytes(), root._revision, false);
        } else if (codeLoader.item) {
            service.write(rootId, path, codeLoader.item.controller.textBytes(),
                          codeLoader.item.controller.revision, false);
        }
    }

    // Bring the freshly read bytes into the active (Loader-built) editor. A no-op
    // until the Loader has instantiated it; the editor's Component.onCompleted
    // calls this too, covering the create-after-read ordering.
    function _applyContent() {
        if (!root.isCurrent || !loaded || binary || isImage)
            return;
        if (isMarkdown) {
            if (mdLoader.item)
                mdLoader.item.loadBytes(_pendingBytes);
        } else if (codeLoader.item) {
            codeLoader.item.controller.loadBytes(_pendingBytes, path, _revision);
        }
    }

    onPathChanged: { titleResolved(baseName(path)); reload(); }
    onRootIdChanged: reload()
    onIsCurrentChanged: if (isCurrent) Qt.callLater(root._applyContent)
    Component.onCompleted: {
        if (path !== "")
            titleResolved(baseName(path));
        reload();
        if (service && path !== "")
            service.watch(rootId, parentDir(path));
    }

    Connections {
        target: root.service
        enabled: root.service !== null
        function onFileRead(r, p, bytes, revision, isBin, truncated) {
            if (r !== root.rootId || p !== root.path)
                return;
            root.binary = isBin;
            root.truncated = truncated;
            root.errorText = truncated ? qsTr("File is too large for editing and was loaded only partially.") : "";
            root._revision = revision;
            root._pendingBytes = bytes;
            root.loaded = true;
            if (isBin || root.isImage || root.truncated)
                return;
            root._applyContent();
        }
        function onWriteResult(r, p, ok, revision, err) {
            if (r !== root.rootId || p !== root.path)
                return;
            if (!ok) {
                root.saving = false;
                root.errorText = err || qsTr("Save failed.");
                root.statusText = "";
                return;
            }
            root.saving = false;
            root.errorText = "";
            root.statusText = qsTr("Saved");
            root._revision = revision;
            if (root.isMarkdown) {
                if (mdLoader.item)
                    mdLoader.item.markSaved(revision);
            } else if (codeLoader.item) {
                codeLoader.item.controller.markSaved(revision);
            }
        }
        function onChanged(r, dir) {
            if (r !== root.rootId || dir !== root.parentDir(root.path))
                return;
            const dirty = root.isMarkdown
                ? (mdLoader.item ? mdLoader.item.modified : false)
                : (codeLoader.item ? codeLoader.item.controller.modified : false);
            if (!dirty)
                root.reload();
        }
    }

    // Rich markdown editor (the block editor) for .md files - Loader-gated so the
    // de_core code editor is never constructed for markdown, and vice versa.
    Loader {
        id: mdLoader
        anchors.fill: parent
        active: root.isCurrent && root.loaded && !root.binary && !root.isImage && root.isMarkdown
        visible: active
        sourceComponent: markdownComponent
    }
    Component {
        id: markdownComponent
        MarkdownDocumentView {
            id: mdv
            isCurrent: root.isCurrent
            fileName: root.path
            contentMargin: 0
            Component.onCompleted: root._applyContent()
            onModifiedChanged: root.dirtyChanged(mdv.modified)
        }
    }

    // QML-native code editor for all other text files.
    Loader {
        id: codeLoader
        anchors.fill: parent
        active: root.isCurrent && root.loaded && !root.binary && !root.isImage && !root.isMarkdown
        visible: active
        sourceComponent: codeComponent
    }
    Component {
        id: codeComponent
        FocusScope {
            property alias controller: ctl
            focus: true
            CodeEditorController { id: ctl }
            CodeEditor {
                anchors.fill: parent
                controller: ctl
                focus: true
            }
            Component.onCompleted: root._applyContent()
            Connections {
                target: ctl
                function onModifiedChanged() { root.dirtyChanged(ctl.modified); }
            }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        visible: root.loaded && (root.binary || root.isImage || root.truncated)
        spacing: Theme.smallSpacing
        Label {
            Layout.alignment: Qt.AlignHCenter
            text: root.truncated ? qsTr("Large file \u2014 preview is truncated and editing is disabled.")
                 : root.isImage ? qsTr("Image preview is not available in the dev build.")
                 : qsTr("Binary file \u2014 not shown.")
            color: Theme.mutedText
        }
        Label {
            Layout.alignment: Qt.AlignHCenter
            text: root.path
            color: Theme.mutedText
            font.pixelSize: Theme.captionFontSize
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: errorLabel.implicitHeight + 2 * Theme.smallSpacing
        visible: root.errorText !== ""
        color: Theme.statusWarning
        opacity: 0.95

        Label {
            id: errorLabel
            anchors.fill: parent
            anchors.margins: Theme.smallSpacing
            text: root.errorText
            color: Theme.text
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
    }

    Rectangle {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.smallSpacing
        width: statusLabel.implicitWidth + 2 * Theme.smallSpacing
        height: statusLabel.implicitHeight + Theme.smallSpacing
        visible: root.statusText !== "" && root.errorText === ""
        color: Theme.toolSurface
        border.color: Theme.toolBorder
        border.width: Theme.hairline
        radius: Theme.radius

        Label {
            id: statusLabel
            anchors.centerIn: parent
            text: root.statusText
            color: Theme.mutedText
            font.pixelSize: Theme.captionFontSize
        }
    }

    Shortcut {
        sequences: [ StandardKey.Save ]
        onActivated: root.save()
    }
}
