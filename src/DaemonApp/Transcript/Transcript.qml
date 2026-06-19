import QtQuick
import QtQuick.Controls as QQC
import DaemonApp.Theme

// Renders the conversation's markdown content. A read-only MarkdownText TextEdit
// is the placeholder renderer until the dedicated content renderer lands.
Rectangle {
    id: root

    color: Theme.background

    property string content: ""

    QQC.ScrollView {
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
            selectionColor: Theme.searchSelection
            selectedTextColor: Theme.text
            font.family: FontIcons.display
            font.pixelSize: 15
            padding: Theme.spacingLarge
        }
    }
}
