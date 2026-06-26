import QtQuick
import DaemonApp.Theme

// Presentational graph node chip (no interaction logic - GraphView wraps it with tap/connector).
// A filled rounded disc sized by the view, an optional presence dot, and a label below.
Item {
    id: chip

    property string text: ""
    property color fill: Theme.textMuted
    property color textColor: Theme.text
    property real diameter: 18
    property bool selected: false
    property color dotColor: "transparent"

    implicitWidth: diameter
    implicitHeight: diameter

    Rectangle {
        id: disc
        anchors.centerIn: parent
        width: chip.diameter
        height: chip.diameter
        radius: width / 2
        color: chip.fill
        opacity: chip.selected ? 1.0 : 0.9
        border.width: chip.selected ? 2 : 0
        border.color: Theme.text
    }

    // Presence / status dot (bottom-right), shown when a non-transparent color is set.
    Rectangle {
        visible: chip.dotColor != "transparent"
        width: 6
        height: 6
        radius: 3
        anchors.right: disc.right
        anchors.bottom: disc.bottom
        color: chip.dotColor
        border.width: 1
        border.color: Theme.surface
    }

    Text {
        anchors.top: disc.bottom
        anchors.topMargin: 1
        anchors.horizontalCenter: parent.horizontalCenter
        text: chip.text
        font.family: FontIcons.display
        font.pixelSize: 9
        color: chip.textColor
        width: 120
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        visible: chip.selected || chip.diameter > 14
    }
}
