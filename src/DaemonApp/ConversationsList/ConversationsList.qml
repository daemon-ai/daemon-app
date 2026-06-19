import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

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

        // --- Notes bar ------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingSmall
            Layout.rightMargin: Theme.spacingSmall
            Layout.topMargin: Theme.spacing
            spacing: Theme.spacingSmall

            Kit.IconButton {
                icon: FontIcons.fa_arrow_left_to_line
                tooltipText: qsTr("Collapse sidebar")
                onClicked: root.toggleSidebarRequested()
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                QQC.Label {
                    Layout.fillWidth: true
                    text: convModel.scopeTitle
                    color: Theme.text
                    font.family: FontIcons.display
                    font.bold: true
                    font.pixelSize: 15
                    elide: Text.ElideRight
                }

                QQC.Label {
                    text: convModel.count === 1 ? qsTr("1 conversation")
                                                : qsTr("%1 conversations").arg(convModel.count)
                    color: Theme.countText
                    font.family: FontIcons.display
                    font.pixelSize: 11
                }
            }

            Kit.IconButton {
                icon: FontIcons.fa_plus
                tooltipText: qsTr("New conversation")
                iconColor: Theme.accent
                onClicked: root.newConversationRequested()
            }
        }

        // --- Search ---------------------------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacing
            Layout.rightMargin: Theme.spacing
            Layout.topMargin: Theme.spacingSmall
            Layout.bottomMargin: Theme.spacingSmall
            implicitHeight: searchField.implicitHeight

            Kit.TextField {
                id: searchField
                anchors.fill: parent
                placeholderText: qsTr("Search")
                leftPadding: 32
                text: convModel.search
                onTextEdited: convModel.search = text
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                x: 11
                text: FontIcons.fa_magnifying_glass
                font.family: FontIcons.faSolid
                font.pixelSize: 13
                color: Theme.textMuted
            }
        }

        // --- List -----------------------------------------------------------
        QQC.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            QQC.ScrollBar.vertical: Kit.ScrollBar {}

            ListView {
                id: list
                model: convModel
                boundsBehavior: Flickable.StopAtBounds

                delegate: Item {
                    id: del
                    required property int index
                    required property string title
                    required property string snippet
                    required property var modified

                    width: ListView.view.width
                    height: 64

                    readonly property bool isSelected: index === root.currentRow

                    Rectangle {
                        anchors.fill: parent
                        color: del.isSelected ? Theme.selection
                             : rowMouse.containsMouse ? Theme.hover
                             : "transparent"

                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 1
                            color: Theme.border
                        }
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacing
                        anchors.rightMargin: Theme.spacing
                        anchors.topMargin: Theme.spacingSmall
                        anchors.bottomMargin: Theme.spacingSmall
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.spacingSmall

                            QQC.Label {
                                Layout.fillWidth: true
                                text: del.title
                                color: Theme.text
                                font.family: FontIcons.display
                                font.pixelSize: 14
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            QQC.Label {
                                text: del.modified ? Qt.formatDateTime(del.modified, "MMM d") : ""
                                color: Theme.countText
                                font.family: FontIcons.display
                                font.pixelSize: 11
                            }
                        }

                        QQC.Label {
                            Layout.fillWidth: true
                            text: del.snippet
                            color: Theme.textMuted
                            font.family: FontIcons.display
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            wrapMode: Text.Wrap
                        }
                    }

                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.currentRow = del.index;
                            root.conversationActivated(convModel.idAt(del.index));
                        }
                    }
                }
            }
        }
    }
}
