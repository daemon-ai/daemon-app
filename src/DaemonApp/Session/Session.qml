// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Dialogs
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Settings
import DaemonApp.Tabs
import DaemonApp.Pages
import DaemonApp.Memory
import DaemonApp.Terminal

// The session pane host, built as two stacked containers with EXPLICIT
// anchored geometry (no outer ColumnLayout): a fixed-height tab bar at the top
// and the transcript body filling the rest. The previous nested-Layout shell let
// the tab strip's height balloon and swallow the transcript; anchoring the bar to
// a hard height makes that impossible. Each transcript tab is its own
// TranscriptPage (own controller + orchestrator, so background/pinned tabs keep
// streaming + scroll state) kept alive in a Repeater and shown/hidden by the
// shared TabModel's currentIndex. The settings "..." popup drops from the bar.
Rectangle {
    id: root

    color: Theme.background

    // The fixed height of the top tab bar. A constant so the bar can never grow
    // into the transcript region.
    readonly property int barHeight: 36

    // Compact (phone) only: pop back to the session list.
    signal backRequested()

    // A page forwarded a window-level command (help / title / save); the shell
    // (Main.qml) routes it.
    signal paneCommandForwarded(string command)

    // A throwaway controller used solely to create new sessions in the
    // shared store; each TranscriptPage owns its own controller for display.
    SessionController {
        id: creator
        store: SessionStore
    }

    TabModel {
        id: tabModel
    }

    // The active tab's TranscriptPage (null when no transcript tab is active), so
    // the settings popup's "Move to Trash" can act on the open session.
    readonly property Item activePage: {
        const ld = tabRepeater.itemAt(tabModel.currentIndex);
        return (ld && ld.item) ? ld.item : null;
    }

    // --- Public API used by the shell (Main.qml) ----------------------------
    // The canonical session title (the same string the list shows), with a
    // generic fallback so a chip is never blank.
    function _titleFor(sessionId) {
        const t = SessionStore.title(sessionId);
        return (t && t.length > 0) ? t : qsTr("Session");
    }
    // Single-click / type-ahead open: load the session into the transient
    // preview tab (reused on the next preview), VSCode-style.
    function open(sessionId) {
        openSession(sessionId);
    }
    function openSession(sessionId) {
        tabModel.previewTranscript(sessionId, _titleFor(sessionId));
    }
    // Deliberate open (list double-click): a permanent, pinned tab. Passing the
    // real title (same as preview) keeps the pin from clobbering the chip label.
    function openSessionPinned(sessionId) {
        tabModel.openTranscriptPinned(sessionId, _titleFor(sessionId));
    }
    // Open a workspace file as an editor tab (File kind). pinned=false is a
    // VSCode-style preview (reused on the next single-click open); pinned=true
    // makes a permanent tab. The File tab loads a FilePage -> CodeEditor.
    function openFile(rootId, path, pinned) {
        const title = path.length > 0 ? path.substring(path.lastIndexOf('/') + 1) : qsTr("File");
        if (pinned)
            tabModel.openFilePinned(rootId, path, title);
        else
            tabModel.previewFile(rootId, path, title);
    }
    // Create a brand-new session and open it in a pinned tab.
    function createNew() {
        const id = creator.createSession("");
        tabModel.openTranscriptPinned(id, _titleFor(id));
    }
    // Open an app-level manager/settings page as a singleton tab (the GUI's
    // replacement for the old Nav modal overlay). Mirrors the TUI's
    // RootWidget::openManagerPage routing. `section` deep-links into pages that
    // expose one (Settings / Models).
    function openManagerPage(pageId, section) {
        const map = {
            "settings":  [TabModel.Settings,  qsTr("Settings")],
            "models":    [TabModel.Models,    qsTr("Models")],
            "accounts":  [TabModel.Accounts,  qsTr("Accounts")],
            "profiles":  [TabModel.Profiles,  qsTr("Profiles")],
            "fleet":     [TabModel.Fleet,     qsTr("Fleet")],
            "sessions":  [TabModel.Sessions,  qsTr("Sessions")],
            "dashboard": [TabModel.Dashboard, qsTr("Dashboard")],
            "approvals": [TabModel.Approvals, qsTr("Approvals")],
            "routing":   [TabModel.Routing,   qsTr("Routing")],
            "cron":      [TabModel.Cron,      qsTr("Scheduled jobs")],
            "channels":  [TabModel.Channels,  qsTr("Channels")],
            "access":    [TabModel.UsersAccess, qsTr("Users & Access")],
        };
        // Capability gate (auth6): the Users & Access admin page never mounts unless the
        // authenticated principal holds access_admin. The node also enforces this server-side.
        if (pageId === "access" && !(typeof Principal !== "undefined"
                                     && Principal && Principal.hasCapability("access_admin")))
            return;
        const entry = map[pageId];
        if (!entry)
            return;
        tabModel.openPage(entry[0], entry[1]);
        if (section && section.length > 0) {
            const ld = tabRepeater.itemAt(tabModel.currentIndex);
            if (ld && ld.item && "section" in ld.item)
                ld.item.section = section;
        }
    }

    // Open (or re-activate) a per-agent surface keyed by the agent's ProfileRef.
    // `kind` is "memory" or "profile"; the loaded page binds its `profile` from
    // the tab's profile (see the Loader.onLoaded above). Memory and Profile are
    // owned by the agent, so opening the same agent re-uses its existing tab.
    function openAgentTab(kind, profileRef, title) {
        if (!profileRef || profileRef.length === 0)
            return;
        const map = {
            "memory":  [TabModel.Memory,  qsTr("Memory")],
            "profile": [TabModel.Profile, qsTr("Profile")],
        };
        const entry = map[kind];
        if (!entry)
            return;
        const label = entry[1] + (title && title.length > 0 ? " \u00b7 " + title : "");
        tabModel.openAgentTab(entry[0], profileRef, label);
    }
    // Route a command (palette / slash) to the active tab's orchestrator, so the
    // command palette can drive the foreground session. No-op without a tab.
    function invokeActiveCommand(command) {
        if (root.activePage && root.activePage.invokeCommand)
            root.activePage.invokeCommand(command);
    }

    // Rename / export the active session (the /title + /save targets, and the
    // command palette's session actions), acting through the shared store + exporter.
    function renameActive() {
        if (root.activePage && root.activePage.sessionController)
            renameDialog.openFor(root.activePage.sessionController.currentId);
    }
    function exportActive() {
        if (root.activePage && root.activePage.sessionController)
            exportDialog.openFor(root.activePage.sessionController.currentId);
    }
    // Open the settings popup over the pane, bound to the active session.
    function openSettings() {
        settingsMenu.controller = root.activePage && root.activePage.sessionController
                                ? root.activePage.sessionController : null;
        settingsMenu.open();
    }

    // --- Bar 1: the tab strip header (fixed height, top) --------------------
    Rectangle {
        id: tabBarBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: root.barHeight
        // Distraction-free hides the chrome (Esc exits).
        visible: !UiSettings.distractionFree
        color: Theme.background

        // A hairline under the bar separates it from the transcript.
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: Theme.border
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 4

            // Compact (phone) only: pop back to the session list.
            Kit.IconButton {
                icon: FontIcons.fa_chevron_left
                iconColor: Theme.iconMuted
                implicitWidth: 30
                implicitHeight: 28
                iconPointSize: 16
                visible: LayoutState.isCompact
                tooltipText: qsTr("Back")
                onClicked: root.backRequested()
            }

            TabBar {
                Layout.fillWidth: true
                Layout.fillHeight: true
                tabModel: tabModel
                onNewTabRequested: root.createNew()
                onSettingsRequested: root.openSettings()
            }
        }
    }

    // --- Bar 2: the transcript body + the optional bottom terminal ----------
    // A vertical split (VSCode-style): the tab content fills the top, and the
    // embedded bash terminal docks at the bottom when toggled on from the status
    // bar. SplitView excludes an invisible pane from the layout, so when the
    // terminal is hidden the body reclaims the full height (and the handle with
    // it), leaving the original single-pane geometry intact.
    QQC.SplitView {
        id: lowerSplit
        orientation: Qt.Vertical
        anchors.top: tabBarBar.visible ? tabBarBar.bottom : parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        handle: Rectangle {
            implicitHeight: 1
            color: QQC.SplitHandle.pressed || QQC.SplitHandle.hovered
                 ? Theme.accent : Theme.border
        }

    Item {
        id: body
        QQC.SplitView.fillHeight: true
        QQC.SplitView.minimumHeight: 120

        // One page per tab, all kept alive (background streaming/scroll), only
        // the active one shown. Keyed by the model so a reassigned preview tab
        // reuses its page (and reloads via TranscriptPage.onSessionIdChanged).
        Repeater {
            id: tabRepeater
            model: tabModel

            delegate: Loader {
                id: pageLoader

                required property int index
                required property int kind
                required property string sessionId
                required property int tabId
                required property string filePath
                required property string fileRoot
                required property string profile

                anchors.fill: parent
                visible: index === tabModel.currentIndex
                active: true
                // Transcript tabs load TranscriptPage; app-level page kinds mount
                // their manager/settings component directly as tab content (the GUI
                // equivalent of the TUI's per-kind page projection).
                sourceComponent: {
                    switch (pageLoader.kind) {
                    case TabModel.Transcript: return transcriptComp;
                    case TabModel.Settings:   return settingsComp;
                    case TabModel.Models:     return modelsComp;
                    case TabModel.Accounts:   return accountsComp;
                    case TabModel.Profiles:   return profilesComp;
                    case TabModel.Fleet:      return fleetComp;
                    case TabModel.Sessions:   return sessionsComp;
                    case TabModel.UsersAccess: return usersAccessComp;
                    case TabModel.Dashboard:  return dashboardComp;
                    case TabModel.Approvals:  return approvalsComp;
                    case TabModel.Routing:    return routingComp;
                    case TabModel.Cron:       return cronComp;
                    case TabModel.Channels:   return channelsComp;
                    case TabModel.Memory:     return memoryComp;
                    case TabModel.Profile:    return agentProfileComp;
                    case TabModel.File:       return fileComp;
                    default:                  return transcriptComp;
                    }
                }

                onLoaded: {
                    // File tabs host a FilePage: bind its (root, path) and mirror
                    // its dirty/title back to the shared TabModel.
                    if (pageLoader.kind === TabModel.File) {
                        item.rootId = Qt.binding(() => pageLoader.fileRoot);
                        item.path = Qt.binding(() => pageLoader.filePath);
                        item.isCurrent = Qt.binding(() => pageLoader.index === tabModel.currentIndex);
                        item.dirtyChanged.connect(function(d) {
                            tabModel.setDirtyById(pageLoader.tabId, d);
                        });
                        item.titleResolved.connect(function(t) {
                            tabModel.setTitle(tabModel.indexOfTabId(pageLoader.tabId), t);
                        });
                        return;
                    }
                    // Per-agent surfaces (Memory / Profile) bind their `profile`
                    // from the tab's profile so each agent's tab shows its own bank.
                    if (pageLoader.kind === TabModel.Memory
                        || pageLoader.kind === TabModel.Profile) {
                        item.profile = Qt.binding(() => pageLoader.profile);
                        return;
                    }
                    // Manager/settings pages are self-contained (they bind global
                    // context-property seams); only transcript tabs need the
                    // session wiring below.
                    if (pageLoader.kind !== TabModel.Transcript)
                        return;
                    // Bind reactively so a preview tab reassigned to another
                    // session reloads in place.
                    item.sessionId = Qt.binding(() => pageLoader.sessionId);
                    // Only the foreground tab feeds the shared footer status model.
                    item.isActive = Qt.binding(() => pageLoader.index === tabModel.currentIndex);
                    item.titleResolved.connect(function(t) {
                        tabModel.setTitle(tabModel.indexOfTabId(pageLoader.tabId), t);
                    });
                    item.openSettingsRequested.connect(root.openSettings);
                    item.commandForwarded.connect(root.paneCommandForwarded);
                    // A submit/edit "commits" to the session -> pin the tab.
                    item.committed.connect(function() {
                        tabModel.pinTabById(pageLoader.tabId);
                    });
                }
            }
        }

        // Empty state: no tabs open yet.
        Column {
            anchors.centerIn: parent
            visible: tabModel.count === 0
            spacing: Theme.spacing

            Kit.Glyph {
                anchors.horizontalCenter: parent.horizontalCenter
                glyph: FontIcons.fa_comments
                font.pointSize: 36 + Theme.pointSizeOffset
                color: Theme.border
            }

            QQC.Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Select a session")
                color: Theme.textMuted
                font.family: FontIcons.display
                font.pixelSize: 16
            }
        }
    }

        // The embedded shell, docked below the tab content. Kept in the tree
        // (not destroyed) when hidden so the running bash session survives a
        // toggle; SplitView drops it from layout while `visible` is false.
        TerminalPanel {
            id: terminalPanel
            visible: UiSettings.showTerminal && !UiSettings.distractionFree
            QQC.SplitView.preferredHeight: 240
            QQC.SplitView.minimumHeight: 120
            onCloseRequested: UiSettings.showTerminal = false
        }
    }

    // The editor settings popup (Style / Text / Theme / Options / Trash / Reset),
    // dropped from the top-right under the "..." button.
    SettingsMenu {
        id: settingsMenu
        objectName: "settingsMenu"
        x: root.width - width - 8
        y: tabBarBar.visible ? tabBarBar.height + 4 : 8
        maxHeight: Math.round(root.height * 0.85)
        // "Search transcript" routes through the active tab's command path, which
        // opens that transcript's find bar (same as Ctrl+F / /find).
        onSearchRequested: root.invokeActiveCommand("find")
    }

    // Transcript page instantiated per tab by the Repeater above.
    Component {
        id: transcriptComp
        TranscriptPage {}
    }

    // App-level manager/settings pages, mounted as tab content by the Repeater's
    // per-kind switch. Each is self-contained (binds global context-property seams).
    Component { id: settingsComp;  SettingsPage {} }
    Component { id: modelsComp;    ModelsPage {} }
    Component { id: accountsComp;  AccountsPage {} }
    Component { id: profilesComp;  ProfilesPage {} }
    Component { id: fleetComp;     FleetPage {} }
    Component { id: sessionsComp;  SessionsPage {} }
    Component { id: usersAccessComp; UsersAccessPage {} }
    Component { id: dashboardComp; DashboardPage {} }
    Component { id: approvalsComp; ApprovalsPage {} }
    Component { id: routingComp;   RoutingPage {} }
    Component { id: cronComp;      CronPage {} }
    Component { id: channelsComp;  ChannelsPage {} }
    Component { id: memoryComp;    MemoryPage {} }
    Component { id: agentProfileComp; AgentProfilePage {} }
    Component { id: fileComp;      FilePage {} }

    // --- Session-action dialogs (rename + export) ---------------------------
    // Rename the session via the store. openFor(id) seeds the field with the
    // current title and remembers the target id.
    Kit.Dialog {
        id: renameDialog
        property string targetId: ""
        title: qsTr("Rename session")
        width: 380
        acceptText: qsTr("Rename")

        function openFor(sessionId) {
            renameDialog.targetId = sessionId;
            renameField.text = SessionStore.title(sessionId);
            open();
            renameField.forceActiveFocus();
            renameField.selectAll();
        }

        onAccepted: {
            if (renameDialog.targetId !== "" && renameField.text.trim().length > 0)
                SessionStore.renameSession(renameDialog.targetId, renameField.text.trim());
        }

        contentItem: Kit.TextField {
            id: renameField
            underline: true
            placeholderText: qsTr("Session title")
            onAccepted: renameDialog.accept()
        }
    }

    // Export the session transcript to a JSON file via the shared Exporter.
    FileDialog {
        id: exportDialog
        property string targetId: ""
        title: qsTr("Export transcript")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("JSON files (*.json)"), qsTr("All files (*)")]
        defaultSuffix: "json"

        function openFor(sessionId) {
            exportDialog.targetId = sessionId;
            const t = SessionStore.title(sessionId);
            exportDialog.currentFile = "file:" + (t && t.length > 0 ? t : "session") + ".json";
            open();
        }

        onAccepted: {
            if (exportDialog.targetId !== "")
                Exporter.writeFile(selectedFile, Exporter.toJson(SessionStore, exportDialog.targetId));
        }
    }
}
