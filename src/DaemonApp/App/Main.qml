import QtQuick
import QtQuick.Controls
import DaemonApp.Theme
import DaemonApp.Settings
import DaemonApp.Sidebar
import DaemonApp.ConversationsList
import DaemonApp.Conversation
import DaemonApp.StatusBar

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
                    conversationExpanded.open(conversationId);
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
                    if (root.activeConversationId >= 0)
                        open(root.activeConversationId);
                }
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
        }
    }
}
