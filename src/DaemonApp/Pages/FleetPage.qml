// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Fleet tree: the flattened orchestrator/worker hierarchy with indentation by
// depth, a status badge, pause/resume, and (orchestrator units) scale. Daemon-backed +
// offline-first; refreshes the Tree on show.
Item {
    id: root

    // Last control rejection (e.g. Pause/Scale Unsupported on an engine leaf - PRO-10).
    property string controlError: ""

    // Refresh the daemon tree whenever the page becomes visible (no-op in mock mode).
    onVisibleChanged: if (visible) FleetTree.refresh()
    Component.onCompleted: FleetTree.refresh()

    Connections {
        target: FleetTree
        function onControlRejected(message) {
            root.controlError = message;
            errorClear.restart();
        }
    }
    Timer { id: errorClear; interval: 5000; onTriggered: root.controlError = "" }

    PageHeader {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr("Fleet")
        icon: FontIcons.fa_sitemap
    }

    Kit.IconButton {
        anchors.verticalCenter: header.verticalCenter
        anchors.right: header.right
        anchors.rightMargin: 12
        icon: FontIcons.fa_rotate
        iconColor: Theme.iconMuted
        iconPointSize: 12
        tooltipText: qsTr("Refresh")
        onClicked: FleetTree.refresh()
    }

    QQC.ScrollView {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        clip: true
        contentWidth: availableWidth

        ListView {
            model: FleetTree.nodes
            spacing: 6
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                id: fleetRow
                required property var entry
                // Click the row to drill into the node's detail.
                property bool expanded: false
                width: ListView.view.width
                height: fbody.implicitHeight + 20
                radius: 8
                color: Theme.surface
                border.color: Theme.border

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: fleetRow.expanded = !fleetRow.expanded
                }

                ColumnLayout {
                    id: fbody
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: 12 + entry.depth * 24
                    anchors.rightMargin: 12
                    anchors.topMargin: 10
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Text {
                            text: entry.kind === "orchestrator" ? FontIcons.fa_sitemap
                                                                : FontIcons.fa_robot
                            font.family: FontIcons.faSolid; font.pixelSize: 13
                            color: Theme.iconMuted
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            Text {
                                text: entry.name
                                font.family: FontIcons.display; font.pixelSize: 13; color: Theme.text
                                elide: Text.ElideRight; Layout.fillWidth: true
                            }
                            Text {
                                text: entry.model
                                font.family: FontIcons.mono; font.pixelSize: 10; color: Theme.textMuted
                            }
                        }

                        Text {
                            text: entry.status
                            font.family: FontIcons.display; font.pixelSize: 11
                            color: entry.status === "running" ? Theme.accent : Theme.textMuted
                        }
                        Kit.IconButton {
                            icon: entry.status === "paused" ? FontIcons.fa_play : FontIcons.fa_pause
                            iconPointSize: 11; implicitWidth: 30; implicitHeight: 26
                            tooltipText: entry.status === "paused" ? qsTr("Resume") : qsTr("Pause")
                            onClicked: entry.status === "paused" ? FleetTree.resume(entry.id)
                                                                : FleetTree.pause(entry.id)
                        }
                    }

                    // --- Drill-in detail panel ----------------------------------
                    ColumnLayout {
                        visible: fleetRow.expanded
                        Layout.fillWidth: true
                        spacing: 6
                        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
                        Text {
                            text: qsTr("%1 · %2 · depth %3").arg(entry.id).arg(entry.kind)
                                  .arg(entry.depth)
                            font.family: FontIcons.mono; font.pixelSize: 11; color: Theme.textMuted
                        }
                        // PRO-10: scale only an orchestrator's worker count (engine leaves reject it).
                        RowLayout {
                            visible: entry.kind === "orchestrator"
                            spacing: 8
                            Text {
                                text: qsTr("Workers")
                                font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
                            }
                            QQC.SpinBox {
                                id: scaleSpin
                                from: 0; to: 16; value: 1
                                implicitHeight: 26
                            }
                            Kit.IconButton {
                                icon: FontIcons.fa_layer_group
                                iconPointSize: 11; implicitWidth: 30; implicitHeight: 26
                                tooltipText: qsTr("Scale to %1").arg(scaleSpin.value)
                                onClicked: FleetTree.scale(entry.id, scaleSpin.value)
                            }
                        }
                    }
                }
            }
        }
    }

    // Empty state (a fresh daemon with no delegated subagents, or offline with an empty cache).
    Text {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 16
        visible: FleetTree.nodes.count === 0
        text: qsTr("No active subagents")
        color: Theme.textMuted
        font.family: FontIcons.display
        font.pixelSize: 13
    }

    // Control-rejection toast (PRO-10 Unsupported on a leaf, transient drop, etc.).
    Rectangle {
        visible: root.controlError !== ""
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 16
        radius: 8
        color: Theme.surface
        border.color: Theme.border
        implicitWidth: toast.implicitWidth + 24
        implicitHeight: toast.implicitHeight + 16
        Text {
            id: toast
            anchors.centerIn: parent
            text: root.controlError
            color: Theme.text
            font.family: FontIcons.display
            font.pixelSize: 12
        }
    }
}
