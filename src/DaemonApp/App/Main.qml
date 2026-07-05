// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Settings
import DaemonApp.Sidebar
import DaemonApp.SessionsList
import DaemonApp.Session
import DaemonApp.StatusBar
import DaemonApp.Pages
import DaemonApp.Files

ApplicationWindow {
    id: root

    width: 1100
    height: 720
    visible: true
    title: qsTr("Daemon")
    color: Theme.background

    // Right-to-left locales (e.g. Arabic) mirror the whole shell. Bound to the
    // app layout direction, which Application updates on a live language switch.
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    // --- Adaptive shell state -----------------------------------------------
    // The active structure (SplitView or StackView) is rebuilt by the shell
    // Loader when the window size class changes. These hold the current
    // selection so the rebuilt structure can restore the same scope and open
    // session instead of resetting to the empty state.
    property string activeSessionId: ""
    property bool hasScope: false
    property int scopeNodeType: 0
    property int scopeId: -1
    property string scopeNodeId: ""

    function rememberScope(nodeType, id, nodeId) {
        root.hasScope = true;
        root.scopeNodeType = nodeType;
        root.scopeId = id;
        root.scopeNodeId = nodeId;
    }

    // The active Session pane (set by whichever shell is mounted), so the
    // command palette can route session-scoped commands (slash actions, model
    // picker, modes) to the foreground tab.
    property var activeSessionPane: null

    // Cycle the four built-in themes, mirroring the TUI's F8 (kept in one place so
    // the palette "Cycle theme" entry and a future menu agree).
    function cycleTheme() {
        const order = ["light", "dark", "sepia", "midnight"];
        const i = order.indexOf(Theme.theme);
        Theme.setTheme(order[(i + 1) % order.length]);
    }

    // Toggle the file Explorer side panel. UiSettings.showFileExplorer is the
    // single source of truth: the expanded shell binds a pane's visibility to it
    // and the compact shell opens/closes a right-edge drawer from it.
    function toggleExplorer() {
        UiSettings.showFileExplorer = !UiSettings.showFileExplorer;
    }
    // Open a file (from the Explorer) as an editor tab in the active pane.
    function openFileInPane(rootId, path, pinned) {
        if (root.activeSessionPane)
            root.activeSessionPane.openFile(rootId, path, pinned);
    }

    // Route a command-palette id to an existing action. Window-level ids are
    // handled here; session-scoped ids fall through to the active pane's
    // orchestrator (which raises front-end overlays via commandRequested). The
    // window-level set is handled directly (never delegated) so a pane that
    // forwards one back up cannot loop.
    function routeCommand(commandId) {
        switch (commandId) {
        case "theme": root.cycleTheme(); break;
        case "distraction": UiSettings.distractionFree = true; break;
        case "new": if (root.activeSessionPane) root.activeSessionPane.createNew(); break;
        // App-level pages open as full-window overlays via the shared Nav seam.
        // (The transcript-scoped appearance popup stays on the tab-bar "..." only.)
        case "settings": Nav.open("settings"); break;
        case "models": Nav.open("models"); break;
        case "accounts": Nav.open("accounts"); break;
        case "profiles": Nav.open("profiles"); break;
        case "fleet": Nav.open("fleet"); break;
        case "sessions": Nav.open("sessions"); break;
        case "dashboard": Nav.open("dashboard"); break;
        case "approvals": Nav.open("approvals"); break;
        case "routing": Nav.open("routing"); break;
        case "cron": Nav.open("cron"); break;
        case "channels": Nav.open("channels"); break;
        // Admin-only (auth6): only route when the principal holds access_admin (the page host
        // also hard-gates the mount, and the node enforces server-side).
        case "access":
            if (typeof Principal !== "undefined" && Principal && Principal.hasCapability("access_admin"))
                Nav.open("access");
            break;
        case "files": root.toggleExplorer(); break;
        case "help": commandPalette.open(); break;
        case "title": if (root.activeSessionPane) root.activeSessionPane.renameActive(); break;
        case "save": if (root.activeSessionPane) root.activeSessionPane.exportActive(); break;
        default:
            if (root.activeSessionPane)
                root.activeSessionPane.invokeActiveCommand(commandId);
        }
    }

    // Mod+K opens the palette (Cmd on macOS, Ctrl elsewhere - StandardKey covers both).
    Shortcut {
        sequences: [StandardKey.QuickOpen, "Ctrl+K", "Meta+K"]
        onActivated: commandPalette.open()
    }

    // Toggle the file Explorer side panel (VS Code-style).
    Shortcut {
        sequences: ["Ctrl+E", "Meta+E"]
        onActivated: root.toggleExplorer()
    }

    Connections {
        target: Commands
        function onCommandTriggered(commandId) {
            commandPalette.close();
            root.routeCommand(commandId);
        }
    }

    // A session pane forwarded a window-level command (e.g. typed "/help").
    Connections {
        target: root.activeSessionPane
        ignoreUnknownSignals: true
        function onPaneCommandForwarded(command) { root.routeCommand(command); }
    }

    // Open a fresh chat when the app asks: first-run completion (P0-B) and "+ New agent" (P0-C)
    // both funnel through App.openChatRequested so a newly-connected user lands in a working
    // transcript. Node-authority: createNew() issues a node SessionCreate (nothing client-minted);
    // the pinned tab opens event-driven when the SessionCreated reply lands. `profileId` is advisory
    // (logged app-side); the node binds its active default.
    Connections {
        target: App
        function onOpenChatRequested(profileId) {
            if (root.activeSessionPane)
                root.activeSessionPane.createNew();
        }
    }

    CommandPalette {
        id: commandPalette
    }

    // App-level manager/settings pages open as singleton TABS in the active
    // session pane's tab strip (not a modal overlay). The shared Nav
    // controller is repurposed as the "open page" signal bus: every Nav.open()
    // call site (command palette, slash commands, sidebar cog, in-page links)
    // routes here and we forward it to the active pane's openManagerPage().
    Connections {
        target: Nav
        function onOpenRequested(page, section) {
            if (root.activeSessionPane)
                root.activeSessionPane.openManagerPage(page, section);
        }
        // Per-agent (ProfileRef-keyed) Memory / Profile tabs.
        function onOpenAgentRequested(kind, profileRef, title) {
            if (root.activeSessionPane)
                root.activeSessionPane.openAgentTab(kind, profileRef, title);
        }
    }

    // First-run / onboarding gate: full-screen over everything until setup
    // completes (FirstRun.active). On subsequent launches setupComplete is true,
    // so this never mounts and the shell shows immediately.
    Loader {
        id: firstRunGate
        anchors.fill: parent
        z: 200
        active: FirstRun ? FirstRun.active : false
        sourceComponent: active ? firstRunComp : null
        onLoaded: if (item) item.forceActiveFocus()
    }
    Component {
        id: firstRunComp
        FirstRunGate {}
    }

    // Drive the size-class layer from live window geometry; foldable fold/unfold
    // and rotation change the width and re-evaluate sizeClass for free.
    onWidthChanged: LayoutState.windowWidth = root.width

    // Restore the persisted theme on launch and write back every change so the
    // UiSettings store stays the source of truth across runs. Also seed the
    // size-class layer and the touch/platform profile from the runtime platform.
    Component.onCompleted: {
        Theme.setTheme(UiSettings.theme);
        LayoutState.windowWidth = root.width;
        const os = Qt.platform.os;
        if (os === "osx" || os === "ios")
            Theme.platform = "Apple";
        Theme.touch = (os === "android" || os === "ios");
    }
    Connections {
        target: Theme
        function onThemeChanged() { UiSettings.theme = Theme.theme; }
    }

    // Entering distraction-free maximizes the window for the most canvas; exiting
    // restores the previous (normal) size. No-op on mobile window managers.
    Connections {
        target: UiSettings
        function onDistractionFreeChanged() {
            if (UiSettings.distractionFree)
                root.showMaximized();
            else
                root.showNormal();
        }
    }

    // Full-width chrome strip below the columns; hidden with the rest of the
    // chrome in distraction-free mode. The shell (anchors.fill) auto-shrinks
    // above it. StatusBar collapses to a slim strip in the compact size class.
    footer: StatusBar {
        visible: !UiSettings.distractionFree
    }

    // The adaptive shell: the same Sidebar / SessionsList / Session
    // components, arranged by size class. Rebuilt on size-class transitions; the
    // root selection state above lets each structure restore on completion.
    Loader {
        id: shell
        anchors.fill: parent
        sourceComponent: LayoutState.isCompact ? compactShell : expandedShell
    }

    // Global reconnect banner (W5): a thin strip pinned to the top of the shell whenever a live
    // connection is recovering from a drop (reconnecting / re-authenticating) or gave up. Bound to
    // Connection.state; collapses to nothing when ready so it stays out of the way. The first-run
    // gate (z: 200) covers it during onboarding, so it only surfaces post-setup. The recovery
    // behavior itself lives in the C++ connection service (backoff reprobe + browser online-event
    // wake-up); this is presentation only.
    Rectangle {
        id: reconnectBanner
        z: 150
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: visible ? 34 : 0
        visible: reconnectBanner.showBanner
        color: Theme.surfaceRaised
        clip: true

        // Show while a previously-live connection is recovering or has given up: a reconnect
        // episode (state "connecting" with a status message - the initial connect clears it), a
        // drop that needs re-auth, or an offline state that carries a failure reason. Ready /
        // first connect / user-initiated offline stay silent.
        readonly property bool showBanner: {
            if (!Connection)
                return false;
            const s = Connection.state;
            if (s === "connecting")
                return Connection.statusMessage.length > 0;
            if (s === "authenticating")
                return true;
            if (s === "offline")
                return Connection.statusMessage.length > 0;
            return false;
        }
        readonly property bool failed: Connection && Connection.state === "offline"

        // Seconds since the banner appeared, reset on each show, so the user sees the outage isn't
        // stalled silently.
        property int elapsedSec: 0
        onVisibleChanged: elapsedSec = 0
        Timer {
            running: reconnectBanner.visible
            interval: 1000
            repeat: true
            onTriggered: reconnectBanner.elapsedSec += 1
        }

        // Bottom hairline so the strip reads as chrome over the shell.
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Theme.border
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.spacing
            anchors.rightMargin: Theme.spacing
            spacing: Theme.spacing

            Rectangle {
                implicitWidth: 8
                implicitHeight: 8
                radius: 4
                Layout.alignment: Qt.AlignVCenter
                color: reconnectBanner.failed ? Theme.danger : Theme.warning
            }
            Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                color: Theme.text
                font.pixelSize: 13
                text: {
                    const base = (Connection && Connection.statusMessage.length)
                        ? Connection.statusMessage
                        : qsTr("Connection lost");
                    return reconnectBanner.elapsedSec > 0
                        ? qsTr("%1 (%2s)").arg(base).arg(reconnectBanner.elapsedSec)
                        : base;
                }
            }
            Kit.TextButton {
                text: qsTr("Reconnect now")
                onClicked: if (Connection)
                    Connection.connectTo(Connection.mode, Connection.target)
            }
        }
    }

    // Update banner (U1 Notify / U2 DownloadAndOpen): a thin strip below the
    // reconnect banner offering an available update, its download progress, and
    // the release-notes / download / open / dismiss actions per capability. Bound
    // to the Update context property (the shared C++ UpdateManager); collapses to
    // nothing on an inert build or when nothing is offered / it was dismissed.
    Rectangle {
        id: updateBanner
        z: 140
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: reconnectBanner.bottom
        height: visible ? 34 : 0
        visible: !!Update && !Update.dismissed
                 && (Update.stateName === "UpdateAvailable"
                     || Update.stateName === "Downloading"
                     || Update.stateName === "ReadyToApply")
        color: Theme.surfaceRaised
        clip: true

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Theme.border
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.spacing
            anchors.rightMargin: Theme.spacing
            spacing: Theme.spacing

            Rectangle {
                implicitWidth: 8
                implicitHeight: 8
                radius: 4
                Layout.alignment: Qt.AlignVCenter
                color: Theme.accent
            }
            Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                color: Theme.text
                font.pixelSize: 13
                text: {
                    if (!Update)
                        return "";
                    if (Update.stateName === "Downloading")
                        return qsTr("Downloading update v%1… %2%")
                            .arg(Update.latestVersion)
                            .arg(Math.round(Update.downloadProgress * 100));
                    if (Update.stateName === "ReadyToApply")
                        return qsTr("Update v%1 is ready to install").arg(Update.latestVersion);
                    return qsTr("Update available: v%1 → v%2")
                        .arg(Update.currentVersion)
                        .arg(Update.latestVersion);
                }
            }
            Kit.TextButton {
                text: qsTr("Release notes")
                visible: !!Update && Update.notesUrl.length > 0
                onClicked: Qt.openUrlExternally(Update.notesUrl)
            }
            Kit.TextButton {
                text: qsTr("Download")
                visible: !!Update && Update.canDownload
                onClicked: Update.download()
            }
            Kit.TextButton {
                // Verb comes from the shared view-model: "Install & restart"
                // for a self-applying build, "Open" for the DownloadAndOpen
                // hand-off. Keeps GUI/TUI copy in one place (UpdateManager).
                text: !!Update ? Update.applyActionLabel : qsTr("Open")
                visible: !!Update && Update.stateName === "ReadyToApply"
                onClicked: Update.apply()
            }
            Kit.TextButton {
                text: qsTr("Dismiss")
                visible: !!Update && Update.stateName === "UpdateAvailable"
                onClicked: Update.dismiss()
            }
        }
    }

    // --- Expanded (>= 900dp): the original three-pane desktop SplitView -------
    Component {
        id: expandedShell

        SplitView {
            orientation: Qt.Horizontal

            handle: Rectangle {
                implicitWidth: 1
                color: SplitHandle.pressed || SplitHandle.hovered ? Theme.accent : Theme.splitter
            }

            Sidebar {
                id: sidebarExpanded
                SplitView.preferredWidth: Theme.sidebarWidth
                SplitView.minimumWidth: 160
                // "Show Fleet Tree" option + the middle-column collapse toggle,
                // and hidden entirely in distraction-free mode.
                visible: !collapsed && UiSettings.showFleetTree && !UiSettings.distractionFree

                property bool collapsed: false

                onScopeSelected: function(nodeType, id, nodeId) {
                    root.rememberScope(nodeType, id, nodeId);
                    listExpanded.setScope(nodeType, id, nodeId);
                }
                // An Integrations-tree session leaf opens its transcript directly.
                onSessionActivated: function(sessionId) {
                    root.activeSessionId = sessionId;
                    sessionExpanded.open(sessionId);
                }
                // "+ New agent/node" opens a fresh chat via the shared open-a-chat path.
                onNewChatRequested: App.openNewAgentChat()
            }

            SessionsList {
                id: listExpanded
                SplitView.preferredWidth: Theme.listWidth
                SplitView.minimumWidth: 220
                // "Show Sessions" option; hidden in distraction-free mode.
                visible: UiSettings.showSessionsList && !UiSettings.distractionFree

                onToggleSidebarRequested: sidebarExpanded.collapsed = !sidebarExpanded.collapsed
                onSessionActivated: function(sessionId) {
                    root.activeSessionId = sessionId;
                    sessionExpanded.open(sessionId); // preview
                }
                onSessionOpened: function(sessionId) {
                    root.activeSessionId = sessionId;
                    sessionExpanded.openSessionPinned(sessionId);
                }
                Component.onCompleted: {
                    if (root.hasScope)
                        setScope(root.scopeNodeType, root.scopeId, root.scopeNodeId);
                }
            }

            Session {
                id: sessionExpanded
                SplitView.fillWidth: true
                SplitView.minimumWidth: 320
                Component.onCompleted: {
                    root.activeSessionPane = sessionExpanded;
                    if (root.activeSessionId !== "")
                        open(root.activeSessionId);
                }
            }

            // Right-side panel: the Participants section above the file Explorer
            // (opposite the sessions sidebar, like Hermes Desktop). Files open as
            // editor tabs in the session pane.
            RightPanel {
                id: explorerExpanded
                SplitView.preferredWidth: Theme.listWidth
                SplitView.minimumWidth: 200
                visible: UiSettings.showFileExplorer && !UiSettings.distractionFree
                onFileActivated: (rootId, path) => root.openFileInPane(rootId, path, false)
                onFileOpened: (rootId, path) => root.openFileInPane(rootId, path, true)
                onCollapseRequested: UiSettings.showFileExplorer = false
            }
        }
    }

    // --- Compact (touch, narrow): single pane, list -> session stack + drawer --
    Component {
        id: compactShell

        Item {
            StackView {
                id: compactStack
                anchors.fill: parent
                initialItem: listPage
            }

            Component {
                id: listPage

                SessionsList {
                    // Always the home page on a phone, regardless of the desktop
                    // "Show Sessions" option (which only hides a side column).
                    onToggleSidebarRequested: sidebarDrawerCompact.open()
                    onSessionActivated: function(sessionId) {
                        root.activeSessionId = sessionId;
                        compactStack.push(sessionPage);
                    }
                    Component.onCompleted: {
                        if (root.hasScope)
                            setScope(root.scopeNodeType, root.scopeId, root.scopeNodeId);
                    }
                }
            }

            Component {
                id: sessionPage

                Session {
                    Component.onCompleted: {
                        root.activeSessionPane = this;
                        if (root.activeSessionId !== "")
                            open(root.activeSessionId);
                    }
                    onBackRequested: compactStack.pop()
                }
            }

            Drawer {
                id: sidebarDrawerCompact
                edge: Qt.LeftEdge
                width: Math.min(320, root.width * 0.88)
                height: root.height

                Sidebar {
                    anchors.fill: parent
                    onScopeSelected: function(nodeType, id, nodeId) {
                        root.rememberScope(nodeType, id, nodeId);
                        const listItem = compactStack.get(0);
                        if (listItem && listItem.setScope)
                            listItem.setScope(nodeType, id, nodeId);
                        sidebarDrawerCompact.close();
                        // Surface the freshly scoped list.
                        compactStack.pop(null);
                    }
                    onSessionActivated: function(sessionId) {
                        root.activeSessionId = sessionId;
                        sidebarDrawerCompact.close();
                        compactStack.push(sessionPage);
                    }
                    // "+ New agent/node" opens a fresh chat via the shared open-a-chat path.
                    onNewChatRequested: {
                        sidebarDrawerCompact.close();
                        if (!root.activeSessionPane)
                            compactStack.push(sessionPage);
                        App.openNewAgentChat();
                    }
                }
            }

            // Right-edge file Explorer drawer, driven by UiSettings.showFileExplorer
            // (the same flag the expanded shell binds a pane to). Opening a file
            // pushes the session page so the editor tab is visible.
            Drawer {
                id: explorerDrawerCompact
                edge: Qt.RightEdge
                width: Math.min(360, root.width * 0.9)
                height: root.height
                onClosed: UiSettings.showFileExplorer = false

                RightPanel {
                    anchors.fill: parent
                    onFileActivated: (rootId, path) => {
                        if (!root.activeSessionPane)
                            compactStack.push(sessionPage);
                        Qt.callLater(() => root.openFileInPane(rootId, path, false));
                        explorerDrawerCompact.close();
                    }
                    onFileOpened: (rootId, path) => {
                        if (!root.activeSessionPane)
                            compactStack.push(sessionPage);
                        Qt.callLater(() => root.openFileInPane(rootId, path, true));
                        explorerDrawerCompact.close();
                    }
                    onCollapseRequested: explorerDrawerCompact.close()
                }
            }

            Connections {
                target: UiSettings
                function onShowFileExplorerChanged() {
                    if (UiSettings.showFileExplorer)
                        explorerDrawerCompact.open();
                    else
                        explorerDrawerCompact.close();
                }
            }
        }
    }
}
