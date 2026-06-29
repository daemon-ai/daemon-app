// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme

// The command palette (Mod+K): a centered, filterable overlay over the shared
// CommandRegistry (`Commands` context property). Typing narrows the list via
// Commands.search; Enter / click triggers the highlighted entry, which the
// registry emits as commandTriggered(id) for Main.qml to route to existing
// actions. A pure view over the shared catalog - the TUI Ctrl+P palette lists the
// same registry.
QQC.Popup {
    id: root

    // The shared CommandRegistry (the `Commands` context property).
    property var registry: Commands

    modal: true
    focus: true
    // Center in the window overlay.
    parent: QQC.Overlay.overlay
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round(parent.height * 0.18) : 0
    width: 480
    padding: 0

    onAboutToShow: {
        filter.text = "";
        if (root.registry)
            root.registry.search("");
        list.currentIndex = 0;
        filter.forceActiveFocus();
    }

    background: Rectangle {
        color: Theme.background
        border.color: Theme.border
        border.width: 1
        radius: Theme.radius
    }

    contentItem: ColumnLayout {
        spacing: 0

        QQC.TextField {
            id: filter
            Layout.fillWidth: true
            Layout.margins: 8
            placeholderText: qsTr("Type a command\u2026")
            font.family: FontIcons.display
            font.pixelSize: 14
            color: Theme.text
            selectByMouse: true
            // No themed edit menu on a transient filter; just suppress Qt's default.
            QQC.ContextMenu.menu: null
            background: Rectangle {
                color: Theme.surfaceRaised
                border.color: filter.activeFocus ? Theme.accent : Theme.border
                border.width: 1
                radius: Theme.radius
            }
            onTextChanged: if (root.registry) root.registry.search(text)
            Keys.onDownPressed: list.currentIndex = Math.min(list.currentIndex + 1, list.count - 1)
            Keys.onUpPressed: list.currentIndex = Math.max(list.currentIndex - 1, 0)
            Keys.onReturnPressed: list.activateCurrent()
            Keys.onEnterPressed: list.activateCurrent()
            Keys.onEscapePressed: root.close()
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 320)
            clip: true
            model: root.registry
            currentIndex: 0

            function activateCurrent() {
                if (currentIndex >= 0 && root.registry)
                    root.registry.trigger(currentIndex);
                root.close();
            }

            delegate: Item {
                required property int index
                required property string title
                required property string group
                required property string hint
                required property string shortcut
                width: ListView.view.width
                implicitHeight: 38

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 2
                    radius: 5
                    color: (list.currentIndex === index || rowArea.containsMouse)
                        ? Theme.hover : "transparent"
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        QQC.Label {
                            text: title
                            font.family: FontIcons.display
                            font.pixelSize: 13
                            color: Theme.text
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        QQC.Label {
                            visible: hint.length > 0
                            text: hint
                            font.family: FontIcons.display
                            font.pixelSize: 10
                            color: Theme.textMuted
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    QQC.Label {
                        text: group
                        font.family: FontIcons.display
                        font.pixelSize: 9
                        font.capitalization: Font.AllUppercase
                        color: Theme.textMuted
                    }

                    QQC.Label {
                        visible: shortcut.length > 0
                        text: shortcut
                        font.family: FontIcons.mono
                        font.pixelSize: 10
                        color: Theme.textMuted
                    }
                }

                MouseArea {
                    id: rowArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: list.currentIndex = index
                    onClicked: {
                        if (root.registry)
                            root.registry.trigger(index);
                        root.close();
                    }
                }
            }
        }
    }
}
