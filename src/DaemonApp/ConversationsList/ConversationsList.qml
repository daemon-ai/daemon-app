import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Middle column: the notes bar (collapse + title/count + trash + search button,
// with button-revealed search) and the NoteListView port - rows stacked as
// title / snippet / date / folder+tag chips note delegate.
Rectangle {
    id: root

    // Joins the sidebar chrome in dark/sepia/midnight; in Light it goes
    // off-white so it reads with the editor instead of as grey sidebar chrome.
    color: Theme.listBackground

    signal conversationActivated(int conversationId)
    signal toggleSidebarRequested()

    property int currentRow: -1
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
        store: ConversationStore
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
                        color: Theme.listTitle
                        font.family: FontIcons.display
                        font.weight: Font.DemiBold
                        font.pixelSize: Theme.labelSize
                        font.letterSpacing: Theme.labelTracking
                        font.capitalization: Font.AllUppercase
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
                    underline: true
                    placeholderText: qsTr("Search conversations")
                    leftPadding: 21
                    rightPadding: 30
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
                    x: 7
                    glyph: FontIcons.fa_magnifying_glass
                    font.pointSize: 11 + Theme.pointSizeOffset
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

                    // LEFT_OFFSET_X = 20, TOP_OFFSET_Y = 10, LAST_EL_SEP_SPACE = 12.
                    readonly property int leftOffset: 20

                    width: ListView.view.width
                    height: content.implicitHeight + 10 + 12

                    readonly property bool isSelected: index === root.currentRow

                    // Inset rounded selection: fill-only, no
                    // hairline - a deliberate step below the sidebar nav rows.
                    Rectangle {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.rowInset
                        anchors.rightMargin: Theme.rowInset
                        anchors.topMargin: Theme.rowVInset
                        anchors.bottomMargin: Theme.rowVInset
                        radius: Theme.rowRadius
                        // Stable wash color + opacity fade (never interpolate out of
                        // transparent-black, which flashed dark on the way in).
                        color: del.isSelected ? Theme.listSelection : Theme.rowHover
                        opacity: del.isSelected || rowMouse.containsMouse ? 1 : 0

                        Behavior on opacity {
                            NumberAnimation { duration: Theme.motionFast }
                        }
                    }

                    ColumnLayout {
                        id: content
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.topMargin: 10
                        anchors.leftMargin: del.leftOffset
                        anchors.rightMargin: del.leftOffset
                        spacing: 0

                        // Title - text-forward: secondary by default, brightens to
                        // primary on hover/selection.
                        QQC.Label {
                            Layout.fillWidth: true
                            text: del.title
                            color: del.isSelected || rowMouse.containsMouse
                                 ? Theme.text : Theme.listText
                            font.family: FontIcons.display
                            font.pixelSize: 13
                            font.weight: Font.Medium
                            elide: Text.ElideRight
                        }

                        // Snippet (snippet above date).
                        QQC.Label {
                            Layout.fillWidth: true
                            Layout.topMargin: 2
                            visible: del.snippet !== ""
                            text: del.snippet
                            color: Theme.listSnippet
                            font.family: FontIcons.display
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }

                        // Date - on its own line, left-aligned (not top-right).
                        QQC.Label {
                            Layout.fillWidth: true
                            Layout.topMargin: 5
                            text: del.modified ? Qt.formatDateTime(del.modified, "MMM d, h:mm AP") : ""
                            color: Theme.countText
                            font.family: FontIcons.display
                            font.pixelSize: 11
                        }

                        // Folder chip + tag chips.
                        Flow {
                            Layout.fillWidth: true
                            Layout.topMargin: 14
                            spacing: Theme.spacing
                            visible: del.folderName !== "" || (del.tagNames && del.tagNames.length > 0)

                            Row {
                                visible: del.folderName !== ""
                                spacing: 5
                                Kit.Glyph {
                                    glyph: FontIcons.fa_folder
                                    font.pointSize: 10 + Theme.pointSizeOffset
                                    color: Theme.listSnippet
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                QQC.Label {
                                    text: del.folderName
                                    color: Theme.listSnippet
                                    font.family: FontIcons.display
                                    font.pixelSize: 11
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }

                            Repeater {
                                model: del.tagNames
                                delegate: Row {
                                    required property int index
                                    required property string modelData
                                    spacing: 5
                                    Rectangle {
                                        width: 9
                                        height: 9
                                        radius: 4.5
                                        anchors.verticalCenter: parent.verticalCenter
                                        color: {
                                            const c = del.tagColors;
                                            return (c && index < c.length && c[index] !== "")
                                                ? c[index] : Theme.sidebarIcon;
                                        }
                                    }
                                    QQC.Label {
                                        text: modelData
                                        color: Theme.listSnippet
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
