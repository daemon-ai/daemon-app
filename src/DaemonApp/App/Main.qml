import QtQuick
import QtQuick.Controls
import DaemonApp.Theme
import DaemonApp.Sidebar
import DaemonApp.ConversationsList
import DaemonApp.Conversation

ApplicationWindow {
    id: root

    width: 1100
    height: 720
    visible: true
    title: qsTr("daemon-app")
    color: Theme.background

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
            visible: !collapsed

            property bool collapsed: false

            onScopeSelected: function(nodeType, nodeId) {
                conversationsList.setScope(nodeType, nodeId);
            }
        }

        ConversationsList {
            id: conversationsList
            SplitView.preferredWidth: Theme.listWidth
            SplitView.minimumWidth: 220

            onToggleSidebarRequested: sidebar.collapsed = !sidebar.collapsed
            onConversationActivated: function(conversationId) {
                conversation.open(conversationId);
            }
            onNewConversationRequested: conversation.createNew()
        }

        Conversation {
            id: conversation
            SplitView.fillWidth: true
            SplitView.minimumWidth: 320
        }
    }
}
