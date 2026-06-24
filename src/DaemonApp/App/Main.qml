import QtQuick
import QtQuick.Controls
import DaemonApp.Theme
import DaemonApp.Settings
import DaemonApp.Sidebar
import DaemonApp.ConversationsList
import DaemonApp.Conversation
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

    // --- Adaptive shell state -----------------------------------------------
    // The active structure (SplitView or StackView) is rebuilt by the shell
    // Loader when the window size class changes. These hold the current
    // selection so the rebuilt structure can restore the same scope and open
    // conversation instead of resetting to the empty state.
    property int activeConversationId: -1
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

    // The active Conversation pane (set by whichever shell is mounted), so the
    // command palette can route conversation-scoped commands (slash actions, model
    // picker, modes) to the foreground tab.
    property var activeConversation: null

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
        if (root.activeConversation)
            root.activeConversation.openFile(rootId, path, pinned);
    }

    // Route a command-palette id to an existing action. Window-level ids are
    // handled here; conversation-scoped ids fall through to the active pane's
    // orchestrator (which raises front-end overlays via commandRequested). The
    // window-level set is handled directly (never delegated) so a pane that
    // forwards one back up cannot loop.
    function routeCommand(commandId) {
        switch (commandId) {
        case "theme": root.cycleTheme(); break;
        case "distraction": UiSettings.distractionFree = true; break;
        case "new": if (root.activeConversation) root.activeConversation.createNew(); break;
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
        case "files": root.toggleExplorer(); break;
        case "help": commandPalette.open(); break;
        case "title": if (root.activeConversation) root.activeConversation.renameActive(); break;
        case "save": if (root.activeConversation) root.activeConversation.exportActive(); break;
        default:
            if (root.activeConversation)
                root.activeConversation.invokeActiveCommand(commandId);
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

    // A conversation pane forwarded a window-level command (e.g. typed "/help").
    Connections {
        target: root.activeConversation
        ignoreUnknownSignals: true
        function onPaneCommandForwarded(command) { root.routeCommand(command); }
    }

    CommandPalette {
        id: commandPalette
    }

    // App-level manager/settings pages open as singleton TABS in the active
    // conversation pane's tab strip (not a modal overlay). The shared Nav
    // controller is repurposed as the "open page" signal bus: every Nav.open()
    // call site (command palette, slash commands, sidebar cog, in-page links)
    // routes here and we forward it to the active pane's openManagerPage().
    Connections {
        target: Nav
        function onOpenRequested(page, section) {
            if (root.activeConversation)
                root.activeConversation.openManagerPage(page, section);
        }
        // Per-agent (ProfileRef-keyed) Memory / Profile tabs.
        function onOpenAgentRequested(kind, profileRef, title) {
            if (root.activeConversation)
                root.activeConversation.openAgentTab(kind, profileRef, title);
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

    // The adaptive shell: the same Sidebar / ConversationsList / Conversation
    // components, arranged by size class. Rebuilt on size-class transitions; the
    // root selection state above lets each structure restore on completion.
    Loader {
        id: shell
        anchors.fill: parent
        sourceComponent: LayoutState.isCompact ? compactShell : expandedShell
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
                // "Show folders tree" option + the middle-column collapse toggle,
                // and hidden entirely in distraction-free mode.
                visible: !collapsed && UiSettings.showFoldersTree && !UiSettings.distractionFree

                property bool collapsed: false

                onScopeSelected: function(nodeType, id, nodeId) {
                    root.rememberScope(nodeType, id, nodeId);
                    listExpanded.setScope(nodeType, id, nodeId);
                }
            }

            ConversationsList {
                id: listExpanded
                SplitView.preferredWidth: Theme.listWidth
                SplitView.minimumWidth: 220
                // "Show notes list" option; hidden in distraction-free mode.
                visible: UiSettings.showNotesList && !UiSettings.distractionFree

                onToggleSidebarRequested: sidebarExpanded.collapsed = !sidebarExpanded.collapsed
                onConversationActivated: function(conversationId) {
                    root.activeConversationId = conversationId;
                    conversationExpanded.open(conversationId); // preview
                }
                onConversationOpened: function(conversationId) {
                    root.activeConversationId = conversationId;
                    conversationExpanded.openConversationPinned(conversationId);
                }
                Component.onCompleted: {
                    if (root.hasScope)
                        setScope(root.scopeNodeType, root.scopeId, root.scopeNodeId);
                }
            }

            Conversation {
                id: conversationExpanded
                SplitView.fillWidth: true
                SplitView.minimumWidth: 320
                Component.onCompleted: {
                    root.activeConversation = conversationExpanded;
                    if (root.activeConversationId >= 0)
                        open(root.activeConversationId);
                }
            }

            // Right-side file Explorer (opposite the conversations sidebar, like
            // Hermes Desktop). Files open as editor tabs in the conversation pane.
            FileExplorer {
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

    // --- Compact (touch, narrow): single pane, list -> conversation stack + drawer --
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

                ConversationsList {
                    // Always the home page on a phone, regardless of the desktop
                    // "show notes list" option (which only hides a side column).
                    onToggleSidebarRequested: sidebarDrawerCompact.open()
                    onConversationActivated: function(conversationId) {
                        root.activeConversationId = conversationId;
                        compactStack.push(conversationPage);
                    }
                    Component.onCompleted: {
                        if (root.hasScope)
                            setScope(root.scopeNodeType, root.scopeId, root.scopeNodeId);
                    }
                }
            }

            Component {
                id: conversationPage

                Conversation {
                    Component.onCompleted: {
                        root.activeConversation = this;
                        if (root.activeConversationId >= 0)
                            open(root.activeConversationId);
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
                }
            }

            // Right-edge file Explorer drawer, driven by UiSettings.showFileExplorer
            // (the same flag the expanded shell binds a pane to). Opening a file
            // pushes the conversation page so the editor tab is visible.
            Drawer {
                id: explorerDrawerCompact
                edge: Qt.RightEdge
                width: Math.min(360, root.width * 0.9)
                height: root.height
                onClosed: UiSettings.showFileExplorer = false

                FileExplorer {
                    anchors.fill: parent
                    onFileActivated: (rootId, path) => {
                        if (!root.activeConversation)
                            compactStack.push(conversationPage);
                        Qt.callLater(() => root.openFileInPane(rootId, path, false));
                        explorerDrawerCompact.close();
                    }
                    onFileOpened: (rootId, path) => {
                        if (!root.activeConversation)
                            compactStack.push(conversationPage);
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
