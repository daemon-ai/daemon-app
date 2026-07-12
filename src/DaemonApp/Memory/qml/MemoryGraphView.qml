// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Presentation

// Graph sub-tab (GUI-only): a 2D node-link render of the shared MemoryGraphModel.
// Nodes are a Repeater positioned from the model's normalized layout; edges are
// drawn on a Canvas. Pan via drag, zoom via wheel/buttons, tap to select, double
// tap to expand. An inspector panel shows the selected node + its neighbours. The
// TUI mirrors this data as an adjacency drill-down (no render).
Item {
    id: root

    function nodeColor(kind) {
        switch (kind) {
        case "memory": return Theme.accent;
        case "entity": return Theme.statusOk;
        case "fact": return Theme.warning;
        case "conflict": return Theme.danger;
        default: return Theme.textMuted;
        }
    }

    MemoryGraphModel {
        id: gm
        service: typeof Memory !== "undefined" ? Memory : null
        kind: "association"
        onGraphChanged: edgeCanvas.requestPaint()
        onSelectionChanged: edgeCanvas.requestPaint()
    }

    readonly property var kinds: [
        { id: "association", label: qsTr("Association") },
        { id: "knowledge", label: qsTr("Knowledge") },
        { id: "constellation", label: qsTr("Constellation") }
    ]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        // --- Controls -------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Repeater {
                model: root.kinds
                delegate: Kit.TextButton {
                    required property var modelData
                    text: modelData.label
                    accentFilled: gm.kind === modelData.id
                    onClicked: gm.kind = modelData.id
                }
            }
            Item { width: 12 }
            Text {
                text: qsTr("Depth %1").arg(gm.depth)
                font.family: FontIcons.display
                font.pixelSize: 11
                color: Theme.textMuted
            }
            Kit.IconButton {
                icon: FontIcons.fa_minus
                iconPointSize: 10
                tooltipText: qsTr("Less depth")
                onClicked: gm.depth = gm.depth - 1
            }
            Kit.IconButton {
                icon: FontIcons.fa_plus
                iconPointSize: 10
                tooltipText: qsTr("More depth")
                onClicked: gm.depth = gm.depth + 1
            }
            Item { Layout.fillWidth: true }
            Text {
                text: qsTr("%n node(s)", "", gm.nodeCount) + " · " + qsTr("%n edge(s)", "", gm.edgeCount)
                font.family: FontIcons.mono
                font.pixelSize: 10
                color: Theme.textMuted
            }
            Kit.IconButton {
                icon: FontIcons.fa_rotate
                iconPointSize: 10
                tooltipText: qsTr("Reset view / focus")
                onClicked: {
                    world.scale = 1.0;
                    world.x = 0;
                    world.y = 0;
                    gm.resetFocus();
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            // --- Graph viewport ----------------------------------------------
            Rectangle {
                id: viewport
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 8
                color: Theme.surface
                border.color: Theme.border
                clip: true

                WheelHandler {
                    target: null
                    onWheel: function(event) {
                        const f = event.angleDelta.y > 0 ? 1.12 : 0.89;
                        world.scale = Math.max(0.35, Math.min(3.0, world.scale * f));
                    }
                }

                Item {
                    id: world
                    width: viewport.width
                    height: viewport.height
                    transformOrigin: Item.TopLeft

                    DragHandler {
                        id: panner
                        target: world
                        dragThreshold: 4
                    }

                    Canvas {
                        id: edgeCanvas
                        anchors.fill: parent
                        onWidthChanged: requestPaint()
                        onHeightChanged: requestPaint()
                        onPaint: {
                            const ctx = getContext("2d");
                            ctx.reset();
                            const w = width;
                            const h = height;
                            const edges = gm.edges();
                            for (let i = 0; i < edges.length; ++i) {
                                const e = edges[i];
                                ctx.beginPath();
                                ctx.moveTo(e.x1 * w, e.y1 * h);
                                ctx.lineTo(e.x2 * w, e.y2 * h);
                                ctx.lineWidth = e.edgeType === "contradicts" ? 2.0 : 1.0;
                                ctx.strokeStyle = e.edgeType === "contradicts"
                                    ? Theme.danger
                                    : (e.edgeType === "mentions" ? Theme.textMuted : Theme.border);
                                ctx.stroke();
                            }
                        }
                    }

                    Repeater {
                        model: gm
                        delegate: Item {
                            id: nodeItem
                            required property string nodeId
                            required property string nodeKind
                            required property string label
                            required property real weight
                            required property real nx
                            required property real ny
                            required property bool selected
                            required property int degree

                            readonly property real radius: 7 + Math.min(10, weight * 6) + Math.min(6, degree)
                            x: nx * world.width - radius
                            y: ny * world.height - radius
                            width: radius * 2
                            height: radius * 2

                            Rectangle {
                                anchors.fill: parent
                                radius: width / 2
                                color: root.nodeColor(nodeItem.nodeKind)
                                opacity: nodeItem.selected ? 1.0 : 0.85
                                border.width: nodeItem.selected ? 2 : 0
                                border.color: Theme.text
                            }
                            Text {
                                anchors.top: parent.bottom
                                anchors.topMargin: 1
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: nodeItem.label
                                font.family: FontIcons.display
                                font.pixelSize: 9
                                color: Theme.text
                                visible: nodeItem.selected || nodeItem.radius > 12
                                width: 120
                                horizontalAlignment: Text.AlignHCenter
                                elide: Text.ElideRight
                            }
                            // Graph node named for its label; activating selects it.
                            Accessible.role: Accessible.Button
                            Accessible.name: nodeItem.label
                            Accessible.selected: nodeItem.selected
                            Accessible.onPressAction: gm.selectedId = nodeItem.nodeId

                            TapHandler { onTapped: gm.selectedId = nodeItem.nodeId }
                            TapHandler {
                                acceptedButtons: Qt.LeftButton
                                onDoubleTapped: gm.expand(nodeItem.nodeId)
                            }
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: gm.nodeCount === 0
                    text: qsTr("No graph data for this scope")
                    font.family: FontIcons.display
                    font.pixelSize: 12
                    color: Theme.textMuted
                }
            }

            // --- Inspector ----------------------------------------------------
            Rectangle {
                id: inspector
                Layout.preferredWidth: 260
                Layout.fillHeight: true
                radius: 8
                color: Theme.surface
                border.color: Theme.border
                visible: gm.selectedId.length > 0

                readonly property var node: gm.selectedId.length > 0 ? gm.nodeById(gm.selectedId) : ({})

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 6

                    Text {
                        text: inspector.node.label !== undefined ? inspector.node.label : ""
                        font.family: FontIcons.display
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: Theme.text
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Text {
                        text: (inspector.node.kind !== undefined
                                   ? DisplayPresenter.enumLabel("memory.nodeKind", inspector.node.kind)
                                   : "")
                              + qsTr(" · degree ") + (inspector.node.degree !== undefined ? inspector.node.degree : 0)
                        font.family: FontIcons.mono
                        font.pixelSize: 10
                        color: Theme.textMuted
                    }
                    Row {
                        spacing: 6
                        Kit.TextButton {
                            text: qsTr("Expand")
                            onClicked: gm.expand(gm.selectedId)
                        }
                        Kit.TextButton {
                            text: qsTr("Clear")
                            onClicked: gm.selectedId = ""
                        }
                    }
                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
                    Text {
                        text: qsTr("Neighbours")
                        font.family: FontIcons.display
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        color: Theme.accent
                    }
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: gm.selectedId.length > 0 ? gm.neighborsOf(gm.selectedId) : []
                        spacing: 3
                        QQC.ScrollBar.vertical: Kit.ScrollBar {}
                        delegate: RowLayout {
                            required property var modelData
                            width: ListView.view.width
                            spacing: 6
                            Text {
                                text: modelData.edgeType
                                font.family: FontIcons.mono
                                font.pixelSize: 9
                                color: Theme.textMuted
                                Layout.preferredWidth: 70
                                elide: Text.ElideRight
                            }
                            Text {
                                text: modelData.label
                                font.family: FontIcons.display
                                font.pixelSize: 11
                                color: Theme.accent
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                                TapHandler { onTapped: gm.selectedId = modelData.id }
                            }
                        }
                    }
                }
            }
        }
    }
}
