import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// The pane-level tab strip: a horizontal, scrollable row of tab chips bound to a
// shared TabModel, plus a "+" affordance to open a new tab and a settings entry.
// Clicking a chip activates it; the chip's "x" closes it. This replaces the old
// placeholder markdown-formatting header at the top of the conversation pane.
RowLayout {
    id: root

    // The shared TabModel (DaemonApp.Tabs). Required.
    property var tabModel: null

    spacing: 4

    // Request a brand-new transcript tab (host decides which conversation).
    signal newTabRequested()
    // Open the Settings page tab.
    signal settingsRequested()

    implicitHeight: 36

    // --- Tab chips (scroll horizontally when they overflow) ------------------
    ListView {
        id: tabList
        objectName: "tabListView"
        Layout.fillWidth: true
        Layout.fillHeight: true
        orientation: ListView.Horizontal
        clip: true
        spacing: 2
        model: root.tabModel
        // Keep the active tab visible when switching via keyboard/clicks.
        currentIndex: root.tabModel ? root.tabModel.currentIndex : -1
        highlightFollowsCurrentItem: true
        boundsBehavior: Flickable.StopAtBounds

        delegate: Rectangle {
            id: chip
            objectName: "tabChip"

            required property int index
            required property string title
            required property bool current
            required property bool closable

            height: tabList.height - 6
            y: 3
            width: Math.min(200, Math.max(96, label.implicitWidth + 44))
            radius: Theme.radius
            color: chip.current ? Theme.surfaceRaised
                 : tabHover.containsMouse ? Theme.hover : "transparent"
            border.width: 1
            border.color: chip.current ? Theme.border : "transparent"

            Text {
                id: label
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.right: closeButton.left
                anchors.rightMargin: 2
                anchors.verticalCenter: parent.verticalCenter
                text: chip.title
                elide: Text.ElideRight
                font.family: FontIcons.display
                font.pixelSize: 13
                color: chip.current ? Theme.text : Theme.textMuted
            }

            Kit.IconButton {
                id: closeButton
                anchors.right: parent.right
                anchors.rightMargin: 3
                anchors.verticalCenter: parent.verticalCenter
                implicitWidth: 20
                implicitHeight: 20
                visible: chip.closable
                icon: FontIcons.fa_xmark
                iconColor: Theme.iconMuted
                iconPointSize: 11
                backgroundRadius: width / 2
                tooltipText: qsTr("Close tab")
                onClicked: root.tabModel.closeTab(chip.index)
            }

            MouseArea {
                id: tabHover
                objectName: "tabChipArea"
                anchors.fill: parent
                anchors.rightMargin: chip.closable ? 24 : 0 // leave the x clickable
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.tabModel.activate(chip.index)
            }
        }
    }

    // --- New tab -------------------------------------------------------------
    Kit.IconButton {
        objectName: "newTabButton"
        Layout.alignment: Qt.AlignVCenter
        implicitWidth: 30
        implicitHeight: 28
        icon: FontIcons.fa_plus
        iconColor: Theme.iconMuted
        iconPointSize: 14
        tooltipText: qsTr("New tab")
        onClicked: root.newTabRequested()
    }

    // --- Settings ------------------------------------------------------------
    Kit.IconButton {
        objectName: "settingsButton"
        Layout.alignment: Qt.AlignVCenter
        implicitWidth: 30
        implicitHeight: 28
        icon: FontIcons.fa_gear
        iconColor: Theme.iconMuted
        iconPointSize: 14
        tooltipText: qsTr("Settings")
        onClicked: root.settingsRequested()
    }
}
