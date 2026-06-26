import QtQuick
import QtQuick.Controls as QQC
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Reusable Daemon Kit graph view: renders a GraphModel as a force-laid-out node-link diagram
// (Repeater of NodeChip positioned by the model's normalized nx/ny, edges on a Canvas), with pan/zoom,
// tap selection, and an optional visual connector (drag from a node's handle onto another node ->
// connectorRequested(srcId, dstId), without creating the edge - the consumer decides). Styling is
// supplied by callbacks so each surface themes nodes/edges by its own kinds. Modeled on QuickQanava's
// GraphView/connector, implemented in-house.
Item {
    id: root

    property var model: null
    property bool connectorEnabled: false

    // Styling callbacks (consumers override). `data` is the whole node/edge map.
    property var nodeColor: function(kind, data) { return Theme.textMuted; }
    property var nodeDot: function(data) { return "transparent"; }
    property var edgeColor: function(edge) { return Theme.border; }
    property var edgeWidth: function(edge) { return 1.0; }
    property var edgeDash: function(edge) { return []; }

    signal nodeActivated(string id)
    signal connectorRequested(string srcId, string dstId)

    // Active connector-drag state (world coords for the rubber-band endpoint).
    property bool _dragging: false
    property string _dragSrc: ""
    property real _dragX: 0
    property real _dragY: 0

    function _radiusFor(degree) { return 7 + Math.min(10, degree * 2); }

    function _hitSession(wx, wy) {
        // Nearest node within its radius to the world point (wx,wy); "" if none.
        var best = "";
        var bestD = 1e9;
        for (var i = 0; i < nodes.count; ++i) {
            const it = nodes.itemAt(i);
            if (!it || it.nodeId === root._dragSrc)
                continue;
            const dx = it.cx - wx;
            const dy = it.cy - wy;
            const d = Math.sqrt(dx * dx + dy * dy);
            if (d < it.hitRadius && d < bestD) {
                bestD = d;
                best = it.nodeId;
            }
        }
        return best;
    }

    Rectangle {
        id: viewport
        anchors.fill: parent
        radius: 8
        color: Theme.surface
        border.color: Theme.border
        clip: true

        WheelHandler {
            target: null
            onWheel: function(event) {
                const f = event.angleDelta.y > 0 ? 1.12 : 0.89;
                world.scale = Math.max(0.4, Math.min(3.0, world.scale * f));
            }
        }

        Item {
            id: world
            width: viewport.width
            height: viewport.height
            transformOrigin: Item.TopLeft

            // Pan by dragging empty space (node/connector handlers grab first).
            DragHandler {
                target: world
                dragThreshold: 4
            }

            // --- Edges ---
            Canvas {
                id: edgeCanvas
                anchors.fill: parent
                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()
                onPaint: {
                    const ctx = getContext("2d");
                    ctx.reset();
                    if (!root.model)
                        return;
                    const w = width;
                    const h = height;
                    const es = root.model.edges();
                    for (let i = 0; i < es.length; ++i) {
                        const e = es[i];
                        ctx.beginPath();
                        ctx.moveTo(e.x1 * w, e.y1 * h);
                        ctx.lineTo(e.x2 * w, e.y2 * h);
                        ctx.lineWidth = root.edgeWidth(e);
                        ctx.strokeStyle = root.edgeColor(e);
                        ctx.setLineDash(root.edgeDash(e));
                        ctx.stroke();
                    }
                    ctx.setLineDash([]);

                    // Active connector rubber band.
                    if (root._dragging && root._dragSrc !== "") {
                        const src = root.model.nodeById(root._dragSrc);
                        // Find the source node's center via its delegate.
                        for (let j = 0; j < nodes.count; ++j) {
                            const it = nodes.itemAt(j);
                            if (it && it.nodeId === root._dragSrc) {
                                ctx.beginPath();
                                ctx.moveTo(it.cx, it.cy);
                                ctx.lineTo(root._dragX, root._dragY);
                                ctx.lineWidth = 2;
                                ctx.strokeStyle = Theme.accent;
                                ctx.setLineDash([4, 3]);
                                ctx.stroke();
                                ctx.setLineDash([]);
                                break;
                            }
                        }
                    }
                }
            }

            // --- Nodes ---
            Repeater {
                id: nodes
                model: root.model

                delegate: Item {
                    id: nodeItem
                    required property string nodeId
                    required property string nodeKind
                    required property string label
                    required property real nx
                    required property real ny
                    required property int degree
                    required property bool selected
                    required property var nodeData

                    readonly property real cx: nx * world.width
                    readonly property real cy: ny * world.height
                    readonly property real diameter: root._radiusFor(degree) * 2
                    readonly property real hitRadius: diameter

                    x: cx - width / 2
                    y: cy - height / 2
                    width: diameter
                    height: diameter

                    NodeChip {
                        anchors.centerIn: parent
                        text: nodeItem.label
                        fill: root.nodeColor(nodeItem.nodeKind, nodeItem.nodeData)
                        dotColor: root.nodeDot(nodeItem.nodeData)
                        diameter: nodeItem.diameter
                        selected: nodeItem.selected
                    }

                    TapHandler {
                        onTapped: {
                            if (root.model)
                                root.model.selectedId = nodeItem.nodeId;
                            root.nodeActivated(nodeItem.nodeId);
                        }
                    }

                    // Connector handle: drag from here onto another node to request an edge.
                    Rectangle {
                        id: handle
                        visible: root.connectorEnabled && (nodeItem.selected || hover.hovered)
                        width: 12
                        height: 12
                        radius: 6
                        color: Theme.accent
                        border.width: 1
                        border.color: Theme.surface
                        anchors.right: parent.right
                        anchors.top: parent.top
                        z: 5

                        HoverHandler { id: hover }

                        DragHandler {
                            target: null
                            onActiveChanged: {
                                if (active) {
                                    root._dragging = true;
                                    root._dragSrc = nodeItem.nodeId;
                                } else if (root._dragging) {
                                    const hit = root._hitSession(root._dragX, root._dragY);
                                    if (hit !== "" && hit !== root._dragSrc)
                                        root.connectorRequested(root._dragSrc, hit);
                                    root._dragging = false;
                                    root._dragSrc = "";
                                    edgeCanvas.requestPaint();
                                }
                            }
                            onCentroidChanged: {
                                if (root._dragging) {
                                    const wp = world.mapFromScene(centroid.scenePosition);
                                    root._dragX = wp.x;
                                    root._dragY = wp.y;
                                    edgeCanvas.requestPaint();
                                }
                            }
                        }
                    }
                }
            }
        }

        Text {
            anchors.centerIn: parent
            visible: !root.model || root.model.nodeCount === 0
            text: qsTr("No graph data")
            font.family: FontIcons.display
            font.pixelSize: 12
            color: Theme.textMuted
        }
    }

    // Repaint edges whenever the graph or selection changes.
    Connections {
        target: root.model
        function onGraphChanged() { edgeCanvas.requestPaint(); }
        function onSelectionChanged() { edgeCanvas.requestPaint(); }
    }
}
