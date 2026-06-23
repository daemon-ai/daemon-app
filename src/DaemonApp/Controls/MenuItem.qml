import QtQuick
import QtQuick.Controls as QQC
import DaemonApp.Theme

// Themed MenuItem - a flat row with a rounded hover/highlight wash (Theme.hover),
// primary text, muted when disabled. The check indicator and submenu arrow are
// collapsed to zero width since the kit's menus are simple action lists. Mirrors
// the row look of ComposerMenu's MenuRow so every menu reads the same.
QQC.MenuItem {
    id: root

    implicitHeight: 32
    leftPadding: 10
    rightPadding: 10
    spacing: 8

    indicator: Item { implicitWidth: 0; implicitHeight: 0 }
    arrow: Item { implicitWidth: 0; implicitHeight: 0 }

    contentItem: Text {
        text: root.text
        font.family: FontIcons.display
        font.pixelSize: 13
        color: root.enabled ? Theme.text : Theme.textMuted
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: 5
        color: root.highlighted ? Theme.hover : "transparent"
    }
}
