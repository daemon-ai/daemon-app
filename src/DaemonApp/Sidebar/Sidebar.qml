import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

Rectangle {
    id: root

    color: Theme.sidebar

    signal scopeSelected(int nodeType, int nodeId)
    signal settingsRequested()

    property int currentRow: 0

    // Glyph per node type (enum: AllConversations, Archived, FolderSep, TagSep,
    // Folder, Tag).
    function iconFor(nodeType) {
        switch (nodeType) {
        case 0: return FontIcons.fa_comments;
        case 1: return FontIcons.fa_box_archive;
        case 4: return FontIcons.fa_folder;
        case 5: return FontIcons.fa_tag;
        default: return "";
        }
    }

    SidebarModel {
        id: sidebarModel
        store: ChatStore
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Header: app title + settings cog -------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacing
            Layout.rightMargin: Theme.spacingSmall
            Layout.topMargin: Theme.spacing
            Layout.bottomMargin: Theme.spacingSmall
            spacing: Theme.spacingSmall

            QQC.Label {
                Layout.fillWidth: true
                text: qsTr("daemon-app")
                color: Theme.text
                font.family: FontIcons.display
                font.bold: true
                font.pixelSize: 16
                elide: Text.ElideRight
            }

            Kit.IconButton {
                icon: FontIcons.fa_gear
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
                boundsBehavior: Flickable.StopAtBounds

                delegate: Item {
                    id: del
                    required property int index
                    required property string label
                    required property int count
                    required property int nodeType
                    required property bool isSeparator
                    required property bool selectable

                    width: ListView.view.width
                    height: isSeparator ? 30 : 34

                    readonly property bool isSelected: !isSeparator && index === root.currentRow

                    Rectangle {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingSmall
                        anchors.rightMargin: Theme.spacingSmall
                        radius: Theme.radius
                        visible: !del.isSeparator
                        color: del.isSelected ? Theme.selection
                             : rowMouse.containsMouse ? Theme.hover
                             : "transparent"
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacing
                        anchors.rightMargin: Theme.spacing
                        spacing: Theme.spacingSmall

                        Text {
                            visible: !del.isSeparator && text !== ""
                            text: root.iconFor(del.nodeType)
                            font.family: FontIcons.faSolid
                            font.pixelSize: 13
                            color: del.isSelected ? Theme.accent : Theme.textMuted
                        }

                        QQC.Label {
                            Layout.fillWidth: true
                            text: del.label
                            elide: Text.ElideRight
                            font.family: FontIcons.display
                            color: del.isSeparator ? Theme.textMuted
                                 : del.isSelected ? Theme.accent : Theme.text
                            font.bold: del.isSeparator || del.isSelected
                            font.pixelSize: del.isSeparator ? 11 : 14
                            font.capitalization: del.isSeparator ? Font.AllUppercase : Font.MixedCase
                        }

                        QQC.Label {
                            visible: del.count >= 0
                            text: del.count
                            font.family: FontIcons.display
                            color: Theme.countText
                            font.pixelSize: 12
                        }
                    }

                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: !del.isSeparator
                        enabled: del.selectable
                        cursorShape: del.selectable ? Qt.PointingHandCursor : Qt.ArrowCursor
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
        x: parent.width - width - Theme.spacingSmall
        y: Theme.spacing + 28
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
                font.pixelSize: 11
                font.capitalization: Font.AllUppercase
                Layout.leftMargin: Theme.spacingSmall
            }

            RowLayout {
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
