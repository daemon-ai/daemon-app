import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme

// A placeholder toolbar button for the conversation header: a rounded
// hover/pressed background with centered content (one or more glyphs/labels),
// editor top-bar buttons. Assign `content` an item or array
// of items. Non-functional unless `onClicked` is handled.
Item {
    id: root

    property string tooltipText: ""
    // Items to center inside the button (e.g. a glyph + a chevron).
    property alias content: holder.data

    signal clicked()

    implicitHeight: 28
    implicitWidth: 30

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: Theme.radius
        color: mouse.pressed ? Theme.pressed
             : mouse.containsMouse ? Theme.hover
             : "transparent"
    }

    // RowLayout centers its items vertically by default, so glyphs of
    // differing point sizes line up on a common center.
    RowLayout {
        id: holder
        anchors.centerIn: parent
        spacing: 6
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    QQC.ToolTip.text: root.tooltipText
    QQC.ToolTip.delay: 500
    QQC.ToolTip.visible: root.tooltipText.length > 0 && mouse.containsMouse
}
