import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Left column: the agent supervision tree. A flattened, uniformly recursive
// tree (VSCode-explorer style) - any node can contain any node to any depth.
// Rows: All / Archived, a "Fleet" header with the indented node tree (twistie +
// kind icon + state dot + folded subtree count), and a "Tags" header with tag
// rows. Selecting a node lists its whole subtree's conversations in the middle.
Rectangle {
    id: root

    color: Theme.sidebar

    // nodeType is a domain::NodeType; `id` is the tag id (Tag scope), `nodeId`
    // is the agent node id (Node scope). Unused fields are -1 / "".
    signal scopeSelected(int nodeType, int id, string nodeId)
    signal settingsRequested()
    signal addRootRequested()
    signal addTagRequested()

    property int currentRow: 0

    // NodeType enum: 0 AllConversations, 1 Archived, 2 FleetSep, 3 TagSep,
    // 4 Node, 5 Tag.
    function iconFor(nodeType) {
        switch (nodeType) {
        case 0: return FontIcons.fa_comments;
        case 1: return FontIcons.fa_box_archive;
        default: return "";
        }
    }

    // AgentNodeKind: 0 Engine, 1 Host, 2 Orchestrator. Cosmetic only.
    function kindIcon(kind) {
        switch (kind) {
        case 2: return FontIcons.fa_sitemap; // orchestrator / fleet
        case 1: return FontIcons.fa_server;  // host
        default: return FontIcons.fa_robot;  // engine / agent leaf
        }
    }

    // AgentState: 0 Running, 1 Finished, 2 Unknown.
    function stateColor(state) {
        switch (state) {
        case 0: return Theme.stateRunning;
        case 1: return Theme.stateFinished;
        default: return Theme.transparent;
        }
    }

    SidebarModel {
        id: sidebarModel
        store: ConversationStore
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Header: gear, right-aligned ------------------------------------
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
                    required property string agentId
                    required property bool isSeparator
                    required property bool selectable
                    required property string color
                    required property int depth
                    required property bool hasChildren
                    required property bool expanded
                    required property int kind
                    required property int state

                    width: ListView.view.width
                    // Dense column: compact section header / 28px nav row.
                    height: isSeparator ? 30 : 28

                    readonly property bool isSelected: !isSeparator && index === root.currentRow
                    readonly property bool isTag: nodeType === 5
                    readonly property bool isNode: nodeType === 4
                    // Left edge of this row's content (twistie gutter), indented
                    // by depth. Works for any depth, no level caps.
                    readonly property int indentBase: 14 + depth * Theme.treeIndent

                    // Inset rounded selection pill, inset from the column edges.
                    Rectangle {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.rowInset
                        anchors.rightMargin: Theme.rowInset
                        anchors.topMargin: Theme.rowVInset
                        anchors.bottomMargin: Theme.rowVInset
                        radius: Theme.rowRadius
                        visible: !del.isSeparator
                        color: del.isSelected ? Theme.sidebarSelection : Theme.sidebarHover
                        opacity: del.isSelected || rowMouse.containsMouse ? 1 : 0
                        border.width: del.isSelected ? 1 : 0
                        border.color: Theme.border

                        Behavior on opacity {
                            NumberAnimation { duration: Theme.motionFast }
                        }
                    }

                    // --- Separator row: label + blue "+" add button ----------
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
                            tooltipText: del.nodeType === 2 ? qsTr("New root node") : qsTr("New tag")
                            onClicked: del.nodeType === 2 ? root.addRootRequested()
                                                          : root.addTagRequested()
                        }
                    }

                    // --- Selectable row: twistie + icon/dot + label + count --
                    Item {
                        anchors.fill: parent
                        visible: !del.isSeparator

                        // Twistie (chevron) in the indent gutter; only nodes with
                        // children show it. Its own hit-area toggles expand
                        // WITHOUT changing the selection (VSCode behavior).
                        Item {
                            id: twistie
                            anchors.left: parent.left
                            anchors.leftMargin: del.indentBase
                            anchors.verticalCenter: parent.verticalCenter
                            width: Theme.twistieSize
                            height: Theme.twistieSize
                            visible: del.isNode && del.hasChildren

                            Kit.Glyph {
                                anchors.centerIn: parent
                                glyph: del.expanded ? FontIcons.fa_chevron_down
                                                    : FontIcons.fa_chevron_right
                                font.pointSize: 9 + Theme.pointSizeOffset
                                color: del.isSelected || rowMouse.containsMouse
                                     ? Theme.sidebarSelectedText : Theme.sidebarIcon
                            }

                            MouseArea {
                                anchors.fill: parent
                                // Enlarge the hit target a touch for easy clicking.
                                anchors.margins: -3
                                cursorShape: Qt.PointingHandCursor
                                onClicked: sidebarModel.toggleExpand(del.index)
                            }
                        }

                        // Icon box, after the twistie gutter so siblings align.
                        Item {
                            id: iconBox
                            anchors.left: parent.left
                            anchors.leftMargin: del.indentBase + Theme.twistieSize
                            anchors.verticalCenter: parent.verticalCenter
                            width: 18
                            height: 18

                            // Tag dot (fa_circle, in the tag color).
                            Kit.Glyph {
                                anchors.centerIn: parent
                                visible: del.isTag
                                glyph: FontIcons.fa_circle
                                font.pointSize: 12 + Theme.pointSizeOffset
                                color: del.isSelected ? Theme.sidebarSelectedText
                                     : del.color !== "" ? del.color : Theme.sidebarIcon
                            }

                            // Node kind icon (cosmetic) or All/Archived icon.
                            Kit.Glyph {
                                anchors.centerIn: parent
                                visible: !del.isTag
                                glyph: del.isNode ? root.kindIcon(del.kind)
                                                  : root.iconFor(del.nodeType)
                                font.pointSize: 12 + Theme.pointSizeOffset
                                color: del.isSelected || rowMouse.containsMouse
                                     ? Theme.sidebarSelectedText : Theme.sidebarIcon
                            }
                        }

                        QQC.Label {
                            anchors.left: iconBox.right
                            anchors.leftMargin: del.isTag ? 11 : 5
                            anchors.right: stateDot.left
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

                        // State dot: a small lifecycle indicator for node rows.
                        Rectangle {
                            id: stateDot
                            anchors.right: countLabel.left
                            anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            width: 7
                            height: 7
                            radius: 3.5
                            visible: del.isNode && del.state !== 2
                            color: root.stateColor(del.state)
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
        function onScopeSelected(nodeType, id, nodeId) {
            root.scopeSelected(nodeType, id, nodeId);
        }
    }

    Component.onCompleted: sidebarModel.activate(0)
}
