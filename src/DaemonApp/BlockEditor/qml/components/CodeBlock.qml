// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick

import org.kde.syntaxhighlighting

import DaemonApp.Theme

// Passive render for a (non-mermaid) code fence: a rounded code card whose body
// is syntax-highlighted by the KDE KSyntaxHighlighting QML module. The fence
// language drives the definition lookup; the app theme selects a light/dark
// highlighting theme so a theme switch recolors the tokens live. Clicking the
// card opens the block for raw-markdown editing (same as the other structured
// blocks). Mermaid fences never reach here - they route to MermaidBlock.
Item {
    id: root

    property var codeData: ({})
    property var editorController: null
    property var blockId: undefined

    readonly property string code: (codeData && codeData.code) ? codeData.code : ""
    readonly property string language: (codeData && codeData.language) ? codeData.language : ""

    implicitHeight: card.implicitHeight

    Rectangle {
        id: card
        anchors.left: parent.left
        anchors.right: parent.right
        color: Theme.codeBackground
        border.width: Theme.hairline
        border.color: Theme.border
        radius: Theme.radiusSmall
        implicitHeight: layout.implicitHeight + Theme.smallSpacing * 2

        Column {
            id: layout
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.smallSpacing
            spacing: 2

            // Language chip (only when the fence carries an info string).
            Text {
                visible: root.language.length > 0
                text: root.language
                color: Theme.mutedText
                font.pixelSize: Theme.captionFontSize
            }

            TextEdit {
                id: codeText
                width: parent.width
                // Read-only, non-interactive: selection/editing is handled by
                // activating the block (the MouseArea below), matching the other
                // structured blocks. PlainText so the highlighter owns coloring.
                readOnly: true
                selectByMouse: false
                activeFocusOnPress: false
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.Wrap
                text: root.code
                color: Theme.text
                font.family: FontIcons.mono
                font.pixelSize: Theme.bodyFontSize - 1

                SyntaxHighlighter {
                    id: highlighter
                    textEdit: codeText
                    repository: Repository
                    // definitionForName("") yields the null definition (no
                    // highlighting), which is the right fallback for a bare fence.
                    definition: Repository.definitionForName(root.language)
                    theme: Theme.isDarkMode
                        ? Repository.defaultTheme(Repository.DarkTheme)
                        : Repository.defaultTheme(Repository.LightTheme)
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onClicked: {
                if (root.editorController && root.blockId !== undefined)
                    root.editorController.activateBlock(Number(root.blockId))
            }

            // Activating the code block drops into raw-markdown editing.
            Accessible.role: Accessible.Button
            Accessible.name: root.language.length > 0
                           ? qsTr("Code block (%1)").arg(root.language)
                           : qsTr("Code block")
            Accessible.onPressAction: {
                if (root.editorController && root.blockId !== undefined)
                    root.editorController.activateBlock(Number(root.blockId));
            }
        }
    }
}
