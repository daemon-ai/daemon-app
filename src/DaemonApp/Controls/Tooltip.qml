import QtQuick
import QtQuick.Controls as QQC
import DaemonApp.Theme

// Themed ToolTip for cases where a tooltip is instantiated directly. IconButton
// uses the attached ToolTip; this is the reusable styled variant.
QQC.ToolTip {
    id: root

    delay: 500
    font.family: FontIcons.display
    font.pixelSize: 12

    contentItem: Text {
        text: root.text
        font: root.font
        color: Theme.background
    }

    background: Rectangle {
        radius: Theme.radius
        color: Theme.text
    }
}
