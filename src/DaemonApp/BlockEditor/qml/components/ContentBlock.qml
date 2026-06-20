import QtQuick

import DaemonApp.Theme

// Passive render for a Content block: a typed content stream emitted outside a
// tool (e.g. a raw ansi/pty stream). Routes by kind to the same sub-renderers
// the tool body uses; falls back to plain monospace text.
Item {
    id: root

    property var contentData: ({})
    property var editorController: null
    property var blockId: undefined

    readonly property string kind: (contentData && contentData.kind) ? contentData.kind : ""
    readonly property string body: (contentData && contentData.body) ? String(contentData.body) : ""

    implicitHeight: loader.implicitHeight

    Loader {
        id: loader
        anchors.left: parent.left
        anchors.right: parent.right
        sourceComponent: {
            switch (root.kind) {
            case "ansi-stream":
            case "pty": return ansiComponent
            case "diff": return diffComponent
            default: return textComponent
            }
        }
    }

    Component {
        id: ansiComponent
        AnsiText {
            width: loader.width
            spans: root.editorController ? root.editorController.ansiSpans(root.body) : []
        }
    }

    Component {
        id: diffComponent
        DiffBlock {
            width: loader.width
            lines: root.editorController ? root.editorController.parseDiff(root.body) : []
        }
    }

    Component {
        id: textComponent
        Text {
            width: loader.width
            text: root.body
            textFormat: Text.PlainText
            color: Theme.text
            font.family: FontIcons.mono
            font.pixelSize: Theme.bodyFontSize - 1
            wrapMode: Text.Wrap
        }
    }
}
