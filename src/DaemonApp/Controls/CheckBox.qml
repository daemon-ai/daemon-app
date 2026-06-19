import QtQuick
import QtQuick.Controls as QQC
import DaemonApp.Theme

// CheckBox subclass, ported from Daino's CustomCheckBox.qml: a rounded indicator
// that fills with the accent on check and shows a FontAwesome check glyph, with a
// spring press animation. Keeps Controls' checked-state behavior.
QQC.CheckBox {
    id: root

    property int boxSize: 18

    font.family: FontIcons.display
    font.pixelSize: 14

    indicator: Rectangle {
        implicitWidth: root.boxSize
        implicitHeight: root.boxSize
        x: root.leftPadding
        y: root.height / 2 - height / 2
        radius: 3
        color: root.checked ? Theme.accent : "transparent"
        border.color: root.checked ? Theme.accent : Theme.textMuted
        scale: root.pressed ? 0.85 : 1.0

        Behavior on scale {
            SpringAnimation { spring: 6; damping: 0.2 }
        }

        Text {
            anchors.centerIn: parent
            visible: root.checked
            text: FontIcons.check
            font.family: FontIcons.faSolid
            font.pixelSize: root.boxSize - 7
            color: Theme.isDark ? "#1a212a" : "white"
        }
    }

    contentItem: Text {
        text: root.text
        font: root.font
        color: Theme.text
        verticalAlignment: Text.AlignVCenter
        leftPadding: root.indicator.width + Theme.spacingSmall
    }
}
