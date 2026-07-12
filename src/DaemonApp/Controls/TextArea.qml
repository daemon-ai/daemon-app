// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import DaemonApp.Theme

// TextArea subclass - the kit's multiline counterpart of Kit.TextField. Keeps
// Controls' input behavior, themes the text/selection/placeholder colors, gives
// the accent-colored blinking caret, and wires the themed EditMenu. The
// background stays null: every in-tree host frames the area itself (a bordered
// Rectangle + ScrollView, or the composer surface).
QQC.TextArea {
    id: root

    // Screen-reader name. The multiline area is usually framed by a host with a
    // separate caption and only carries a placeholder, so default the accessible
    // name to the placeholder; a call site sets this when neither is present.
    property string accessibleName: placeholderText
    Accessible.name: accessibleName

    color: Theme.text
    placeholderTextColor: Theme.textMuted
    selectionColor: Theme.searchSelection
    selectedTextColor: Theme.text
    font.family: FontIcons.display
    font.pixelSize: 14
    wrapMode: TextEdit.Wrap
    selectByMouse: true
    background: null

    cursorDelegate: Rectangle {
        id: caret
        width: 2
        color: Theme.accent
        visible: root.cursorVisible

        SequentialAnimation {
            loops: Animation.Infinite
            running: root.activeFocus
            PropertyAction { target: caret; property: "visible"; value: true }
            PauseAnimation { duration: 500 }
            PropertyAction { target: caret; property: "visible"; value: false }
            PauseAnimation { duration: 500 }
        }
    }

    // Themed right-click context menu. Suppress Qt 6.9+'s default Basic-styled
    // ContextMenu and open our kit menu instead, so every Kit.TextArea gets the
    // app's editing menu for free.
    QQC.ContextMenu.menu: null

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: editMenu.popup()
    }

    EditMenu {
        id: editMenu
        target: root
    }
}
