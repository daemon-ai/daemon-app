import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

Rectangle {
    id: root

    color: Theme.surface

    signal conversationActivated(int conversationId)
    signal toggleSidebarRequested()

    property int currentRow: -1
    // Reveals the search field via a button; it is hidden by default.
    property bool searchActive: false

    function setScope(nodeType, nodeId) {
        root.currentRow = -1;
        convModel.setScope(nodeType, nodeId);
    }

    function closeSearch() {
        convModel.search = "";
        root.searchActive = false;
    }

    ConversationsListModel {
        id: convModel
        store: ChatStore
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Notes bar ------------------------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingSmall
            Layout.rightMargin: Theme.spacingSmall
            Layout.topMargin: Theme.spacing
            Layout.bottomMargin: Theme.spacingSmall
            implicitHeight: 36

            // Default bar: collapse + title/count + trash + search button.
            RowLayout {
                anchors.fill: parent
                visible: !root.searchActive
                spacing: Theme.spacingSmall

                Kit.IconButton {
                    icon: FontIcons.fa_angles_left
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
                    icon: FontIcons.fa_trash
                    tooltipText: qsTr("Trash")
                    onClicked: convModel.setScope(1, -1)
                }

                Kit.IconButton {
                    icon: FontIcons.fa_magnifying_glass
                    tooltipText: qsTr("Search")
                    onClicked: {
                        root.searchActive = true;
                        searchField.forceActiveFocus();
                    }
                }
            }

            // Search row: revealed field with leading magnifier + clear button.
            Item {
                anchors.fill: parent
                visible: root.searchActive

                Kit.TextField {
                    id: searchField
                    anchors.fill: parent
                    placeholderText: qsTr("Search conversations")
                    leftPadding: 32
                    rightPadding: 32
                    text: convModel.search
                    onTextEdited: convModel.search = text
                    Keys.onEscapePressed: root.closeSearch()
                    onActiveFocusChanged: {
                        if (!activeFocus && text === "")
                            root.searchActive = false;
                    }
                }

                Kit.Glyph {
                    anchors.verticalCenter: parent.verticalCenter
                    x: 11
                    glyph: FontIcons.fa_magnifying_glass
                    font.pointSize: 12 + Theme.pointSizeOffset
                    color: Theme.textMuted
                }

                Kit.IconButton {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 2
                    implicitWidth: 26
                    implicitHeight: 26
                    icon: FontIcons.fa_xmark
                    iconPointSize: 12
                    tooltipText: qsTr("Close search")
                    onClicked: root.closeSearch()
                }
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
                    required property string folderName
                    required property var tagNames
                    required property var tagColors

                    width: ListView.view.width
                    height: content.implicitHeight + 2 * Theme.spacingSmall + 1

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
                        id: content
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: Theme.spacing
                        anchors.rightMargin: Theme.spacing
                        spacing: 3

                        // Line 1: title + time.
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

                        // Line 2: snippet.
                        QQC.Label {
                            Layout.fillWidth: true
                            visible: del.snippet !== ""
                            text: del.snippet
                            color: Theme.textMuted
                            font.family: FontIcons.display
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            wrapMode: Text.Wrap
                        }

                        // Line 3: folder chip + tag chips.
                        Flow {
                            Layout.fillWidth: true
                            Layout.topMargin: 1
                            spacing: Theme.spacingSmall
                            visible: del.folderName !== "" || (del.tagNames && del.tagNames.length > 0)

                            // Folder chip.
                            Row {
                                visible: del.folderName !== ""
                                spacing: 4
                                Kit.Glyph {
                                    glyph: FontIcons.fa_folder
                                    font.pointSize: 9 + Theme.pointSizeOffset
                                    color: Theme.countText
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                QQC.Label {
                                    text: del.folderName
                                    color: Theme.countText
                                    font.family: FontIcons.display
                                    font.pixelSize: 11
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }

                            // Tag chips (colored dot + name).
                            Repeater {
                                model: del.tagNames
                                delegate: Row {
                                    required property int index
                                    required property string modelData
                                    spacing: 4
                                    Rectangle {
                                        width: 8
                                        height: 8
                                        radius: 4
                                        anchors.verticalCenter: parent.verticalCenter
                                        color: {
                                            const c = del.tagColors;
                                            return (c && index < c.length && c[index] !== "")
                                                ? c[index] : Theme.iconMuted;
                                        }
                                    }
                                    QQC.Label {
                                        text: modelData
                                        color: Theme.countText
                                        font.family: FontIcons.display
                                        font.pixelSize: 11
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }
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
