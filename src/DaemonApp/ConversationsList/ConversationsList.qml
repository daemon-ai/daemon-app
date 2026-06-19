import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DaemonApp.Theme

Rectangle {
    id: root

    color: Theme.surface

    signal conversationActivated(int conversationId)
    signal newConversationRequested()
    signal toggleSidebarRequested()

    property int currentRow: -1

    function setScope(nodeType, nodeId) {
        root.currentRow = -1;
        convModel.setScope(nodeType, nodeId);
    }

    ConversationsListModel {
        id: convModel
        store: ChatStore
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Theme.spacing
            spacing: Theme.spacingSmall

            ToolButton {
                text: "\u2630"
                ToolTip.text: qsTr("Toggle sidebar")
                ToolTip.visible: hovered
                onClicked: root.toggleSidebarRequested()
            }

            Label {
                Layout.fillWidth: true
                text: convModel.scopeTitle
                color: Theme.text
                font.bold: true
                font.pixelSize: 15
                elide: Text.ElideRight
            }

            ToolButton {
                text: "+"
                ToolTip.text: qsTr("New conversation")
                ToolTip.visible: hovered
                onClicked: root.newConversationRequested()
            }
        }

        TextField {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacing
            Layout.rightMargin: Theme.spacing
            Layout.bottomMargin: Theme.spacingSmall
            placeholderText: qsTr("Search conversations")
            text: convModel.search
            onTextEdited: convModel.search = text
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: convModel
            boundsBehavior: Flickable.StopAtBounds

            delegate: ItemDelegate {
                id: del
                required property int index
                required property string title
                required property string snippet

                width: ListView.view.width
                highlighted: index === root.currentRow

                background: Rectangle {
                    color: del.highlighted ? Theme.selection : "transparent"
                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 1
                        color: Theme.border
                    }
                }

                contentItem: ColumnLayout {
                    spacing: 2

                    Label {
                        Layout.fillWidth: true
                        text: del.title
                        color: Theme.text
                        font.pixelSize: 14
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: del.snippet
                        color: Theme.textMuted
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                }

                onClicked: {
                    root.currentRow = index;
                    root.conversationActivated(convModel.idAt(index));
                }
            }
        }
    }
}
