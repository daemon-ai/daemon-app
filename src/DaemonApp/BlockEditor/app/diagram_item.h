// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "app/diagram_controller.h"

#include <QPointer>
#include <QQuickItem>
#include <QtQmlIntegration/qqmlintegration.h>

namespace be::app {

// Renders a DiagramController's RenderSnapshot with batched QSGGeometryNodes plus
// QSGTextNodes. Geometry is rebuilt only when the snapshot revision changes; pan,
// zoom, and hover/selection only touch the camera transform / highlight node, so
// interaction never re-uploads diagram geometry.
class DiagramItem : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(be::app::DiagramController* controller READ controller WRITE setController NOTIFY
                   controllerChanged)
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(qreal panX READ panX WRITE setPanX NOTIFY panChanged)
    Q_PROPERTY(qreal panY READ panY WRITE setPanY NOTIFY panChanged)
    Q_PROPERTY(QString hoverId READ hoverId NOTIFY hoverChanged)
    Q_PROPERTY(QString selectedId READ selectedId WRITE setSelectedId NOTIFY selectionChanged)
    Q_PROPERTY(qreal contentScale READ contentScale NOTIFY layoutChanged)

public:
    explicit DiagramItem(QQuickItem* parent = nullptr);

    DiagramController* controller() const { return m_controller; }
    void setController(DiagramController* controller);

    qreal zoom() const { return m_zoom; }
    void setZoom(qreal zoom);
    qreal panX() const { return m_panX; }
    void setPanX(qreal v);
    qreal panY() const { return m_panY; }
    void setPanY(qreal v);

    QString hoverId() const { return m_hoverId; }
    QString selectedId() const { return m_selectedId; }
    void setSelectedId(const QString& id);

    qreal contentScale() const;

    // Map an item-space point into diagram space (accounts for fit/zoom/pan).
    Q_INVOKABLE QPointF toDiagram(const QPointF& itemPoint) const;
    // Hit-test an item-space point; returns the node/edge id or "" .
    Q_INVOKABLE QString hitTest(const QPointF& itemPoint) const;
    // Convenience for QML wheel handlers: zoom around an item-space focus point.
    Q_INVOKABLE void zoomAt(qreal factor, const QPointF& itemPoint);
    Q_INVOKABLE void setHoverAt(const QPointF& itemPoint);
    Q_INVOKABLE void resetView();

signals:
    void controllerChanged();
    void zoomChanged();
    void panChanged();
    void hoverChanged();
    void selectionChanged();
    void layoutChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

private:
    qreal fitScale() const;
    void onSnapshotChanged();
    void updateImplicitSize();

    QPointer<DiagramController> m_controller;
    qreal m_zoom = 1.0;
    qreal m_panX = 0.0;
    qreal m_panY = 0.0;
    QString m_hoverId;
    QString m_selectedId;

    // Render-thread bookkeeping.
    be::diagram::RenderSnapshotPtr m_renderSnapshot;
    quint64 m_builtRevision = ~quint64(0);
    int m_lodTier = -1;
    bool m_geometryDirty = true;
    bool m_highlightDirty = true;
};

} // namespace be::app
