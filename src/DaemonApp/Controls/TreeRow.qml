import QtQuick
import QtQuick.Controls.Basic

import DaemonApp.Theme
import DaemonApp.Controls as Kit

Item {
    id: root

    property string label: ""
    property string icon: ""
    property int depth: 0
    property bool current: false
    property bool hovered: false
    property bool ignored: false
    property bool hasChildren: false
    property bool expanded: false
    property bool showTwistie: hasChildren
    property string trailingText: ""
    property color iconColor: current || hovered ? Theme.sidebarSelectedText : Theme.sidebarIcon
    property color textColor: current || hovered ? Theme.sidebarSelectedText : Theme.sidebarText

    signal twistieClicked()

    implicitHeight: Theme.rowHeight

    readonly property int indentBase: 14 + depth * Theme.treeIndent

    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: Theme.rowInset
        anchors.rightMargin: Theme.rowInset
        anchors.topMargin: Theme.rowVInset
        anchors.bottomMargin: Theme.rowVInset
        radius: Theme.rowRadius
        color: root.current ? Theme.sidebarSelection : Theme.sidebarHover
        opacity: root.current || root.hovered ? 1 : 0
        border.width: root.current ? 1 : 0
        border.color: Theme.border

        Behavior on opacity { NumberAnimation { duration: Theme.motionFast } }
    }

    Item {
        id: twistie
        anchors.left: parent.left
        anchors.leftMargin: root.indentBase
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.twistieSize
        height: Theme.twistieSize
        visible: root.showTwistie

        Kit.Glyph {
            anchors.centerIn: parent
            glyph: FontIcons.fa_chevron_right
            font.pointSize: 9 + Theme.pointSizeOffset
            rotation: root.expanded ? 90 : 0
            color: root.iconColor
            Behavior on rotation { NumberAnimation { duration: Theme.motionFast } }
        }

        MouseArea {
            anchors.fill: parent
            anchors.margins: -3
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.twistieClicked()
        }
    }

    Item {
        id: iconBox
        anchors.left: parent.left
        anchors.leftMargin: root.indentBase + Theme.twistieSize
        anchors.verticalCenter: parent.verticalCenter
        width: 18
        height: 18

        Kit.Glyph {
            anchors.centerIn: parent
            glyph: root.icon
            font.pointSize: 12 + Theme.pointSizeOffset
            color: root.iconColor
            opacity: root.ignored ? 0.5 : 1
        }
    }

    Label {
        anchors.left: iconBox.right
        anchors.leftMargin: 5
        anchors.right: trailing.left
        anchors.rightMargin: 5
        anchors.verticalCenter: parent.verticalCenter
        text: root.label
        elide: Text.ElideRight
        font.family: FontIcons.display
        font.pixelSize: 13
        font.weight: Font.Medium
        color: root.textColor
        opacity: root.ignored ? 0.5 : 1
    }

    Label {
        id: trailing
        anchors.right: parent.right
        anchors.rightMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        visible: root.trailingText !== ""
        text: root.trailingText
        font.family: FontIcons.display
        font.pixelSize: 11
        font.weight: Font.Medium
        color: root.current || root.hovered ? Theme.sidebarSelectedText : Theme.countText
    }
}
