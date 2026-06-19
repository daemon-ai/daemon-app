import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Transcript
import DaemonApp.Composer

Rectangle {
    id: root

    color: Theme.background

    function open(conversationId) {
        controller.open(conversationId);
    }

    function createNew() {
        controller.createConversation(-1);
    }

    ConversationController {
        id: controller
        store: ChatStore
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: controller.hasConversation

        Transcript {
            Layout.fillWidth: true
            Layout.fillHeight: true
            content: controller.content
        }

        Composer {
            Layout.fillWidth: true
            onSubmitted: function(text) { controller.appendUserText(text); }
        }
    }

    Label {
        anchors.centerIn: parent
        visible: !controller.hasConversation
        text: qsTr("Select a conversation")
        color: Theme.textMuted
        font.pixelSize: 16
    }
}
