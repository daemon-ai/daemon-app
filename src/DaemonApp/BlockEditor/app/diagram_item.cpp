#include "app/diagram_item.h"

#include "diagram/geometry/path_mesh.h"

#include <cstring>
#include <QMatrix4x4>
#include <QPainterPath>
#include <QQuickWindow>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGTextNode>
#include <QSGTransformNode>
#include <QSGVertexColorMaterial>
#include <util/numeric.h>

namespace be::app {

using be::diagram::Mesh;
using be::diagram::RenderSnapshot;
using be::diagram::Vertex;

namespace {

constexpr qreal kPad = 6.0;

QSGGeometryNode* buildMeshNode(const Mesh& mesh) {
    if (mesh.isEmpty()) {
        return nullptr;
    }
    auto* node = new QSGGeometryNode;
    auto* geometry = new QSGGeometry(
        QSGGeometry::defaultAttributes_ColoredPoint2D(), daemon_app::to_int(mesh.vertices.size()),
        daemon_app::to_int(mesh.indices.size()), QSGGeometry::UnsignedShortType);
    geometry->setDrawingMode(QSGGeometry::DrawTriangles);
    std::memcpy(geometry->vertexData(), mesh.vertices.constData(),
                size_t(mesh.vertices.size()) * sizeof(Vertex));
    std::memcpy(geometry->indexDataAsUShort(), mesh.indices.constData(),
                size_t(mesh.indices.size()) * sizeof(quint16));
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry);
    auto* material = new QSGVertexColorMaterial;
    node->setMaterial(material);
    node->setFlag(QSGNode::OwnsMaterial);
    return node;
}

void clearChildren(QSGNode* node) {
    while (QSGNode* child = node->firstChild()) {
        node->removeChildNode(child);
        delete child;
    }
}

} // namespace

DiagramItem::DiagramItem(QQuickItem* parent) : QQuickItem(parent) {
    setFlag(ItemHasContents, true);
}

void DiagramItem::setController(DiagramController* controller) {
    if (m_controller == controller) {
        return;
    }
    if (m_controller) {
        disconnect(m_controller, nullptr, this, nullptr);
    }
    m_controller = controller;
    if (m_controller) {
        connect(m_controller, &DiagramController::snapshotChanged, this,
                &DiagramItem::onSnapshotChanged);
    }
    emit controllerChanged();
    onSnapshotChanged();
}

void DiagramItem::setZoom(qreal zoom) {
    zoom = qBound(0.2, zoom, 6.0);
    if (qFuzzyCompare(m_zoom, zoom)) {
        return;
    }
    m_zoom = zoom;
    emit zoomChanged();
    update();
}

void DiagramItem::setPanX(qreal v) {
    if (qFuzzyCompare(m_panX, v)) {
        return;
    }
    m_panX = v;
    emit panChanged();
    update();
}

void DiagramItem::setPanY(qreal v) {
    if (qFuzzyCompare(m_panY, v)) {
        return;
    }
    m_panY = v;
    emit panChanged();
    update();
}

void DiagramItem::setSelectedId(const QString& id) {
    if (m_selectedId == id) {
        return;
    }
    m_selectedId = id;
    m_highlightDirty = true;
    emit selectionChanged();
    update();
}

qreal DiagramItem::contentScale() const {
    return fitScale() * m_zoom;
}

qreal DiagramItem::fitScale() const {
    if (!m_controller) {
        return 1.0;
    }
    const qreal w = m_controller->diagramWidth();
    if (w <= 0.0 || width() <= 0.0) {
        return 1.0;
    }
    const qreal avail = width() - (2 * kPad);
    return avail < w ? avail / w : 1.0;
}

void DiagramItem::onSnapshotChanged() {
    m_geometryDirty = true;
    m_highlightDirty = true;
    updateImplicitSize();
    emit layoutChanged();
    update();
}

void DiagramItem::updateImplicitSize() {
    qreal h = 0.0;
    if (m_controller) {
        h = m_controller->diagramHeight() * fitScale();
    }
    setImplicitHeight(h > 0.0 ? h + (2 * kPad) : 0.0);
}

void DiagramItem::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) {
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.width() != oldGeometry.width()) {
        updateImplicitSize();
        update();
    }
}

QPointF DiagramItem::toDiagram(const QPointF& itemPoint) const {
    if (!m_controller) {
        return itemPoint;
    }
    const auto s = m_controller->snapshot();
    const QRectF bounds = s ? s->diagramBounds : QRectF();
    const qreal scale = contentScale();
    if (scale <= 0.0) {
        return itemPoint;
    }
    const qreal contentW = bounds.width() * scale;
    const qreal offsetX = ((width() - contentW) / 2.0) + m_panX;
    const qreal offsetY = kPad + m_panY;
    return {((itemPoint.x() - offsetX) / scale) + bounds.left(),
            ((itemPoint.y() - offsetY) / scale) + bounds.top()};
}

QString DiagramItem::hitTest(const QPointF& itemPoint) const {
    if (!m_controller) {
        return {};
    }
    const auto snap = m_controller->snapshot();
    if (!snap) {
        return {};
    }
    const QPointF p = toDiagram(itemPoint);
    // Nodes first (top of z-order for picking), then edges.
    for (const auto& hit : snap->hits) {
        if (hit.kind == be::diagram::HitKind::Node && hit.bounds.contains(p)) {
            return hit.id;
        }
    }
    for (const auto& hit : snap->hits) {
        if (hit.kind == be::diagram::HitKind::Edge && hit.bounds.contains(p)) {
            return hit.id;
        }
    }
    return {};
}

