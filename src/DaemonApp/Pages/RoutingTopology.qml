import QtQuick
import DaemonApp.Theme
import DaemonApp.Graph as Graph

// The routing manager's topology: a force-directed view of the DaemonNet (agents chained by
// delegation/runs, wired to origins/destinations) rendered with the reusable Daemon Kit GraphView.
// Curates the routing+fleet subgraph from DaemonNet.nodes()/edges(), styles nodes by kind and edges
// by provenance/SinkKind, and turns drag-an-origin-onto-a-session into a pinRequested. Read-mostly
// otherwise; selection syncs with the control surface via highlight()/sessionActivated.
Item {
    id: root

    signal sessionActivated(string sessionId)
    signal pinRequested(string originKey, string sessionId)

    // Highlight a session node from the outside (the control surface selecting a route).
    function highlight(sessionId) { gm.selectedId = sessionId; }

    Graph.GraphModel { id: gm }

    function rebuild() {
        if (typeof DaemonNet === "undefined")
            return;
        const allNodes = DaemonNet.nodes();
        const allEdges = DaemonNet.edges();
        const keepNode = { agent: 1, session: 1, origin: 1, destination: 1, room: 1, channel: 1 };
        const keepEdge = { delegation: 1, runs: 1, inbound: 1, outbound: 1, inPlace: 1 };

        const byId = {};
        for (let i = 0; i < allNodes.length; ++i)
            byId[allNodes[i].id] = allNodes[i];

        const edgesOut = [];
        for (let j = 0; j < allEdges.length; ++j) {
            const e = allEdges[j];
            if (!keepEdge[e.edgeKind])
                continue;
            edgesOut.push({
                "source": e.source, "target": e.target, "edgeType": e.edgeKind,
                "provenance": e.provenance !== undefined ? e.provenance : "",
                "sinkKind": e.sinkKind !== undefined ? e.sinkKind : ""
            });
        }

        const nodesOut = [];
        const seen = {};
        function take(id) {
            if (seen[id] || !byId[id] || !keepNode[byId[id].kind])
                return;
            seen[id] = 1;
            nodesOut.push(byId[id]);
        }
        for (let k = 0; k < edgesOut.length; ++k) {
            take(edgesOut[k].source);
            take(edgesOut[k].target);
        }
        gm.setNodes(nodesOut);
        gm.setEdges(edgesOut);
    }

    Component.onCompleted: rebuild()
    Connections {
        target: typeof DaemonNet !== "undefined" ? DaemonNet : null
        function onChanged() { root.rebuild(); }
    }

    Graph.GraphView {
        id: view
        anchors.fill: parent
        model: gm
        connectorEnabled: true

        nodeColor: function(kind, data) {
            switch (kind) {
            case "agent": return Theme.accent;
            case "session": return Theme.statusOk;
            case "destination": return Theme.warning;
            case "room":
            case "channel": return Theme.iconMuted;
            default: return Theme.textMuted; // origin
            }
        }
        edgeColor: function(e) {
            if (e.edgeType === "inbound")
                return e.provenance === "pinned" ? Theme.accent : Theme.textMuted;
            if (e.edgeType === "outbound")
                return Theme.statusOk;
            return Theme.border; // delegation / runs / inPlace (structural)
        }
        edgeWidth: function(e) {
            return (e.edgeType === "inbound" && e.provenance === "pinned") ? 2.0 : 1.0;
        }
        edgeDash: function(e) {
            if (e.edgeType === "inbound" && e.provenance === "derived")
                return [2, 3];
            if (e.edgeType === "outbound" && e.sinkKind === "spectator")
                return [4, 3];
            return [];
        }

        onNodeActivated: function(id) {
            const n = gm.nodeById(id);
            if (n && n.kind === "session")
                root.sessionActivated(id);
        }
        // Drag an origin onto a session -> request a pin (consumer decides).
        onConnectorRequested: function(srcId, dstId) {
            const s = gm.nodeById(srcId);
            const d = gm.nodeById(dstId);
            if (s && d && s.kind === "origin" && d.kind === "session"
                && s.originKey !== undefined && s.originKey !== "")
                root.pinRequested(s.originKey, dstId);
        }
    }

    // Legend.
    Row {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 8
        spacing: 12
        Text { text: qsTr("\u2014 pinned"); color: Theme.accent; font.pixelSize: 9; font.family: FontIcons.display }
        Text { text: qsTr("\u00b7\u00b7 derived"); color: Theme.textMuted; font.pixelSize: 9; font.family: FontIcons.display }
        Text { text: qsTr("- - spectator"); color: Theme.statusOk; font.pixelSize: 9; font.family: FontIcons.display }
    }
}
