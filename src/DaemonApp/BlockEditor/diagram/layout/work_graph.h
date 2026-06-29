// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QHash>
#include <QPointF>
#include <QString>
#include <QVector>

namespace be::diagram {

// Layout-only node kinds. Real nodes map back to DiagramModel.nodes; the rest are
// virtual nodes inserted during the dagre-style pipeline.
enum class WorkKind {
    Real,           // a DiagramNode (srcNode >= 0)
    EdgeDummy,      // long-edge waypoint (becomes a bend point)
    EdgeLabelDummy, // sized label slot on an edge
    BorderLeft,     // left wall of a cluster at a given rank
    BorderRight,    // right wall of a cluster at a given rank
};

struct WorkNode {
    QString id;
    WorkKind kind = WorkKind::Real;
    int rank = 0;
    int order = 0;
    qreal x = 0.0;
    qreal y = 0.0;
    qreal w = 0.0;
    qreal h = 0.0;
    QString cluster;  // innermost cluster id this node belongs to (for ordering)
    int srcNode = -1; // index into DiagramModel.nodes (Real only)
    int edgeIdx = -1; // index into DiagramModel.edges (dummy/label nodes)
};

struct WorkEdge {
    int u = -1; // tail work-node index
    int v = -1; // head work-node index
    double weight = 1.0;
    int minlen = 1;
    bool reversed = false; // flipped to break a cycle
    int edgeIdx = -1;      // originating DiagramEdge, or -1 for synthetic edges
};

// A directed graph the layout pipeline mutates in place. Edges always point from
// lower rank to higher rank after acyclic()/rank().
struct WorkGraph {
    QVector<WorkNode> nodes;
    QVector<WorkEdge> edges;
    QHash<QString, int> index;

    int add(const WorkNode& node) {
        const int i = nodes.size();
        nodes.push_back(node);
        if (!node.id.isEmpty()) {
            index.insert(node.id, i);
        }
        return i;
    }

    int addEdge(int u, int v, double weight, int minlen, int edgeIdx) {
        WorkEdge e;
        e.u = u;
        e.v = v;
        e.weight = weight;
        e.minlen = minlen;
        e.edgeIdx = edgeIdx;
        const int i = edges.size();
        edges.push_back(e);
        return i;
    }

    QVector<QVector<int>> outEdges() const {
        QVector<QVector<int>> out(nodes.size());
        for (int i = 0; i < edges.size(); ++i) {
            if (edges[i].u >= 0) {
                out[edges[i].u].push_back(i);
            }
        }
        return out;
    }

    QVector<QVector<int>> inEdges() const {
        QVector<QVector<int>> in(nodes.size());
        for (int i = 0; i < edges.size(); ++i) {
            if (edges[i].v >= 0) {
                in[edges[i].v].push_back(i);
            }
        }
        return in;
    }
};

} // namespace be::diagram
