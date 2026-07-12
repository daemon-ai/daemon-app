// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Sessions roster: durable/live sessions with state, profile, token usage, and
// suspend/resume/close actions. The Active|Archived scope switcher (F6/DEL-6)
// swaps the roster to archived sessions with a per-row Restore.
Item {
    id: root
    objectName: "sessionsPage"

    readonly property bool archivedScope: SessionRoster.scope === "archived"

    PageHeader {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr("Sessions")
        icon: FontIcons.fa_comments
    }

    // Active | Archived scope switcher (F6). Switching to Archived re-fetches the
    // SessionScope::Archived listing (the TopLevel roster excludes it).
    RowLayout {
        id: scopeSwitch
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.leftMargin: 16
        spacing: 6

        Kit.TextButton {
            text: qsTr("Active")
            accentFilled: !root.archivedScope
            onClicked: SessionRoster.scope = "active"
        }
        Kit.TextButton {
            text: qsTr("Archived")
            accentFilled: root.archivedScope
            onClicked: SessionRoster.scope = "archived"
        }
    }

    QQC.ScrollView {
        anchors.top: scopeSwitch.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        clip: true
        contentWidth: availableWidth

        ListView {
            model: SessionRoster.sessions
            spacing: 8
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                id: sessionRow
                required property var entry
                // Click the row to drill in: an expandable detail panel.
                property bool expanded: false
                width: ListView.view.width
                height: body.implicitHeight + 24
                radius: 8
                color: Theme.surface
                border.color: Theme.border

                // Below the controls: clicking empty row area toggles the drill-in.
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sessionRow.expanded = !sessionRow.expanded

                    // Expandable session row named for its title.
                    Accessible.role: Accessible.ListItem
                    Accessible.name: sessionRow.entry.title
                    Accessible.onPressAction: sessionRow.expanded = !sessionRow.expanded
                }

                ColumnLayout {
                    id: body
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 12
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Text {
                            text: sessionRow.expanded ? FontIcons.fa_chevron_down
                                                      : FontIcons.fa_chevron_right
                            font.family: FontIcons.faSolid; font.pixelSize: 10
                            color: Theme.iconMuted
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            RowLayout {
                                spacing: 8
                                Text {
                                    text: entry.title
                                    font.family: FontIcons.display; font.pixelSize: 14
                                    font.bold: true; color: Theme.text
                                    elide: Text.ElideRight; Layout.maximumWidth: 320
                                }
                                Rectangle {
                                    radius: 3; color: Theme.hover
                                    implicitWidth: lc.implicitWidth + 10
                                    implicitHeight: lc.implicitHeight + 4
                                    Text {
                                        id: lc; anchors.centerIn: parent
                                        text: entry.lifecycle === "live" ? qsTr("LIVE")
                                                                         : qsTr("DURABLE")
                                        font.family: FontIcons.display; font.pixelSize: 9
                                        color: Theme.textMuted
                                    }
                                }
                            }
                            Text {
                                text: entry.profile + " · " + entry.lastActivity + " · "
                                      + entry.tokens + qsTr(" tok")
                                font.family: FontIcons.mono; font.pixelSize: 11
                                color: Theme.textMuted
                            }
                        }

                        Text {
                            text: entry.state
                            font.family: FontIcons.display; font.pixelSize: 11
                            color: entry.state === "active" ? Theme.accent : Theme.textMuted
                        }
                        // Archived scope: the row's single action is Restore (back to Active).
                        Kit.TextButton {
                            visible: root.archivedScope
                            text: qsTr("Restore")
                            onClicked: SessionRoster.restore(entry.id)
                        }
                        Kit.IconButton {
                            visible: !root.archivedScope
                            icon: entry.state === "suspended" ? FontIcons.fa_play
                                                              : FontIcons.fa_pause
                            iconPointSize: 11; implicitWidth: 30; implicitHeight: 26
                            tooltipText: entry.state === "suspended" ? qsTr("Resume")
                                                                     : qsTr("Suspend")
                            onClicked: entry.state === "suspended" ? SessionRoster.resume(entry.id)
                                                                  : SessionRoster.suspend(entry.id)
                        }
                        Kit.IconButton {
                            visible: !root.archivedScope
                            icon: FontIcons.fa_xmark; iconColor: Theme.danger
                            iconPointSize: 12; implicitWidth: 30; implicitHeight: 26
                            tooltipText: qsTr("Close session")
                            onClicked: SessionRoster.close(entry.id)
                        }
                    }

                    // --- Drill-in detail panel ----------------------------------
                    ColumnLayout {
                        visible: sessionRow.expanded
                        Layout.fillWidth: true
                        Layout.topMargin: 4
                        spacing: 3
                        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
                        Text {
                            text: qsTr("Session %1 · %2 · %3").arg(entry.id)
                                  .arg(entry.lifecycle).arg(entry.state)
                            font.family: FontIcons.mono; font.pixelSize: 11; color: Theme.textMuted
                        }
                        Text {
                            text: entry.rewindable ? qsTr("Rewindable — checkpoints available")
                                                   : qsTr("Not rewindable")
                            font.family: FontIcons.display; font.pixelSize: 11
                            color: Theme.textMuted
                        }
                    }
                }
            }
        }
    }
}
