import QtQuick
import QtQuick.Controls as QQC
import DaemonApp.Theme

// A compact themed toggle: an accent-filled track when on, a muted track when
// off, with a white thumb that slides across. Label-less by design; the settings
// rows (OptionRow) supply their own caption to the left.
QQC.Switch {
    id: root

    padding: 0
    spacing: 0
    implicitWidth: 38
    implicitHeight: 22

    indicator: Rectangle {
        implicitWidth: 38
        implicitHeight: 22
        radius: height / 2
        color: !root.enabled ? Theme.hover
             : root.checked ? Theme.accent
             : Theme.isDarkMode ? "#4a4a4a" : "#d4d4d4"
        opacity: root.enabled ? 1.0 : 0.4

        Behavior on color { ColorAnimation { duration: 120 } }

        Rectangle {
            width: 16
            height: 16
            radius: height / 2
            y: (parent.height - height) / 2
            x: root.checked ? parent.width - width - 3 : 3
            color: "white"

            Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
        }
    }

    // The caption lives in the host row, so the control itself draws only the
    // toggle.
    contentItem: Item {}
}
