// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import DaemonApp.Theme

// TextField subclass, keeps Controls'
// input behavior, restyles the background/colors via theme tokens and gives the
// accent-colored blinking caret. Used for search and single-line inputs.
QQC.TextField {
    id: root

    // Borderless underline-on-focus variant: transparent fill,
    // no box, just a 1px bottom edge that picks up the accent on focus. Default
    // is the boxed search field.
    property bool underline: false

    // Screen-reader name. A single-line field is usually label-less (only a
    // placeholder), so default the accessible name to the placeholder text; a
    // call site with neither an adjacent label nor a placeholder sets this.
    property string accessibleName: placeholderText
    Accessible.name: accessibleName

    color: Theme.searchText
    placeholderTextColor: Theme.textMuted
    selectionColor: Theme.searchSelection
    selectedTextColor: Theme.searchText
    font.family: FontIcons.display
    font.pixelSize: 14
    leftPadding: Theme.spacing
    rightPadding: Theme.spacing
    selectByMouse: true

    background: Rectangle {
        radius: root.underline ? 0 : Theme.radius
        color: root.underline ? "transparent" : Theme.searchBackground
        border.width: root.underline ? 0 : 1
        border.color: root.activeFocus ? Theme.searchFocusBorder : Theme.searchBorder

        // Underline mode: a single bottom hairline, accent on focus.
        Rectangle {
            visible: root.underline
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: root.activeFocus ? Theme.accent : Theme.searchBorder
        }
    }

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
    // ContextMenu and open our kit menu instead, so every Kit.TextField gets the
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
