import QtQuick
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// A settings toggle row: caption on the left, themed Switch on the right. The
// whole row is a hover target; clicking anywhere requests the opposite state via
// toggled(newValue). State is unidirectional: the parent flips UiSettings, which
// flows back into `checked` (the Switch never mutates itself).
Item {
    id: root

    property string label: ""
    property bool checked: false

    signal toggled(bool checked)

    implicitWidth: parent ? parent.width : 200
    implicitHeight: 34

    Rectangle {
        anchors.fill: parent
        radius: 5
        color: hover.containsMouse ? Theme.hover : "transparent"
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        text: root.label
        font.family: FontIcons.display
        font.pixelSize: 14
        color: Theme.text
    }

    Kit.Switch {
        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        checkable: false
        checked: root.checked
    }

    MouseArea {
        id: hover
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.toggled(!root.checked)
    }
}
