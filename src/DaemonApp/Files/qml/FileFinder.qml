import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Files

// Ctrl+P fuzzy file finder: a query field over FileFinderModel (which indexes
// the node through the Fs seam and ranks with an end-anchored fuzzy match) and
// a results list. Enter / click emits fileChosen.
Item {
    id: root

    property var service: (typeof Fs !== "undefined") ? Fs : null
    signal fileChosen(string rootId, string path)
    signal dismissed()

    function focusInput() {
        field.forceActiveFocus();
        field.selectAll();
    }

    FileFinderModel {
        id: finder
        service: root.service
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.smallSpacing

        Kit.TextField {
            id: field
            Layout.fillWidth: true
            placeholderText: finder.indexing
                ? qsTr("Indexing\u2026 (%n file(s))", "", finder.fileCount)
                : qsTr("Go to file\u2026")
            text: finder.query
            onTextChanged: finder.query = text
            Keys.onDownPressed: { list.incrementCurrentIndex(); }
            Keys.onUpPressed: { list.decrementCurrentIndex(); }
            Keys.onReturnPressed: list.activateCurrent()
            Keys.onEnterPressed: list.activateCurrent()
            Keys.onEscapePressed: root.dismissed()
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: finder
            currentIndex: 0
            ScrollBar.vertical: ScrollBar {}

            function activateCurrent() {
                if (currentIndex < 0 || currentIndex >= count)
                    return;
                const rootId = model.data(model.index(currentIndex, 0), FileFinderModel.RootIdRole);
                const path = model.data(model.index(currentIndex, 0), FileFinderModel.PathRole);
                finder.noteRecent(rootId, path);
                root.fileChosen(rootId, path);
            }

            delegate: Item {
                id: del
                required property int index
                required property string path
                required property string name
                required property string rootId
                width: ListView.view.width
                implicitHeight: rowLayout.implicitHeight + Theme.smallSpacing

                Rectangle {
                    anchors.fill: parent
                    color: del.ListView.isCurrentItem ? Theme.rowActive
                         : hover.hovered ? Theme.rowHover
                         : "transparent"
                }

                HoverHandler { id: hover }
                TapHandler {
                    onTapped: {
                        list.currentIndex = del.index;
                        list.activateCurrent();
                    }
                }

                ColumnLayout {
                    id: rowLayout
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: Theme.smallSpacing
                    anchors.rightMargin: Theme.smallSpacing
                    spacing: 0
                    Label {
                        text: del.name
                        color: Theme.text
                        font.pixelSize: Theme.bodyFontSize
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Label {
                        text: del.path
                        color: Theme.mutedText
                        font.pixelSize: Theme.captionFontSize
                        elide: Text.ElideLeft
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}
