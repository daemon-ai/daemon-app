#pragma once

#include <QColor>
#include <QFont>
#include <QHash>
#include <QMetaType>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

namespace be::diagram {

enum class Direction { TB, BT, LR, RL };

enum class NodeShape {
    Rect,
    RoundRect,
    Stadium,     // ([text])
    Subroutine,  // [[text]]
    Cylinder,    // [(text)]
    Circle,      // ((text))
    Diamond,     // {text}
    Hexagon,     // {{text}}
};

enum class EdgeKind { Normal, Thick, Dotted };

enum class ArrowHead { None, Arrow };

// Resolved presentation colors and metrics. Mapped from the host theme so the
// diagram matches the editor chrome; classDef/style directives may override.
struct Style {
    QColor nodeFill = QColor("#ffffff");
    QColor nodeStroke = QColor("#e8e5df");
    QColor edgeStroke = QColor("#6f6f6f");
    QColor textColor = QColor("#111111");
    QColor clusterFill = QColor("#fbfbfa");
    QColor clusterStroke = QColor("#e8e5df");
    QColor labelBackground = QColor("#ffffff");
    qreal nodeStrokeWidth = 1.5;
    qreal edgeStrokeWidth = 1.5;
    QFont font = QFont(QStringLiteral("sans-serif"), 11);
};

struct DiagramNode {
    QString id;
    QString label;
    NodeShape shape = NodeShape::Rect;
    QString classRef;
    QString parentCluster;   // innermost enclosing subgraph id, or empty

    // Filled by layout (diagram space).
    int rank = -1;
    int order = -1;
    qreal x = 0.0;       // center
    qreal y = 0.0;       // center
    qreal width = 0.0;
    qreal height = 0.0;
};

struct DiagramEdge {
    QString fromId;
    QString toId;
    QString label;
    EdgeKind kind = EdgeKind::Normal;
    ArrowHead head = ArrowHead::Arrow;
    ArrowHead tail = ArrowHead::None;

    // Measured label box (set by the engine before layout so the layout can
    // reserve space via an edge-label dummy node). 0 when there is no label.
    qreal labelWidth = 0.0;
    qreal labelHeight = 0.0;

    // Filled by layout: polyline from source border to target border.
    QVector<QPointF> points;
    QPointF labelPos;
};

struct DiagramCluster {
    QString id;
    QString title;
    QString parentId;            // enclosing subgraph id, or empty
    Direction dir = Direction::TB;
    bool dirSet = false;         // true if the subgraph declared its own direction
    QStringList memberIds;       // direct child node ids (not nested-cluster members)

    // Filled by layout (diagram space).
    QRectF bounds;
};

struct DiagramModel {
    QString family;                  // "flowchart"
    Direction dir = Direction::TB;
    QVector<DiagramNode> nodes;
    QVector<DiagramEdge> edges;
    QVector<DiagramCluster> clusters;
    QHash<QString, int> nodeIndex;                       // id -> index into nodes
    QHash<QString, QHash<QString, QString>> classDefs;   // classDef name -> props

    bool valid = false;
    QString error;

    int indexOf(const QString &id) const { return nodeIndex.value(id, -1); }
};

} // namespace be::diagram

Q_DECLARE_METATYPE(be::diagram::Style)
