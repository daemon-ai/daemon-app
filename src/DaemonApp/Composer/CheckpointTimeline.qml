// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Checkpoint timeline strip (E4/TOOL-9): one dot per durable checkpoint of the
// bound session, oldest -> newest, rendered above the composer. Hovering a dot
// names the checkpoint (the tool it precedes + time); clicking a non-current dot
// opens the rewind confirm. Foreign (ACP) sessions render the dots read-only
// with an explanatory tooltip — rewind is the foreign agent's, not ours (C4).
// Self-hides when the session has no checkpoints.
Item {
    id: root

    // The bound session runs on a foreign ACP engine: rewind is not offered.
    property bool foreignSession: false

    visible: Checkpoints.count > 0
    implicitHeight: visible ? row.implicitHeight + 4 : 0
    implicitWidth: row.implicitWidth

    RewindConfirmDialog { id: confirmDialog }

    RowLayout {
        id: row
        anchors.verticalCenter: parent.verticalCenter
        spacing: 6

        Text {
            text: FontIcons.fa_clock
            font.family: FontIcons.faSolid
            font.pixelSize: 9
            renderType: Text.NativeRendering
            color: Theme.textMuted
        }

        Repeater {
            model: Checkpoints.checkpoints

            delegate: Rectangle {
                id: dot
                required property var entry

                width: 9
                height: 9
                radius: width / 2
                color: entry.current ? Theme.accent
                     : dotArea.containsMouse ? Theme.text : Theme.textMuted
                border.width: entry.current ? 0 : 1
                border.color: Theme.border

                MouseArea {
                    id: dotArea
                    anchors.fill: parent
                    anchors.margins: -4 // finger-friendlier hit area than a 9px dot
                    hoverEnabled: true
                    cursorShape: entry.current || root.foreignSession ? Qt.ArrowCursor
                                                                      : Qt.PointingHandCursor
                    onClicked: {
                        if (!entry.current && !root.foreignSession)
                            confirmDialog.confirmFor(entry.id, entry.label);
                    }
                }

                Kit.Tooltip {
                    text: root.foreignSession
                          ? qsTr("%1 · %2 — rewind is managed by the foreign agent")
                                .arg(entry.label).arg(entry.time)
                          : entry.current ? qsTr("%1 · %2 (current)").arg(entry.label).arg(entry.time)
                                          : qsTr("%1 · %2 — click to rewind").arg(entry.label).arg(entry.time)
                    visible: dotArea.containsMouse
                    x: Math.round((dot.width - implicitWidth) / 2)
                    y: -implicitHeight - 8
                }
            }
        }
    }
}
