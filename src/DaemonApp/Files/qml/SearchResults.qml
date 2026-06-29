// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Files

// Project-wide search: a query field driving a server-side search over the Fs
// seam (SearchResultsModel) and a results list grouped by file. Clicking a hit
// emits hitChosen so the host opens the file at the line.
Item {
    id: root

    property var service: (typeof Fs !== "undefined") ? Fs : null
    property string rootId: "workspace"
    signal hitChosen(string rootId, string path, int line, int column)
    signal dismissed()

    function focusInput() {
        field.forceActiveFocus();
        field.selectAll();
    }

    SearchResultsModel {
        id: results
        service: root.service
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.smallSpacing

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.smallSpacing
            Kit.TextField {
                id: field
                Layout.fillWidth: true
                placeholderText: qsTr("Search in workspace\u2026")
                Keys.onReturnPressed: results.search(root.rootId, text, {})
                Keys.onEnterPressed: results.search(root.rootId, text, {})
                Keys.onEscapePressed: root.dismissed()
            }
            BusyIndicator {
                running: results.searching
                visible: results.searching
                implicitWidth: 20
                implicitHeight: 20
            }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: results
            ScrollBar.vertical: ScrollBar {}

            delegate: Item {
                id: del
                required property string path
                required property string preview
                required property int line
                required property int column
                required property string rootId
                width: ListView.view.width
                implicitHeight: rowLayout.implicitHeight + Theme.smallSpacing

                Rectangle {
                    anchors.fill: parent
                    color: hover.hovered ? Theme.rowHover : "transparent"
                }

                HoverHandler { id: hover }
                TapHandler {
                    onTapped: root.hitChosen(del.rootId, del.path, del.line, del.column)
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
                        text: del.path + ":" + del.line
                        color: Theme.link
                        font.pixelSize: Theme.captionFontSize
                        elide: Text.ElideLeft
                        Layout.fillWidth: true
                    }
                    Label {
                        text: del.preview
                        color: Theme.text
                        font.family: FontIcons.mono
                        font.pixelSize: Theme.captionFontSize
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}
