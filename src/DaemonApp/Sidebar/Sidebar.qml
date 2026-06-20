import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Left column: the NodeTreeView port. tree delegates -
// gear right-aligned above the tree, full-rect blue selection with white text,
// folder/tag rows with blue icons + counts, and Folders/Tags separators with a
// blue "+" add button.
Rectangle {
    id: root

    color: Theme.sidebar

    signal scopeSelected(int nodeType, int nodeId)
    signal settingsRequested()
    signal addFolderRequested()
    signal addTagRequested()

    property int currentRow: 0

    // Node enum: 0 AllConversations, 1 Archived, 2 FolderSep, 3 TagSep,
    // 4 Folder, 5 Tag.
    function iconFor(nodeType) {
        switch (nodeType) {
        case 0: return FontIcons.fa_comments;
        case 1: return FontIcons.fa_box_archive;
        case 4: return FontIcons.fa_folder;
        default: return "";
        }
    }

    SidebarModel {
        id: sidebarModel
        store: ConversationStore
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Header: gear, right-aligned (mainwindow.ui spacer -> gear) ------
        Item {
            Layout.fillWidth: true
            implicitHeight: 28

            Kit.IconButton {
                anchors.right: parent.right
                anchors.rightMargin: 3
                anchors.verticalCenter: parent.verticalCenter
                implicitWidth: 33
                implicitHeight: 25
                icon: FontIcons.fa_gear
                iconColor: Theme.iconMuted
                tooltipText: qsTr("Settings")
                onClicked: settingsMenu.open()
            }
        }

        // --- Rows -----------------------------------------------------------
        QQC.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            QQC.ScrollBar.vertical: Kit.ScrollBar {}

            ListView {
                id: list
                model: sidebarModel
                spacing: 1
                boundsBehavior: Flickable.StopAtBounds

                delegate: Item {
                    id: del
                    required property int index
                    required property string label
                    required property int count
                    required property int nodeType
                    required property int nodeId
                    required property bool isSeparator
                    required property bool selectable
                    required property string color

                    width: ListView.view.width
                    // Dense column: compact section header / 28px nav row.
                    height: isSeparator ? 30 : 28

                    readonly property bool isSelected: !isSeparator && index === root.currentRow
                    readonly property bool isTag: nodeType === 5

                    // Inset rounded selection pill: the
                    // highlight is inset from the column edges and rounded, with a
                    // faint hairline on the selected row.
                    Rectangle {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.rowInset
                        anchors.rightMargin: Theme.rowInset
                        anchors.topMargin: Theme.rowVInset
                        anchors.bottomMargin: Theme.rowVInset
                        radius: Theme.rowRadius
                        visible: !del.isSeparator
                        // Stable wash color + opacity fade (no transparent-black flash).
                        color: del.isSelected ? Theme.sidebarSelection : Theme.sidebarHover
                        opacity: del.isSelected || rowMouse.containsMouse ? 1 : 0
                        border.width: del.isSelected ? 1 : 0
                        border.color: Theme.border

                        Behavior on opacity {
                            NumberAnimation { duration: Theme.motionFast }
                        }
                    }

                    // --- Separator row: label (x+5) + blue "+" add button ----
                    Item {
                        anchors.fill: parent
                        visible: del.isSeparator

                        QQC.Label {
                            anchors.left: parent.left
                            anchors.leftMargin: 14
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 5
                            text: del.label
                            color: Theme.separatorText
                            font.family: FontIcons.display
                            font.pixelSize: Theme.labelSize
                            font.weight: Font.DemiBold
                            font.letterSpacing: Theme.labelTracking
                            font.capitalization: Font.AllUppercase
                        }

                        Kit.IconButton {
                            anchors.right: parent.right
                            anchors.rightMargin: 4
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 2
                            implicitWidth: 30
                            implicitHeight: 24
                            icon: FontIcons.fa_plus
                            iconPointSize: 12
                            iconColor: Theme.addButton
                            tooltipText: del.nodeType === 2 ? qsTr("New folder") : qsTr("New tag")
                            onClicked: del.nodeType === 2 ? root.addFolderRequested()
                                                          : root.addTagRequested()
                        }
                    }

                    // --- Selectable row: icon/dot (x+22) + label + count -----
                    Item {
                        anchors.fill: parent
                        visible: !del.isSeparator

                        // Icon box: 18 wide, flush-left at x+14 (shares the left
                        // edge with the FOLDERS/TAGS section labels).
                        Item {
                            id: iconBox
                            anchors.left: parent.left
                            anchors.leftMargin: 14
                            anchors.verticalCenter: parent.verticalCenter
                            width: 18
                            height: 18

                            Kit.Glyph {
                                anchors.centerIn: parent
                                visible: !del.isTag
                                glyph: root.iconFor(del.nodeType)
                                font.pointSize: 12 + Theme.pointSizeOffset
                                color: del.isSelected || rowMouse.containsMouse
                                     ? Theme.sidebarSelectedText : Theme.sidebarIcon
                            }

                            // Tag dot (fa_circle, in the tag color).
                            Kit.Glyph {
                                anchors.centerIn: parent
                                visible: del.isTag
                                glyph: FontIcons.fa_circle
                                font.pointSize: 12 + Theme.pointSizeOffset
                                color: del.isSelected ? Theme.sidebarSelectedText
                                     : del.color !== "" ? del.color : Theme.sidebarIcon
                            }
                        }

                        QQC.Label {
                            anchors.left: iconBox.right
                            anchors.leftMargin: del.isTag ? 11 : 5
                            anchors.right: countLabel.left
                            anchors.rightMargin: 5
                            anchors.verticalCenter: parent.verticalCenter
                            text: del.label
                            elide: Text.ElideRight
                            font.family: FontIcons.display
                            font.pixelSize: 13
                            font.weight: Font.Medium
                            color: del.isSelected || rowMouse.containsMouse
                                 ? Theme.sidebarSelectedText : Theme.sidebarText
                        }

                        QQC.Label {
                            id: countLabel
                            anchors.right: parent.right
                            anchors.rightMargin: 14
                            anchors.verticalCenter: parent.verticalCenter
                            visible: del.count >= 0
                            text: del.count
                            font.family: FontIcons.display
                            font.pixelSize: 11
                            font.weight: Font.Medium
                            color: del.isSelected || rowMouse.containsMouse
                                 ? Theme.sidebarSelectedText : Theme.countText
                        }
                    }

                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: !del.isSeparator
                        enabled: del.selectable && !del.isSeparator
                        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: {
                            root.currentRow = del.index;
                            sidebarModel.activate(del.index);
                        }
                    }
                }
            }
        }
    }

    // --- Theme switcher popup (opened by the cog) ---------------------------
    QQC.Popup {
        id: settingsMenu
        x: parent.width - width - 3
        y: 31
        padding: Theme.spacingSmall
        modal: false
        focus: true

        background: Rectangle {
            color: Theme.background
            border.color: Theme.border
            radius: Theme.radius
        }

        contentItem: ColumnLayout {
            spacing: Theme.spacingSmall

            QQC.Label {
                text: qsTr("Theme")
                color: Theme.textMuted
                font.family: FontIcons.display
                font.pixelSize: Theme.labelSize
                font.weight: Font.DemiBold
                font.letterSpacing: Theme.labelTracking
                font.capitalization: Font.AllUppercase
                Layout.leftMargin: Theme.spacingSmall
            }

            Grid {
                columns: 2
                spacing: 0
                Kit.ThemeSwatch {
                    themeName: qsTr("Light"); chipColor: Theme.chipLight
                    selected: Theme.theme === "Light"
                    onPicked: Theme.setTheme("Light")
                }
                Kit.ThemeSwatch {
                    themeName: qsTr("Dark"); chipColor: Theme.chipDark
                    selected: Theme.theme === "Dark"
                    onPicked: Theme.setTheme("Dark")
                }
                Kit.ThemeSwatch {
                    themeName: qsTr("Sepia"); chipColor: Theme.chipSepia
                    selected: Theme.theme === "Sepia"
                    onPicked: Theme.setTheme("Sepia")
                }
                Kit.ThemeSwatch {
                    themeName: qsTr("Midnight"); chipColor: Theme.chipMidnight
                    selected: Theme.theme === "Midnight"
                    onPicked: Theme.setTheme("Midnight")
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
