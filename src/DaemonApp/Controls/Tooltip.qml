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
        color: Theme.isDark ? "#1f1f1f" : "white"
    }

    background: Rectangle {
        radius: Theme.radius
        color: Theme.isDark ? "#e6e6e6" : "#2f2f2f"
    }
}
