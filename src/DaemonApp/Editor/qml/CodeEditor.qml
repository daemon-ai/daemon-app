import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import org.kde.syntaxhighlighting

import DaemonApp.Theme
import DaemonApp.Settings
import DaemonApp.Controls as Kit
import DaemonApp.Editor

// Native Qt Quick source editor surface. Text input, selection, clipboard,
// IME/preedit, touch, and accessibility stay with TextArea/QTextDocument; the
// C++ controller owns file revision/dirty state and remains the GUI/TUI seam.
FocusScope {
    id: root

    property CodeEditorController controller
    property string fontFamily: FontIcons.mono
    property int fontSize: UiSettings.editorFontSize
    property int maxGutterLines: 2000
    property bool _loading: false

    function syncFromController() {
        if (!controller)
            return;
        const next = controller.text();
        if (textArea.text === next)
            return;
        _loading = true;
        textArea.text = next;
        _loading = false;
    }

    Component.onCompleted: {
        if (controller)
            controller.setDarkTheme(Theme.isDarkMode);
        syncFromController();
    }
    onControllerChanged: syncFromController()

    Connections {
        target: root.controller
        function onDocumentChanged() {
            // Loads and remote reloads are authoritative even while the text
            // control has focus (e.g. preview-tab reassignment). User edits are
            // safe because syncFromController() is an equality-guarded no-op once
            // TextArea.onTextChanged has pushed the same text into the controller.
            root.syncFromController();
        }
        function onLanguageChanged() { highlighter.definition = Repository.definitionForName(root.controller.languageName || "None"); }
    }

    FontMetrics {
        id: gutterFontMetrics
        font.family: root.fontFamily
        font.pixelSize: root.fontSize
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.codeBackground

        RowLayout {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                Layout.preferredWidth: 56
                Layout.fillHeight: true
                color: Theme.codeBackground

                Text {
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    text: {
                        const lineCount = Number.isFinite(textArea.lineCount) ? textArea.lineCount : 1;
                        const n = Math.min(root.maxGutterLines, Math.max(1, lineCount));
                        let out = "";
                        for (let i = 1; i <= n; ++i)
                            out += i + (i === n ? "" : "\n");
                        if (lineCount > root.maxGutterLines)
                            out += "\n…";
                        return out;
                    }
                    color: Theme.mutedText
                    font.family: root.fontFamily
                    font.pixelSize: root.fontSize
                    lineHeightMode: Text.FixedHeight
                    lineHeight: gutterFontMetrics.lineSpacing
                }
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.vertical: Kit.ScrollBar {}

                TextArea {
                    id: textArea
                    width: parent.width
                    wrapMode: TextEdit.NoWrap
                    background: null
                    color: Theme.text
                    selectionColor: Theme.selection
                    selectedTextColor: Theme.selectionText
                    font.family: root.fontFamily
                    font.pixelSize: root.fontSize
                    persistentSelection: true

                    ContextMenu.menu: null
                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: editMenu.popup()
                    }
                    Kit.EditMenu {
                        id: editMenu
                        target: textArea
                    }

                    onTextChanged: {
                        if (!root._loading && activeFocus && root.controller)
                            root.controller.replaceTextFromEditor(text);
                    }

                    SyntaxHighlighter {
                        id: highlighter
                        textEdit: textArea
                        repository: Repository
                        definition: Repository.definitionForName(root.controller ? (root.controller.languageName || "None") : "None")
                        theme: Theme.isDarkMode
                            ? Repository.defaultTheme(Repository.DarkTheme)
                            : Repository.defaultTheme(Repository.LightTheme)
                    }
                }
            }
        }
    }
}