void DiagramItem::setHoverAt(const QPointF& itemPoint) {
    const QString id = hitTest(itemPoint);
    if (id == m_hoverId) {
        return;
    }
    m_hoverId = id;
    m_highlightDirty = true;
    emit hoverChanged();
    update();
}

void DiagramItem::zoomAt(qreal factor, const QPointF& itemPoint) {
    const QPointF before = toDiagram(itemPoint);
    setZoom(m_zoom * factor);
    const QPointF after = toDiagram(itemPoint);
    const qreal scale = contentScale();
    // Keep the focus point stationary by compensating with pan.
    m_panX += (after.x() - before.x()) * scale;
    m_panY += (after.y() - before.y()) * scale;
    emit panChanged();
    update();
}

void DiagramItem::resetView() {
    m_zoom = 1.0;
    m_panX = 0.0;
    m_panY = 0.0;
    emit zoomChanged();
    emit panChanged();
    update();
}

QSGNode* DiagramItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
    auto* camera = static_cast<QSGTransformNode*>(oldNode);
    QSGNode* content = nullptr;
    QSGGeometryNode* highlight = nullptr;

    if (!camera) {
        camera = new QSGTransformNode;
        content = new QSGNode;
        content->setFlag(QSGNode::OwnedByParent);
        camera->appendChildNode(content);
        m_geometryDirty = true;
        m_highlightDirty = true;
    } else {
        content = camera->firstChild();
    }

    const auto snap = m_controller ? m_controller->snapshot() : be::diagram::RenderSnapshotPtr();
    m_renderSnapshot = snap;

    // Level-of-detail: drop small labels when zoomed far out. Tier changes are
    // rare, so this rebuilds text at most once per tier crossing (not per frame),
    // keeping pan/zoom otherwise upload-free.
    const qreal scale = contentScale();
    const int tier = scale < 0.55 ? 0 : 1;
    if (tier != m_lodTier) {
        m_lodTier = tier;
        m_geometryDirty = true;
    }

    const quint64 rev = snap ? snap->contentRevision : 0;
    if (m_geometryDirty || rev != m_builtRevision) {
        clearChildren(content);
        if (snap && !snap->hasError) {
            for (const Mesh* mesh : {&snap->clusters, &snap->edges, &snap->arrowheads,
                                     &snap->nodeBorders, &snap->nodeFills}) {
                if (QSGGeometryNode* node = buildMeshNode(*mesh)) {
                    content->appendChildNode(node);
                }
            }
            if (!snap->labels.isEmpty() && window()) {
                for (const auto& label : snap->labels) {
                    if (!label.layout) {
                        continue;
                    }
                    // Hide small labels at the coarse LOD tier.
                    if (m_lodTier == 0 && label.layout->boundingRect().height() < 16.0) {
                        continue;
                    }
                    QSGTextNode* textNode = window()->createTextNode();
                    textNode->setFlag(QSGNode::OwnedByParent);
                    textNode->setColor(label.color);
                    textNode->addTextLayout(label.origin, label.layout.get());
                    content->appendChildNode(textNode);
                }
            }
        }
        m_builtRevision = rev;
        m_geometryDirty = false;
        m_highlightDirty = true;
    }

    // Highlight node (hover/selection outline) lives after content.
    QSGNode* maybeHighlight = content->nextSibling();
    if (m_highlightDirty) {
        if (maybeHighlight) {
            camera->removeChildNode(maybeHighlight);
            delete maybeHighlight;
        }
        if (snap) {
            Mesh hl;
            const auto addOutline = [&](const QString& id, const QColor& color) {
                if (id.isEmpty()) {
                    return;
                }
                for (const auto& hit : snap->hits) {
                    if (hit.id == id) {
                        QPainterPath p;
                        p.addRoundedRect(hit.bounds.adjusted(-1, -1, 1, 1), 4, 4);
                        be::diagram::addStrokedPath(p, 2.0, color, hl);
                    }
                }
            };
            addOutline(m_hoverId, QColor(90, 160, 240, 200));
            addOutline(m_selectedId, QColor(40, 120, 220, 255));
            if (!hl.isEmpty()) {
                highlight = buildMeshNode(hl);
                if (highlight) {
                    highlight->setFlag(QSGNode::OwnedByParent);
                    camera->appendChildNode(highlight);
                }
            }
        }
        m_highlightDirty = false;
    }

    // Camera transform: fit-to-width + zoom about center + pan.
    QMatrix4x4 matrix;
    if (snap) {
        const QRectF bounds = snap->diagramBounds;
        const qreal scale = contentScale();
        const qreal contentW = bounds.width() * scale;
        const qreal offsetX = ((width() - contentW) / 2.0) + m_panX;
        const qreal offsetY = kPad + m_panY;
        matrix.translate(static_cast<float>(offsetX), static_cast<float>(offsetY));
        matrix.scale(static_cast<float>(scale));
        matrix.translate(static_cast<float>(-bounds.left()), static_cast<float>(-bounds.top()));
    }
    camera->setMatrix(matrix);

    return camera;
}

} // namespace be::app
