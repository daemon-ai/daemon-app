// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Checkpoints / rewind timeline popover (opens upward from the composer): the
// session's checkpoints with a Restore action. Backed by the Checkpoints facade
// (E4/TOOL-9: repo-backed in daemon mode); restoring confirms, then rewinds to
// that point. Foreign (ACP) sessions hide Restore — rewind is managed by the
// foreign agent (C4 honesty).
QQC.Popup {
    id: root

    // The bound session runs on a foreign ACP engine: rewind is not ours to offer.
    property bool foreignSession: false

    implicitWidth: 340
    padding: 12
    // Open upward, right-aligned to the trigger button so the panel grows leftward
    // into the window (the buttons sit at the window's right edge).
    y: -implicitHeight - 8
    x: parent ? parent.width - width : 0

    background: Rectangle {
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.border
    }

    // Rewind confirm (destructive: drops everything after the selected point).
    RewindConfirmDialog { id: confirmDialog }

    contentItem: ColumnLayout {
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: qsTr("Checkpoints")
                font.family: FontIcons.display; font.pixelSize: 14; font.bold: true
                color: Theme.text; Layout.fillWidth: true
            }
            Kit.TextButton {
                // Hidden when the seam cannot create (daemon mode: checkpoints are node-created
                // on tool events; no wire op).
                visible: Checkpoints.canCreate
                text: qsTr("Save here")
                onClicked: Checkpoints.createCheckpoint(qsTr("Manual checkpoint"))
            }
        }

        // Foreign-session honesty: say WHY rewind is absent instead of silently omitting it.
        Text {
            visible: root.foreignSession
            Layout.fillWidth: true
            text: qsTr("Rewind is managed by the foreign agent")
            font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
            wrapMode: Text.WordWrap
        }

        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 260)
            clip: true
            model: Checkpoints.checkpoints
            spacing: 6
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                required property var entry
                width: ListView.view.width
                height: 48
                radius: 6
                color: entry.current ? Theme.hover : "transparent"
                border.color: entry.current ? Theme.accent : Theme.border

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 8
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Text {
                            text: entry.label
                            font.family: FontIcons.display; font.pixelSize: 12; color: Theme.text
                            elide: Text.ElideRight; Layout.fillWidth: true
                        }
                        Text {
                            // tokens < 0 = unknown (the daemon wire carries no token counter).
                            text: entry.tokens >= 0 ? entry.time + " · " + entry.tokens + qsTr(" tok")
                                                    : entry.time
                            font.family: FontIcons.mono; font.pixelSize: 10; color: Theme.textMuted
                        }
                    }

                    Text {
                        visible: entry.current === true
                        text: qsTr("current")
                        font.family: FontIcons.display; font.pixelSize: 10; color: Theme.accent
                    }
                    Kit.TextButton {
                        visible: entry.current !== true && !root.foreignSession
                        text: qsTr("Restore")
                        onClicked: confirmDialog.confirmFor(entry.id, entry.label)
                    }
                }
            }
        }

        // Empty state: a fresh session has no checkpoints yet (they appear as tools run).
        Text {
            visible: Checkpoints.count === 0
            Layout.fillWidth: true
            text: qsTr("No checkpoints yet")
            font.family: FontIcons.display; font.pixelSize: 12; color: Theme.textMuted
        }
    }
}
