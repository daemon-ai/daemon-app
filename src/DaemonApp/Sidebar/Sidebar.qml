// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Presentation
import DaemonApp.Pages

// Left column: the agent supervision tree. A flattened, uniformly recursive
// tree (VSCode-explorer style) - any node can contain any node to any depth.
// Rows: All / Archived, a "Fleet" header with the indented node tree (twistie +
// kind icon + state dot + folded subtree count), and a "Tags" header with tag
// rows. Selecting a node lists its whole subtree's sessions in the middle.
Rectangle {
    id: root

    color: Theme.sidebar

    // nodeType is a domain::NodeType; `id` is the tag id (Tag scope), `nodeId`
    // is the agent node id (Node scope) / the lens key (ByTransport/ByPeer scopes).
    // Unused fields are -1 / "".
    signal scopeSelected(int nodeType, int id, string nodeId)
    // An Integrations-section session leaf was activated - open its transcript.
    signal sessionActivated(string sessionId)
    // An UNPINNED external conversation leaf was activated (B4): browse-only - the shell opens
    // the Channels page (pin explicitly from there / the routing manager).
    signal conversationActivated(string transport, string conversation)
    // "+ New agent/node" (Fleet header) - open a fresh chat (routed by the shell to the same
    // open-a-chat path the first-run completion uses).
    signal newChatRequested()
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

    // Integrations-section row icon: the transport account, its conversation groups,
    // and the per-ConversationType conversation leaves / generic job & caller leaves.
    function transportIcon(txKind, convType) {
        switch (txKind) {
        case "account": return FontIcons.fa_server;
        case "convGroup": return FontIcons.fa_comments;
        case "job": return FontIcons.fa_clock;
        case "caller": return FontIcons.fa_globe;
        case "conversation":
            switch (convType) {
            case "channel": return FontIcons.fa_hashtag;
            case "dm": return FontIcons.fa_user;
            case "groupdm": return FontIcons.fa_users;
            default: return FontIcons.fa_comments;
            }
        default: return FontIcons.fa_comments;
        }
    }

    // Presence dot color for a transport account (the faithful PresencePrimitive,
    // coarsened to the sidebar's three tones).
    function presenceColor(presence) {
        switch (presence) {
        case "available": return Theme.stateRunning;
        case "": return Theme.transparent;
        default: return Theme.stateFinished;
        }
    }

    SidebarModel {
        id: sidebarModel
        objectName: "fleetSidebarModel"
        store: SessionStoreMirror
        // The dedicated IntegrationsTree below owns the integrations surface (persons/spaces/browse
        // + account add/edit/remove + interactive auth); the legacy fleet-sidebar integrations
        // rendering was deleted in M3 (the routing manager now reads the mirror store). This model
        // renders only the Fleet + Tags sections.
        // Agents == profiles (§0): the FLEET section renders one agent row per ProfileInfo.
        profiles: Profiles
        // The node/connection ROOT label: the connection target for a remote node, else a generic
        // local-node label (the cheap existing node identity; no node-info contract change).
        nodeLabel: (typeof Connection !== "undefined" && Connection && Connection.target
                    && Connection.mode !== "local")
                   ? Connection.target : qsTr("Local node")
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Header: gear, right-aligned ------------------------------------
        Item {
            Layout.fillWidth: true
            implicitHeight: 28

            Kit.IconButton {
                id: settingsCog
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
            // TOOL-8: mirror the Approvals count on the always-visible cog so pending gates are
            // surfaced without opening the menu. Same event-driven Approvals view-model.
            Kit.Badge {
                anchors.right: settingsCog.right
                anchors.top: settingsCog.top
                anchors.rightMargin: -2
                anchors.topMargin: -2
                count: Approvals.count
            }
        }

        // --- Rows -----------------------------------------------------------
        // A vertical split: the Fleet/Tags supervision tree on top, the co-equal Integrations
        // surface (work package AC) below. The Integrations section moved out of the fleet
        // SidebarModel into the dedicated IntegrationsTree (persons/spaces/browse + account
        // management + interactive auth); mounting it here is what makes that surface reachable.
        QQC.SplitView {
            orientation: Qt.Vertical
            Layout.fillWidth: true
            Layout.fillHeight: true

            handle: Rectangle {
                implicitHeight: 1
                color: QQC.SplitHandle.pressed || QQC.SplitHandle.hovered ? Theme.accent
                                                                          : Theme.splitter
            }

            QQC.ScrollView {
                QQC.SplitView.fillHeight: true
                QQC.SplitView.minimumHeight: 120
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
                    required property string unitId
                    required property bool isSeparator
                    required property bool selectable
                    required property string color
                    required property int depth
                    required property bool hasChildren
                    required property bool expanded
                    required property int kind
                    required property int state
                    required property bool current
                    required property string profile
                    required property string sessionId
                    required property string txKind
                    required property string convType
                    required property string subLabel
                    required property string presence

                    width: ListView.view.width
                    // Dense column: compact section header / 28px nav row
                    // (finger-sized nav rows on touch via Theme.rowHeight).
                    height: isSeparator ? 30 : Theme.rowHeight

                    // Highlight by identity (the model's `current` role), so it
                    // survives expand/collapse rebuilds without index tracking.
                    readonly property bool isSelected: !isSeparator && current
                    readonly property bool isTag: nodeType === 5
                    readonly property bool isNode: nodeType === 4
                    readonly property bool isTransport: nodeType === 9
                    // Fleet membership rows (§0): node/connection root (10), agent (11), session (12).
                    readonly property bool isFleetNode: nodeType === 10
                    readonly property bool isAgent: nodeType === 11
                    readonly property bool isAgentSession: nodeType === 12
                    // Transport account/convGroup + fleet node/agent rows are expandable.
                    readonly property bool isTwistable: (isNode || isTransport || isFleetNode
                                                         || isAgent) && hasChildren
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

                    // --- Separator row: collapsible section header (twistie +
                    //     label + blue "+" add button) -------------------------
                    Item {
                        anchors.fill: parent
                        visible: del.isSeparator

                        // Whole-header click folds/unfolds the section (Fleet / Tags
                        // / Integrations). The "+" and expand-all buttons are declared
                        // after this and sit on top, so they handle their own clicks.
                        MouseArea {
                            anchors.fill: parent
                            enabled: del.hasChildren
                            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onClicked: sidebarModel.toggleExpand(del.index)
                        }

                        // Section disclosure chevron (rotated 90deg when expanded).
                        Kit.Glyph {
                            id: sectionChevron
                            visible: del.hasChildren
                            anchors.left: parent.left
                            anchors.leftMargin: 12
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 6
                            glyph: FontIcons.fa_chevron_right
                            font.pointSize: 8 + Theme.pointSizeOffset
                            rotation: del.expanded ? 90 : 0
                            color: Theme.separatorText
                            Behavior on rotation {
                                NumberAnimation { duration: Theme.motionFast }
                            }
                        }

                        QQC.Label {
                            anchors.left: parent.left
                            anchors.leftMargin: del.hasChildren ? 28 : 14
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
                            // Fleet (2) adds a root node; Tags (3) adds a tag; the
                            // Integrations (8) header is read-only (mock-seeded).
                            visible: del.nodeType === 2 || del.nodeType === 3
                            anchors.right: parent.right
                            anchors.rightMargin: 4
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 2
                            implicitWidth: 30
                            implicitHeight: 24
                            icon: FontIcons.fa_plus
                            iconPointSize: 12
                            iconColor: Theme.addButton
                            tooltipText: del.nodeType === 2 ? qsTr("New agent") : qsTr("New tag")
                            onClicked: del.nodeType === 2 ? sidebarModel.createRootUnit()
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

                        // Integrations header: the same one-shot expand-all /
                        // collapse-all over the transport tree. No "+" button here,
                        // so it sits at the right edge.
                        Kit.IconButton {
                            visible: del.nodeType === 8
                            anchors.right: parent.right
                            anchors.rightMargin: 4
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 2
                            implicitWidth: 30
                            implicitHeight: 24
                            icon: sidebarModel.anyTransportExpanded ? FontIcons.fa_angles_up
                                                                    : FontIcons.fa_angles_down
                            iconPointSize: 11
                            iconColor: Theme.iconMuted
                            tooltipText: sidebarModel.anyTransportExpanded ? qsTr("Collapse all")
                                                                           : qsTr("Expand all")
                            onClicked: sidebarModel.anyTransportExpanded
                                ? sidebarModel.collapseAllTransports()
                                : sidebarModel.expandAllTransports()
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
                            visible: del.isTwistable
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

                            // Node kind icon (cosmetic), transport-tree icon, or
                            // the All/Archived scope icon.
                            Kit.Glyph {
                                anchors.centerIn: parent
                                visible: !del.isTag
                                glyph: del.isNode ? root.kindIcon(del.kind)
                                     : del.isTransport ? root.transportIcon(del.txKind, del.convType)
                                     : del.isFleetNode ? FontIcons.fa_server
                                     : del.isAgent ? FontIcons.fa_robot
                                     : del.isAgentSession ? FontIcons.fa_comments
                                                       : root.iconFor(del.nodeType)
                                font.pointSize: 12 + Theme.pointSizeOffset
                                color: del.isSelected || del.hovered
                                     ? Theme.sidebarSelectedText : Theme.sidebarIcon
                            }

                            // Presence dot on a transport account (bottom-right of
                            // its icon), the coarsened PresencePrimitive.
                            Rectangle {
                                visible: del.isTransport && del.txKind === "account"
                                      && del.presence !== ""
                                width: 6
                                height: 6
                                radius: 3
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                color: root.presenceColor(del.presence)
                                border.width: 1
                                border.color: Theme.sidebar
                            }
                        }

                        QQC.Label {
                            id: rowLabel
                            anchors.left: iconBox.right
                            anchors.leftMargin: del.isTag ? 11 : 5
                            anchors.right: subLabelText.visible ? subLabelText.left : stateDot.left
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

                        // Muted secondary label that elides before the main label: a transport
                        // leaf's inline session title ("#secops > triage"). Fleet agent rows
                        // carry no sublabel — provider/model detail lives in Settings > Profiles.
                        QQC.Label {
                            id: subLabelText
                            visible: del.isTransport && del.subLabel !== ""
                            anchors.right: stateDot.left
                            anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            width: Math.min(implicitWidth, parent.width * 0.42)
                            text: del.subLabel
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignRight
                            font.family: FontIcons.display
                            font.pixelSize: 11
                            color: Theme.countText
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
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: function(mouse) {
                            list.forceActiveFocus();
                            // Right-click an agent (a profile-backed unit) for its
                            // per-agent Profile + Memory surfaces.
                            if (mouse.button === Qt.RightButton) {
                                if ((del.isNode || del.isAgent) && del.profile.length > 0)
                                    agentMenu.openFor(del.profile, del.label);
                                return;
                            }
                            sidebarModel.activate(del.index);
                        }
                        // Double-clicking anywhere on a node row toggles it,
                        // matching the twistie.
                        onDoubleClicked: {
                            list.forceActiveFocus();
                            if (del.isTwistable)
                                sidebarModel.toggleExpand(del.index);
                        }
                    }
                }
            }
            }

            // The co-equal Integrations surface (work package AC): the finished IntegrationsTree,
            // now placed into the sidebar. Activating a room/DM opens a native chat tab through the
            // shell's conversation-activation dispatch (A4); the account menu / add-edit-remove +
            // interactive auth are self-contained in the tree.
            IntegrationsTree {
                id: integrationsTree
                QQC.SplitView.preferredHeight: root.height * 0.42
                QQC.SplitView.minimumHeight: 90
                onConversationActivated: function(transport, conversation) {
                    root.conversationActivated(transport, conversation);
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
            // TOOL-8: pending-approvals count, event-driven off the shared Approvals view-model.
            badgeCount: Approvals.count
            onTriggered: Nav.open("approvals")
        }
        Kit.MenuItem {
            text: qsTr("Channels")
            onTriggered: Nav.open("channels")
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

    // Per-agent context menu (right-click a profile-backed unit in the Fleet tree).
    // Memory and Profile are owned by the agent (== its profile), so both open a
    // tab keyed by the unit's ProfileRef.
    Kit.Menu {
        id: agentMenu
        property string targetProfile: ""
        property string targetTitle: ""

        function openFor(profile, title) {
            agentMenu.targetProfile = profile;
            agentMenu.targetTitle = title;
            popup();
        }

        // SEPARATE "new session" affordance (distinct from the Fleet "+" which mints an agent):
        // open a fresh chat under this agent. The session binds to the agent's profile.
        Kit.MenuItem {
            text: qsTr("New chat")
            onTriggered: root.newChatRequested()
        }
        Kit.MenuSeparator {}
        Kit.MenuItem {
            text: qsTr("Profile settings")
            onTriggered: Nav.openAgent("profile", agentMenu.targetProfile, agentMenu.targetTitle)
        }
        Kit.MenuItem {
            text: qsTr("Memory")
            onTriggered: Nav.openAgent("memory", agentMenu.targetProfile, agentMenu.targetTitle)
        }
    }

    // "+ New agent" (A7 + foreign engines): the Kit-themed create-agent dialog — Name + engine
    // picker (daemon-core / ACP catalog entries) + the shared AgentInferencePicker for the native
    // engine. Extracted to NewAgentDialog.qml; this file only instantiates it.
    NewAgentDialog {
        id: newAgentDialog
    }

    Connections {
        target: sidebarModel
        function onScopeSelected(nodeType, id, nodeId) {
            root.scopeSelected(nodeType, id, nodeId);
        }
        function onSessionActivated(sessionId) {
            root.sessionActivated(sessionId);
        }
        function onConversationActivated(transport, conversation) {
            root.conversationActivated(transport, conversation);
        }
        function onNewChatRequested() {
            root.newChatRequested();
        }
        function onCreateAgentRequested() {
            newAgentDialog.openForm();
        }
    }

    Component.onCompleted: sidebarModel.activate(0)
}
