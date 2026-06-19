import QtQuick
import QtQuick.Controls as QQC
import DaemonApp.Theme

// TextField subclass, keeps Controls'
// input behavior, restyles the background/colors via theme tokens and gives the
// accent-colored blinking caret. Used for search and single-line inputs.
QQC.TextField {
    id: root

    color: Theme.text
    placeholderTextColor: Theme.textMuted
    selectionColor: Theme.searchSelection
    selectedTextColor: Theme.text
    font.family: FontIcons.display
    font.pixelSize: 14
    leftPadding: Theme.spacing
    rightPadding: Theme.spacing
    selectByMouse: true

    background: Rectangle {
        radius: Theme.radius
        color: Theme.searchBackground
        border.width: root.activeFocus ? 2 : 1
        border.color: root.activeFocus ? Theme.searchFocusBorder : Theme.searchBorder
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
}
