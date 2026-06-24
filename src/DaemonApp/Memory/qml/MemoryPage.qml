import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Pages

// The Memory page: inspect ONE agent's Mnemosyne memory. Memory is owned by the
// agent (== its profile / per-profile bank), so this tab is bound to a `profile`
// (set from the tab's profile) and shows that agent's WHOLE bank by default. A
// session == a conversation; the session filter is an optional lens within the
// agent's memory (not the binding), and `scope` (session/global) is row metadata.
// Internal sub-tabs: Overview / Memories / Graph / Timeline. Graph is GUI-only;
// the TUI mirrors the rest plus a graph adjacency fallback.
Item {
    id: root

    // The agent identity (ProfileRef == the Mnemosyne bank) this tab inspects.
    property string profile: ""

    // Deep-link target (a sub-tab id); defaults to Overview.
    property string section: "overview"

    readonly property var subtabs: [
        { id: "overview", label: qsTr("Overview"), icon: FontIcons.fa_gauge_high },
        { id: "memories", label: qsTr("Memories"), icon: FontIcons.fa_list_ul },
        { id: "graph", label: qsTr("Graph"), icon: FontIcons.fa_sitemap },
        { id: "timeline", label: qsTr("Timeline"), icon: FontIcons.fa_clock }
    ]

    function _current() {
        return root.section && root.section.length > 0 ? root.section : "overview";
    }

    // Session FILTER over the agent's bank: index 0 = "All conversations" (whole
    // bank); the rest are the agent's conversations (populated from sessionsReady).
    property var sessionFilter: [{ id: "", label: qsTr("All conversations") }]
    property bool includeGlobal: true

    // Apply the agent binding + the current session/global filter. `profile` is the
    // primary key (the bank); an empty session means the whole bank.
    function applyScope() {
        if (typeof Memory === "undefined" || !Memory || root.profile.length === 0)
            return;
        const idx = Math.max(0, sessionPicker.currentIndex);
        const session = idx < root.sessionFilter.length ? root.sessionFilter[idx].id : "";
        Memory.setScope(root.profile, session, root.includeGlobal);
    }

    onProfileChanged: {
        if (typeof Memory !== "undefined" && Memory && root.profile.length > 0) {
            sessionPicker.currentIndex = 0;
            Memory.setScope(root.profile, "", root.includeGlobal); // whole bank by default
            Memory.requestSessions(root.profile);
        }
    }

    Component.onCompleted: {
        if (root.profile.length > 0) {
            Memory.setScope(root.profile, "", root.includeGlobal);
            Memory.requestSessions(root.profile);
        }
    }

    // Populate the session filter facet with the agent's conversations.
    Connections {
        target: typeof Memory !== "undefined" ? Memory : null
        function onSessionsReady(profile, sessions) {
            if (profile !== root.profile)
                return;
            const list = [{ id: "", label: qsTr("All conversations") }];
            for (let i = 0; i < sessions.length; ++i)
                list.push({ id: sessions[i].id, label: sessions[i].label });
            root.sessionFilter = list;
            sessionPicker.currentIndex = 0;
        }
    }

    PageHeader {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr("Memory")
        icon: FontIcons.fa_brain
    }

    // --- Session filter (an optional lens over the agent's whole bank) ------
    RowLayout {
        id: scopeRow
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: 10
        spacing: 12

        Text {
            text: qsTr("Conversation")
            font.family: FontIcons.display
            font.pixelSize: 12
            color: Theme.textMuted
        }
        Kit.Dropdown {
            id: sessionPicker
            model: root.sessionFilter.map(function(s) { return s.label; })
            currentIndex: 0
            Layout.preferredWidth: 200
            onActivated: root.applyScope()
        }
        Kit.TextButton {
            text: root.includeGlobal ? qsTr("Global: on") : qsTr("Global: off")
            onClicked: {
                root.includeGlobal = !root.includeGlobal;
                root.applyScope();
            }
        }
        Item { Layout.fillWidth: true }
    }

    // --- Sub-tab strip ------------------------------------------------------
    RowLayout {
        id: tabStrip
        anchors.top: scopeRow.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: 10
        spacing: 4

        Repeater {
            model: root.subtabs
            delegate: Rectangle {
                id: tabChip
                required property var modelData
                readonly property bool isCurrent: root._current() === modelData.id
                implicitWidth: chipBody.implicitWidth + 24
                implicitHeight: 30
                radius: 6
                color: isCurrent ? Theme.hover : (chipHover.hovered ? Theme.hover : "transparent")
                opacity: isCurrent ? 1.0 : (chipHover.hovered ? 0.7 : 1.0)

                RowLayout {
                    id: chipBody
                    anchors.centerIn: parent
                    spacing: 7
                    Text {
                        text: tabChip.modelData.icon
                        font.family: FontIcons.faSolid
                        font.pixelSize: 12
                        color: tabChip.isCurrent ? Theme.accent : Theme.iconMuted
                    }
                    Text {
                        text: tabChip.modelData.label
                        font.family: FontIcons.display
                        font.pixelSize: 13
                        font.weight: tabChip.isCurrent ? Font.DemiBold : Font.Normal
                        color: tabChip.isCurrent ? Theme.text : Theme.textMuted
                    }
                }
                HoverHandler { id: chipHover }
                TapHandler { onTapped: root.section = tabChip.modelData.id }
            }
        }
        Item { Layout.fillWidth: true }
    }

    Rectangle {
        anchors.top: tabStrip.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 8
        height: 1
        color: Theme.border
    }

    // --- Active sub-view ----------------------------------------------------
    Loader {
        id: content
        anchors.top: tabStrip.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.topMargin: 9
        clip: true
        sourceComponent: {
            switch (root._current()) {
            case "memories": return memoriesComp;
            case "graph": return graphComp;
            case "timeline": return timelineComp;
            default: return overviewComp;
            }
        }
    }

    Component { id: overviewComp; MemoryOverview {} }
    Component { id: memoriesComp; MemoryList {} }
    Component { id: graphComp; MemoryGraphView {} }
    Component { id: timelineComp; MemoryTimeline {} }
}
