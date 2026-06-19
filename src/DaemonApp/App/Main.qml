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

    // Restore the persisted theme on launch and write back every change so the
    // UiSettings store stays the source of truth across runs.
    Component.onCompleted: Theme.setTheme(UiSettings.theme)
    Connections {
        target: Theme
        function onThemeChanged() { UiSettings.theme = Theme.theme; }
    }

    // Entering distraction-free maximizes the window for the most canvas; exiting
    // restores the previous (normal) size.
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
    // chrome in distraction-free mode. The SplitView (anchors.fill) auto-shrinks
    // above it.
    footer: StatusBar {
        visible: !UiSettings.distractionFree
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        handle: Rectangle {
            implicitWidth: 1
            color: SplitHandle.pressed || SplitHandle.hovered ? Theme.accent : Theme.splitter
        }

        Sidebar {
            id: sidebar
            SplitView.preferredWidth: Theme.sidebarWidth
            SplitView.minimumWidth: 160
            // "Show folders tree" option + the middle-column collapse toggle, and
            // hidden entirely in distraction-free mode.
            visible: !collapsed && UiSettings.showFoldersTree && !UiSettings.distractionFree

            property bool collapsed: false

            onScopeSelected: function(nodeType, nodeId) {
                conversationsList.setScope(nodeType, nodeId);
            }
        }

        ConversationsList {
            id: conversationsList
            SplitView.preferredWidth: Theme.listWidth
            SplitView.minimumWidth: 220
            // "Show notes list" option; hidden in distraction-free mode.
            visible: UiSettings.showNotesList && !UiSettings.distractionFree

            onToggleSidebarRequested: sidebar.collapsed = !sidebar.collapsed
            onConversationActivated: function(conversationId) {
                conversation.open(conversationId);
            }
        }

        Conversation {
            id: conversation
            SplitView.fillWidth: true
            SplitView.minimumWidth: 320
        }
    }
}
