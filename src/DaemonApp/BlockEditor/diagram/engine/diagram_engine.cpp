#include "diagram/engine/diagram_engine.h"

#include "diagram/geometry/path_mesh.h"
#include "diagram/geometry/shapes.h"
#include "diagram/layout/layered_layout.h"
#include "diagram/parser/detect.h"
#include "diagram/parser/flowchart_parser.h"

#include <algorithm>
#include <cmath>

namespace be::diagram {

namespace {

constexpr qreal kPadX = 16.0;
constexpr qreal kPadY = 10.0;
constexpr qreal kMinW = 40.0;
constexpr qreal kMinH = 30.0;
constexpr qreal kArrow = 11.0;

QColor parseColor(const QString &s, const QColor &fallback)
{
    if (s.isEmpty()) {
        return fallback;
    }
    QColor c(s.trimmed());
    return c.isValid() ? c : fallback;
}

// Apply a node's classDef overrides (fill/stroke/color) onto a base style.
Style styleForNode(const DiagramNode &node, const DiagramModel &model, const Style &base)
{
    Style style = base;
    if (node.classRef.isEmpty()) {
        return style;
    }
    const auto it = model.classDefs.constFind(node.classRef);
    if (it == model.classDefs.constEnd()) {
        return style;
    }
    const QHash<QString, QString> &props = it.value();
    style.nodeFill = parseColor(props.value(QStringLiteral("fill")), style.nodeFill);
    style.nodeStroke = parseColor(props.value(QStringLiteral("stroke")), style.nodeStroke);
    style.textColor = parseColor(props.value(QStringLiteral("color")), style.textColor);
    return style;
}

void sizeNode(DiagramNode &node, const TextMeasurer &measurer, const Style &style, qreal labelWrap)
{
    const TextMetrics m = measurer.measure(node.label, style.font, labelWrap);
    qreal w = m.width + 2 * kPadX;
    qreal h = m.height + 2 * kPadY;

    switch (node.shape) {
    case NodeShape::Circle:
        w = h = std::max({w, h, kMinH * 1.4});
        break;
    case NodeShape::Diamond:
        w = std::max(w * 1.4, kMinW * 1.6);
        h = std::max(h * 1.5, kMinH * 1.4);
        break;
    case NodeShape::Hexagon:
        w += h * 0.5;
        break;
    case NodeShape::Stadium:
        w += h * 0.4;
        break;
    case NodeShape::Cylinder:
        h += 12.0;
        break;
    default:
        break;
    }
    node.width = std::max(w, kMinW);
    node.height = std::max(h, kMinH);
}

QColor withAlpha(QColor c, int a)
{
    c.setAlpha(a);
    return c;
}

} // namespace

DiagramModel DiagramEngine::buildModel(const QString &source, const Style &style, qreal maxWidth) const
{
    DiagramModel model = parseFlowchart(source);
    if (!model.valid) {
        return model;
    }
    const qreal labelWrap = std::clamp(maxWidth * 0.5, 120.0, 240.0);
    for (DiagramNode &node : model.nodes) {
        sizeNode(node, m_measurer, styleForNode(node, model, style), labelWrap);
    }
    // Measure edge labels so the layout can reserve space via label dummies.
    for (DiagramEdge &edge : model.edges) {
        if (edge.label.isEmpty()) {
            continue;
        }
        const TextMetrics tm = m_measurer.measure(edge.label, style.font);
        edge.labelWidth = tm.width + 8.0;
        edge.labelHeight = tm.height + 4.0;
    }
    layoutFlowchart(model);
    return model;
}

RenderSnapshotPtr DiagramEngine::buildSnapshot(const QString &source, const Style &style,
                                               qreal maxWidth, quint64 revision) const
{
    auto snap = std::make_shared<RenderSnapshot>();
    snap->contentRevision = revision;

    const QString family = detectFamily(source);
    if (family != QStringLiteral("flowchart")) {
        snap->hasError = true;
        snap->errorText = family.isEmpty()
            ? QStringLiteral("Unsupported or empty diagram")
            : QStringLiteral("Unsupported diagram: %1").arg(family);
        return snap;
    }

    DiagramModel model = buildModel(source, style, maxWidth);
    if (!model.valid) {
        snap->hasError = true;
        snap->errorText = model.error.isEmpty() ? QStringLiteral("Parse error") : model.error;
        return snap;
    }

    const qreal labelWrap = std::clamp(maxWidth * 0.5, 120.0, 240.0);

    // Subgraph cluster boxes (behind everything).
    for (const DiagramCluster &cluster : model.clusters) {
        if (!cluster.bounds.isValid()) {
            continue;
        }
        QPainterPath box;
        box.addRoundedRect(cluster.bounds, 6, 6);
        addFilledPath(box, style.clusterFill, snap->clusters);
        addStrokedPath(box, 1.0, style.clusterStroke, snap->clusters);
        if (!cluster.title.isEmpty()) {
            auto layout = m_measurer.layout(cluster.title, style.font);
            LabelRun run;
            run.layout = layout;
            run.color = style.textColor;
            run.origin = QPointF(cluster.bounds.left() + 8.0, cluster.bounds.top() + 4.0);
            run.bounds = QRectF(run.origin, QSizeF(cluster.bounds.width(), 18.0));
            snap->labels.push_back(run);
        }
    }

    // Edges first (under nodes), then arrowheads.
    for (const DiagramEdge &edge : model.edges) {
        if (edge.points.size() < 2) {
            continue;
        }
        const QColor color = style.edgeStroke;
        const qreal width = edge.kind == EdgeKind::Thick ? style.edgeStrokeWidth * 2.2
                                                         : style.edgeStrokeWidth;
        const qreal dash = edge.kind == EdgeKind::Dotted ? 6.0 : 0.0;

        // Densify into the same curveBasis spline the renderer draws so the
        // arrowhead tangent matches the curve, not the raw last knot.
        QVector<QPointF> dense = curvedPath(edge.points);
        if (dense.size() < 2) {
            dense = edge.points;
        }
        const QPointF tip = dense.last();
        QPointF dir = tip - dense[dense.size() - 2];
        const qreal dl = std::hypot(dir.x(), dir.y());
        if (edge.head == ArrowHead::Arrow && dl > 1e-6) {
            // Pull the curve back along its tangent so it does not poke through
            // the arrowhead.
            const QPointF unit = dir / dl;
            const qreal pull = kArrow * 0.9;
            qreal remaining = pull;
            while (dense.size() > 2 && remaining > 0.0) {
                const QPointF last = dense.last();
                const QPointF prev = dense[dense.size() - 2];
                const qreal seg = std::hypot(last.x() - prev.x(), last.y() - prev.y());
                if (seg <= remaining) {
                    dense.removeLast();
                    remaining -= seg;
                } else {
                    break;
                }
            }
            dense.last() = tip - unit * pull;
        }
        addThickPolyline(dense, width, color, snap->edges, dash);
        if (edge.head == ArrowHead::Arrow) {
            addArrowhead(tip, dir, kArrow, color, snap->arrowheads);
        }

        if (!edge.label.isEmpty()) {
            const TextMetrics tm = m_measurer.measure(edge.label, style.font);
            const QRectF bg(edge.labelPos.x() - tm.width / 2.0 - 3.0,
                            edge.labelPos.y() - tm.height / 2.0 - 1.0,
                            tm.width + 6.0, tm.height + 2.0);
            QPainterPath bgPath;
            bgPath.addRect(bg);
            addFilledPath(bgPath, withAlpha(style.labelBackground, 235), snap->nodeFills);

            auto layout = m_measurer.layout(edge.label, style.font);
            LabelRun run;
            run.layout = layout;
            run.color = style.textColor;
            run.origin = QPointF(edge.labelPos.x() - tm.width / 2.0,
                                 edge.labelPos.y() - tm.height / 2.0);
            run.bounds = bg;
            snap->labels.push_back(run);
        }
    }

    // Nodes: a full shape in the stroke color (nodeBorders, drawn first) with an
    // inset shape in the fill color on top (nodeFills) yields a clean, uniform
    // border for every shape, including sharp-cornered diamonds/hexagons where a
    // mitred path stroke would self-intersect.
    for (const DiagramNode &node : model.nodes) {
        const Style ns = styleForNode(node, model, style);
        const QRectF rect(node.x - node.width / 2.0, node.y - node.height / 2.0,
                          node.width, node.height);
        const qreal sw = ns.nodeStrokeWidth;
        const QRectF inset = rect.adjusted(sw, sw, -sw, -sw);
        addFilledPath(shapeOutline(node.shape, rect), ns.nodeStroke, snap->nodeBorders);
        addFilledPath(shapeOutline(node.shape, inset), ns.nodeFill, snap->nodeFills);

        // Lay the label out (and centre it) within the node's content box so the
        // text wrap width matches the box that determined the node's size.
        const qreal textBox = qMax(8.0, node.width - 2 * kPadX);
        auto layout = m_measurer.layout(node.label, ns.font, textBox);
        const qreal th = layout->boundingRect().height();
        LabelRun run;
        run.layout = layout;
        run.color = ns.textColor;
        run.origin = QPointF(node.x - textBox / 2.0, node.y - th / 2.0);
        run.bounds = rect;
        snap->labels.push_back(run);

        snap->hits.push_back(HitShape{rect, node.id, HitKind::Node});
    }

    // Edge hit shapes (bounding box over the full polyline incl. bends).
    for (const DiagramEdge &edge : model.edges) {
        if (edge.points.size() < 2) {
            continue;
        }
        QRectF box(edge.points.first(), QSizeF(0.0, 0.0));
        for (const QPointF &p : edge.points) {
            box = box.united(QRectF(p, QSizeF(0.0, 0.0)));
        }
        box = box.normalized().adjusted(-4, -4, 4, 4);
        snap->hits.push_back(HitShape{box, edge.fromId + QStringLiteral("->") + edge.toId,
                                      HitKind::Edge});
    }

    QRectF bounds;
    for (const Mesh *m : {&snap->clusters, &snap->edges, &snap->arrowheads, &snap->nodeFills,
                          &snap->nodeBorders}) {
        if (!m->bounds.isNull()) {
            bounds = bounds.isNull() ? m->bounds : bounds.united(m->bounds);
        }
    }
    bounds.adjust(-6, -6, 6, 6);
    snap->diagramBounds = bounds;
    return snap;
}

} // namespace be::diagram
