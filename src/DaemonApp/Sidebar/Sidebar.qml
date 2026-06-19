import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DaemonApp.Theme

Rectangle {
    id: root

    color: Theme.sidebar

    signal scopeSelected(int nodeType, int nodeId)
    signal settingsRequested()

    property int currentRow: 0

    SidebarModel {
        id: sidebarModel
        store: ChatStore
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Theme.spacing
            spacing: Theme.spacingSmall

            Label {
                Layout.fillWidth: true
                text: qsTr("daemon-app")
                color: Theme.text
                font.bold: true
                font.pixelSize: 16
                elide: Text.ElideRight
            }

            ToolButton {
                text: "\u2699"
                ToolTip.text: qsTr("Settings")
                ToolTip.visible: hovered
                onClicked: root.settingsRequested()
            }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: sidebarModel
            boundsBehavior: Flickable.StopAtBounds

            delegate: ItemDelegate {
                id: del
                required property int index
                required property string label
                required property int count
                required property bool isSeparator
                required property bool selectable

                width: ListView.view.width
                height: isSeparator ? 30 : 36
                enabled: selectable
                highlighted: !isSeparator && index === root.currentRow

                background: Rectangle {
                    color: del.highlighted ? Theme.selection : "transparent"
                }

                contentItem: RowLayout {
                    spacing: Theme.spacingSmall

                    Label {
                        Layout.fillWidth: true
                        Layout.leftMargin: del.isSeparator ? 0 : Theme.spacingSmall
                        text: del.label
                        elide: Text.ElideRight
                        color: del.isSeparator ? Theme.textMuted : Theme.text
                        font.bold: del.isSeparator
                        font.pixelSize: del.isSeparator ? 11 : 14
                        font.capitalization: del.isSeparator ? Font.AllUppercase : Font.MixedCase
                    }

                    Label {
                        visible: del.count >= 0
                        text: del.count
                        color: Theme.textMuted
                        font.pixelSize: 12
                    }
                }

                onClicked: {
                    root.currentRow = index;
                    sidebarModel.activate(index);
                }
            }
        }
    }

    Connections {
        target: sidebarModel
        function onScopeSelected(nodeType, nodeId) {
            root.scopeSelected(nodeType, nodeId);
        }
    }

    Component.onCompleted: sidebarModel.activate(0)
}
