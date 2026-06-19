import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DaemonApp.Theme

Rectangle {
    id: root

    color: Theme.surface
    implicitHeight: layout.implicitHeight + 2 * Theme.spacing

    signal submitted(string text)

    function send() {
        const trimmed = input.text.trim();
        if (trimmed.length > 0) {
            root.submitted(trimmed);
            input.clear();
        }
    }

    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: Theme.border
    }

    RowLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: Theme.spacing
        spacing: Theme.spacingSmall

        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(input.implicitHeight, 120)

            TextArea {
                id: input
                placeholderText: qsTr("Send a message...  (Enter to send, Shift+Enter for newline)")
                wrapMode: TextArea.Wrap
                color: Theme.text

                Keys.onReturnPressed: function(event) {
                    if (event.modifiers & Qt.ShiftModifier) {
                        event.accepted = false;
                    } else {
                        event.accepted = true;
                        root.send();
                    }
                }
            }
        }

        Button {
            text: qsTr("Send")
            enabled: input.text.trim().length > 0
            onClicked: root.send()
        }
    }
}
