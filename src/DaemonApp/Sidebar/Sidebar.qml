import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Presentation

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

    // The category decision (which NodeType/Kind/State maps to which icon/tone)
    // lives once in the shared C++ DisplayPresenter; QML only resolves the
    // returned key/tone to the concrete FontIcons glyph / Theme color. Visuals
    // are unchanged - only the source of the decision moved out of QML.
    function iconFor(nodeType) {
        switch (DisplayPresenter.scopeIconKey(nodeType)) {
        case "comments": return FontIcons.fa_comments;
        case "archive": return FontIcons.fa_box_archive;
        default: return "";
        }
    }

    function kindIcon(kind) {
        switch (DisplayPresenter.agentKindIconKey(kind)) {
        case "sitemap": return FontIcons.fa_sitemap; // orchestrator / fleet
        case "server": return FontIcons.fa_server;   // host
        default: return FontIcons.fa_robot;          // engine / agent leaf
        }
    }

    function stateColor(state) {
        switch (DisplayPresenter.agentStateTone(state)) {
        case DisplayPresenter.StateTone.Running: return Theme.stateRunning;
        case DisplayPresenter.StateTone.Finished: return Theme.stateFinished;
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
                focus: true
                // Selection lives in the model (by identity), so disable the
                // built-in currentIndex navigation and drive it ourselves.
                keyNavigationEnabled: false

                // Keep the model's selected row visible after keyboard moves.
                function syncView() {
                    var r = sidebarModel.currentRow();
                    if (r >= 0)
                        list.positionViewAtIndex(r, ListView.Contain);
                }

                Keys.onUpPressed: { sidebarModel.selectPrevious(); syncView(); }
                Keys.onDownPressed: { sidebarModel.selectNext(); syncView(); }
                Keys.onLeftPressed: { sidebarModel.collapseCurrent(); syncView(); }
                Keys.onRightPressed: { sidebarModel.expandCurrent(); syncView(); }
                Keys.onReturnPressed: sidebarModel.activateCurrent()
                Keys.onEnterPressed: sidebarModel.activateCurrent()

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
                    required property bool current

                    width: ListView.view.width
                    // Dense column: compact section header / 28px nav row
                    // (finger-sized nav rows on touch via Theme.rowHeight).
                    height: isSeparator ? 30 : Theme.rowHeight

                    // Highlight by identity (the model's `current` role), so it
                    // survives expand/collapse rebuilds without index tracking.
                    readonly property bool isSelected: !isSeparator && current
                    readonly property bool isTag: nodeType === 5
                    readonly property bool isNode: nodeType === 4
                    // Row hover stays stable even when the cursor is over the
                    // twistie (which has its own hover-enabled hit area).
                    readonly property bool hovered: rowMouse.containsMouse || twMouse.containsMouse
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
                        opacity: del.isSelected || del.hovered ? 1 : 0
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
                            onClicked: del.nodeType === 2 ? sidebarModel.createRootNode()
                                                          : sidebarModel.createTag()
                        }

                        // Fleet header only: one-shot expand-all / collapse-all.
                        Kit.IconButton {
                            visible: del.nodeType === 2
                            anchors.right: parent.right
                            anchors.rightMargin: 34
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 2
                            implicitWidth: 30
                            implicitHeight: 24
                            icon: sidebarModel.anyExpanded ? FontIcons.fa_angles_up
                                                           : FontIcons.fa_angles_down
                            iconPointSize: 11
                            iconColor: Theme.iconMuted
                            tooltipText: sidebarModel.anyExpanded ? qsTr("Collapse all")
                                                                  : qsTr("Expand all")
                            onClicked: sidebarModel.anyExpanded ? sidebarModel.collapseAll()
                                                                : sidebarModel.expandAll()
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
                            // Sit above the full-row MouseArea so twistie clicks
                            // toggle expand instead of selecting the row.
                            z: 2

                            Kit.Glyph {
                                id: chevron
                                anchors.centerIn: parent
                                // A single chevron rotated 90deg when expanded,
                                // so the twistie animates smoothly on toggle.
                                glyph: FontIcons.fa_chevron_right
                                font.pointSize: 9 + Theme.pointSizeOffset
                                rotation: del.expanded ? 90 : 0
                                color: twMouse.containsMouse || del.isSelected || del.hovered
                                     ? Theme.sidebarSelectedText : Theme.sidebarIcon
                                Behavior on rotation {
                                    NumberAnimation { duration: Theme.motionFast }
                                }
                                Behavior on color {
                                    ColorAnimation { duration: Theme.motionFast }
                                }
                            }

                            MouseArea {
                                id: twMouse
                                anchors.fill: parent
                                // Enlarge the hit target a touch for easy clicking.
                                anchors.margins: -3
                                hoverEnabled: true
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
                                color: del.isSelected || del.hovered
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
                            color: del.isSelected || del.hovered
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
                            color: del.isSelected || del.hovered
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
                            list.forceActiveFocus();
                            sidebarModel.activate(del.index);
                        }
                        // Double-clicking anywhere on a node row toggles it,
                        // matching the twistie.
                        onDoubleClicked: {
                            list.forceActiveFocus();
                            if (del.isNode && del.hasChildren)
                                sidebarModel.toggleExpand(del.index);
                        }
                    }
                }
            }
        }
    }

    // --- Settings/manager page menu (opened by the cog) ---------------------
    // Every app-level page is reachable here; each item asks the shared Nav bus to
    // open that page as a tab (Theme now lives in Settings -> Appearance).
    Kit.Menu {
        id: settingsMenu
        x: parent.width - width - 3
        y: 31

        Kit.MenuItem {
            text: qsTr("Settings")
            onTriggered: Nav.open("settings")
        }
        Kit.MenuSeparator {}
        Kit.MenuItem {
            text: qsTr("Accounts")
            onTriggered: Nav.open("accounts")
        }
        Kit.MenuItem {
            text: qsTr("Profiles")
            onTriggered: Nav.open("profiles")
        }
        Kit.MenuItem {
            text: qsTr("Models")
            onTriggered: Nav.open("models")
        }
        Kit.MenuSeparator {}
        Kit.MenuItem {
            text: qsTr("Dashboard")
            onTriggered: Nav.open("dashboard")
        }
        Kit.MenuItem {
            text: qsTr("Fleet")
            onTriggered: Nav.open("fleet")
        }
        Kit.MenuItem {
            text: qsTr("Sessions")
            onTriggered: Nav.open("sessions")
        }
        Kit.MenuItem {
            text: qsTr("Approvals")
            onTriggered: Nav.open("approvals")
        }
        Kit.MenuItem {
            text: qsTr("Routing")
            onTriggered: Nav.open("routing")
        }
        Kit.MenuItem {
            text: qsTr("Scheduled jobs")
            onTriggered: Nav.open("cron")
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
