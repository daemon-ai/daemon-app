#pragma once

#include <QColor>
#include <QMetaType>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QTextLayout>
#include <QVector>

#include <cstdint>
#include <memory>

namespace be::diagram {

// Matches QSGGeometry::ColoredPoint2D (float position + unsigned byte RGBA), so a
// Mesh can be memcpy-friendly when uploaded to the scene graph.
struct Vertex {
    float x = 0.0f;
    float y = 0.0f;
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
    unsigned char a = 255;
};

// One batchable geometry chunk. Indices are 16-bit so the chunk maps onto Qt's
// scene-graph batching directly; callers must split before 65536 vertices.
struct Mesh {
    QVector<Vertex> vertices;
    QVector<quint16> indices;
    QRectF bounds;

    bool isEmpty() const { return vertices.isEmpty() || indices.isEmpty(); }
    void clear()
    {
        vertices.clear();
        indices.clear();
        bounds = {};
    }
};

// A laid-out label. The QTextLayout is built off the render thread and handed to
// a QSGTextNode unchanged, so it is shared (immutable after publication).
struct LabelRun {
    std::shared_ptr<QTextLayout> layout;
    QPointF origin;          // top-left in diagram space
    QColor color;
    QRectF bounds;           // diagram space
    int lod = 0;             // minimum zoom tier at which the label is shown
};

enum class HitKind { Node, Edge };

struct HitShape {
    QRectF bounds;           // diagram space
    QString id;
    HitKind kind = HitKind::Node;
};

// Immutable snapshot transferred from the engine (worker) to the item (render).
// Opaque and translucent geometry are split because opaque batches are cheaper.
struct RenderSnapshot {
    Mesh nodeFills;          // opaque
    Mesh nodeBorders;
    Mesh edges;
    Mesh arrowheads;
    Mesh clusters;           // subgraph boxes (drawn behind nodes)

    QVector<LabelRun> labels;
    QVector<HitShape> hits;  // simple linear index; small diagrams

    QRectF diagramBounds;
    quint64 contentRevision = 0;
    bool hasError = false;
    QString errorText;
};

using RenderSnapshotPtr = std::shared_ptr<const RenderSnapshot>;

} // namespace be::diagram

Q_DECLARE_METATYPE(be::diagram::RenderSnapshotPtr)
