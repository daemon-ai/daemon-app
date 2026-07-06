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
    // Operator steer/cancel rejections (F4): Submit to a child session the node refused
    // (Forbidden / unknown session / transport drop) surfaces in the same toast.
    Connections {
        target: SessionRoster
        function onOperationFailed(message) {
            root.controlError = message;
            errorClear.restart();
        }
    }
    Timer { id: errorClear; interval: 5000; onTriggered: root.controlError = "" }

    // Steer prompt (F4/DEL-4): a small composer that injects operator input into the child
    // session — Steer nudges a running turn; on an idle child it starts a turn instead.
    Kit.Dialog {
        id: steerDialog
        property string sessionId: ""
        property bool running: false
        title: qsTr("Steer this agent")
        acceptText: qsTr("Send")
        acceptEnabled: steerInput.text.trim().length > 0
        onAccepted: {
            var text = steerInput.text.trim();
            if (text.length === 0 || steerDialog.sessionId.length === 0)
                return;
            if (steerDialog.running)
                SessionRoster.steer(steerDialog.sessionId, text);
            else
                SessionRoster.startTurn(steerDialog.sessionId, text);
            steerInput.text = "";
        }
        onRejected: steerInput.text = ""

        contentItem: ColumnLayout {
            spacing: 6
            Rectangle {
                Layout.preferredWidth: 360
                Layout.preferredHeight: 80
                radius: Theme.radius
                color: Theme.searchBackground
                border.width: 1
                border.color: steerInput.activeFocus ? Theme.searchFocusBorder
                                                     : Theme.searchBorder
                QQC.ScrollView {
                    anchors.fill: parent
                    anchors.margins: 4
                    Kit.TextArea {
                        id: steerInput
                        placeholderText: qsTr("Message to inject…")
                    }
                }
            }
            Text {
                Layout.fillWidth: true
                text: steerDialog.running
                      ? qsTr("Steers the running turn without interrupting it.")
                      : qsTr("The agent is idle — this starts a new turn.")
                font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
                wrapMode: Text.WordWrap
            }
        }

        function openFor(sessionId, running) {
            steerDialog.sessionId = sessionId;
            steerDialog.running = running;
            steerDialog.open();
        }
    }

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

                        // Engine identity (C3/ENG-7 + F3): Native core vs the foreign agent the
                        // unit runs on, decoded straight off the wire UnitNode (v29). The label is
                        // single-sourced through the shared EngineIdentity facade so the Fleet chip,
                        // the composer/session chips and the approval origin chip all read identically
                        // ("<agent> · ACP" / "· stream-json" / bare agent when the catalog is cold).
                        Kit.Chip {
                            visible: entry.engine !== undefined && entry.engine !== ""
                            text: {
                                if (entry.engine !== "Foreign")
                                    return qsTr("Native")
                                return EngineIdentity.labelFor(
                                    entry.engine, entry.engineAgent || "",
                                    EngineIdentity.protocolForAgent(entry.engineAgent || ""))
                            }
                            iconGlyph: entry.engine === "Foreign" ? FontIcons.fa_robot
                                                                  : FontIcons.fa_microchip
                            tone: entry.engine === "Foreign" ? "accent" : "muted"
                        }
                        // Delegation lifetime (F3): a persistent managed child stays in the tree;
                        // an ephemeral subagent is reaped after completion.
                        Kit.Chip {
                            visible: entry.lifetime !== undefined && entry.lifetime !== ""
                            text: entry.lifetime === "Ephemeral" ? qsTr("Ephemeral")
                                                                 : qsTr("Persistent")
                            iconGlyph: entry.lifetime === "Ephemeral" ? FontIcons.fa_hourglass_half
                                                                      : FontIcons.fa_thumbtack
                            tone: "muted"
                        }
                        Text {
                            text: entry.status
                            font.family: FontIcons.display; font.pixelSize: 11
                            color: entry.status === "running" ? Theme.accent : Theme.textMuted
                        }
                        // Operator steer/cancel (F4/DEL-4): delegated children only (a child IS a
                        // session; the wire Submit is session-addressable). Roots/primaries keep
                        // their own composer.
                        readonly property bool isDelegatedChild:
                            entry.sessionId !== undefined && entry.sessionId !== "" &&
                            (entry.role === "ManagedChild" || entry.role === "EphemeralSubagent" ||
                             (entry.role === "" && entry.depth > 0))
                        // Drill into the child's transcript, read-only (F1/DEL-1). A child IS a
                        // session; the viewer reuses the transcript stack with the composer hidden.
                        Kit.IconButton {
                            visible: parent.isDelegatedChild
                            icon: FontIcons.fa_comments
                            iconPointSize: 10; implicitWidth: 30; implicitHeight: 26
                            tooltipText: qsTr("Open transcript")
                            onClicked: Nav.openSession(entry.sessionId, true)
                        }
                        Kit.IconButton {
                            visible: parent.isDelegatedChild
                            icon: FontIcons.fa_wand_magic_sparkles
                            iconPointSize: 11; implicitWidth: 30; implicitHeight: 26
                            tooltipText: qsTr("Steer…")
                            onClicked: steerDialog.openFor(entry.sessionId,
                                                           entry.status === "running")
                        }
                        Kit.IconButton {
                            visible: parent.isDelegatedChild && entry.status === "running"
                            icon: FontIcons.fa_square
                            iconPointSize: 10; implicitWidth: 30; implicitHeight: 26
                            tooltipText: qsTr("Cancel the running turn")
                            onClicked: SessionRoster.interrupt(entry.sessionId)
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
                            Kit.SpinBox {
                                id: scaleSpin
                                from: 0; to: 16; value: 1
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
