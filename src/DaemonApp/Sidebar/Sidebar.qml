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
        store: ChatStore
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Header: gear only (no title) ------------------------
        Item {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacingSmall
            Layout.leftMargin: Theme.spacingSmall
            implicitHeight: 32

            Kit.IconButton {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
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
            Layout.topMargin: Theme.spacingSmall
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
                    required property int nodeId
                    required property bool isSeparator
                    required property bool selectable
                    required property string color

                    width: ListView.view.width
                    height: isSeparator ? 32 : 34

                    readonly property bool isSelected: !isSeparator && index === root.currentRow
                    readonly property bool isTag: nodeType === 5

                    // Rounded selection / hover highlight, inset from edges.
                    Rectangle {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingSmall
                        anchors.rightMargin: Theme.spacingSmall
                        anchors.topMargin: 1
                        anchors.bottomMargin: 1
                        radius: Theme.radius
                        visible: !del.isSeparator && (del.isSelected || rowMouse.containsMouse)
                        color: del.isSelected ? Theme.selection : Theme.hover
                    }

                    // --- Separator row: uppercase label + "+" add button ----
                    RowLayout {
                        visible: del.isSeparator
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacing
                        anchors.rightMargin: Theme.spacingSmall
                        anchors.bottomMargin: 2
                        spacing: 0

                        QQC.Label {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignBottom
                            text: del.label
                            color: Theme.textMuted
                            font.family: FontIcons.display
                            font.pixelSize: 11
                            font.bold: true
                            font.capitalization: Font.AllUppercase
                            elide: Text.ElideRight
                        }

                        Kit.IconButton {
                            implicitWidth: 22
                            implicitHeight: 22
                            icon: FontIcons.fa_plus
                            iconPointSize: 11
                            iconColor: Theme.textMuted
                            tooltipText: del.nodeType === 2 ? qsTr("New folder") : qsTr("New tag")
                            onClicked: del.nodeType === 2 ? root.addFolderRequested()
                                                          : root.addTagRequested()
                        }
                    }

                    // --- Selectable row: icon/dot + label + count -----------
                    RowLayout {
                        visible: !del.isSeparator
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacing
                        anchors.rightMargin: Theme.spacing
                        spacing: Theme.spacingSmall

                        // Tag rows show a colored dot; others show a glyph.
                        Item {
                            Layout.preferredWidth: 16
                            Layout.preferredHeight: 16
                            Layout.alignment: Qt.AlignVCenter

                            Kit.Glyph {
                                anchors.centerIn: parent
                                visible: !del.isTag
                                glyph: root.iconFor(del.nodeType)
                                font.pointSize: 12 + Theme.pointSizeOffset
                                color: del.isSelected ? Theme.accent : Theme.iconMuted
                            }

                            Rectangle {
                                anchors.centerIn: parent
                                visible: del.isTag
                                width: 10
                                height: 10
                                radius: 5
                                color: del.color !== "" ? del.color : Theme.iconMuted
                            }
                        }

                        QQC.Label {
                            Layout.fillWidth: true
                            text: del.label
                            elide: Text.ElideRight
                            font.family: FontIcons.display
                            font.pixelSize: 14
                            font.bold: del.isSelected
                            color: del.isSelected ? Theme.accent : Theme.text
                        }

                        QQC.Label {
                            visible: del.count >= 0
                            text: del.count
                            font.family: FontIcons.display
                            font.pixelSize: 12
                            color: del.isSelected ? Theme.accent : Theme.countText
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
        x: Theme.spacingSmall
        y: Theme.spacingSmall + 34
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
