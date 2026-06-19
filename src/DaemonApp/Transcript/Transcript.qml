import QtQuick
import QtQuick.Controls
import DaemonApp.Theme

Rectangle {
    id: root

    color: Theme.background

    property string content: ""

    ScrollView {
        id: scroll
        anchors.fill: parent
        contentWidth: availableWidth

        TextEdit {
            width: scroll.availableWidth
            readOnly: true
            selectByMouse: true
            textFormat: TextEdit.MarkdownText
            wrapMode: TextEdit.Wrap
            text: root.content
            color: Theme.text
            padding: Theme.spacingLarge
        }
    }
}
