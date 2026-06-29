// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "diagram/geometry/shapes.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace be::diagram {

namespace {

// Intersection of segment center->toward with the rectangle boundary.
QPointF rectBoundary(const QRectF& rect, const QPointF& toward) {
    const QPointF c = rect.center();
    QPointF d = toward - c;
    if (qFuzzyIsNull(d.x()) && qFuzzyIsNull(d.y())) {
        return c;
    }
    const qreal hw = rect.width() / 2.0;
    const qreal hh = rect.height() / 2.0;
    const qreal sx =
        qFuzzyIsNull(d.x()) ? std::numeric_limits<qreal>::infinity() : hw / std::abs(d.x());
    const qreal sy =
        qFuzzyIsNull(d.y()) ? std::numeric_limits<qreal>::infinity() : hh / std::abs(d.y());
    return c + d * std::min(sx, sy);
}

// Closest intersection of the ray center->toward with a closed polygon.
QPointF polyBoundary(const QList<QPointF>& poly, const QRectF& rect, const QPointF& toward) {
    const QPointF c = rect.center();
    const QPointF d = toward - c;
    qreal best = std::numeric_limits<qreal>::infinity();
    QPointF hit = rectBoundary(rect, toward);
    for (int i = 0; i < poly.size(); ++i) {
        const QPointF a = poly[i];
        const QPointF b = poly[(i + 1) % poly.size()];
        const QPointF e = b - a;
        // Solve c + t*d = a + s*e, t>=0, s in [0,1].
        const qreal denom = (d.x() * e.y()) - (d.y() * e.x());
        if (qFuzzyIsNull(denom)) {
            continue;
        }
        const QPointF ac = a - c;
        const qreal t = (ac.x() * e.y() - ac.y() * e.x()) / denom;
        const qreal s = (ac.x() * d.y() - ac.y() * d.x()) / denom;
        if (t >= 0.0 && s >= -1e-6 && s <= 1.0 + 1e-6 && t < best) {
            best = t;
            hit = c + d * t;
        }
    }
    return hit;
}

} // namespace

QPainterPath shapeOutline(NodeShape shape, const QRectF& rect) {
    QPainterPath path;
    const qreal w = rect.width();
    const qreal h = rect.height();

    switch (shape) {
    case NodeShape::Rect:
    case NodeShape::Subroutine:
        path.addRect(rect);
        break;
    case NodeShape::RoundRect: {
        const qreal r = std::min<qreal>(10.0, h / 2.0);
        path.addRoundedRect(rect, r, r);
        break;
    }
    case NodeShape::Stadium: {
        const qreal r = h / 2.0;
        path.addRoundedRect(rect, r, r);
        break;
    }
    case NodeShape::Circle:
        path.addEllipse(rect);
        break;
    case NodeShape::Cylinder: {
        const qreal ry = std::min<qreal>(h * 0.18, 12.0);
        const qreal top = rect.top();
        const qreal bottom = rect.bottom();
        path.moveTo(rect.left(), top + ry);
        path.arcTo(QRectF(rect.left(), top, w, 2 * ry), 180, -180);
        path.lineTo(rect.right(), bottom - ry);
        path.arcTo(QRectF(rect.left(), bottom - (2 * ry), w, 2 * ry), 0, -180);
        path.closeSubpath();
        break;
    }
    case NodeShape::Diamond: {
        const QPointF c = rect.center();
        path.moveTo(c.x(), rect.top());
        path.lineTo(rect.right(), c.y());
        path.lineTo(c.x(), rect.bottom());
        path.lineTo(rect.left(), c.y());
        path.closeSubpath();
        break;
    }
    case NodeShape::Hexagon: {
        const qreal inset = std::min<qreal>(w * 0.18, h);
        const qreal midY = rect.center().y();
        path.moveTo(rect.left() + inset, rect.top());
        path.lineTo(rect.right() - inset, rect.top());
        path.lineTo(rect.right(), midY);
        path.lineTo(rect.right() - inset, rect.bottom());
        path.lineTo(rect.left() + inset, rect.bottom());
        path.lineTo(rect.left(), midY);
        path.closeSubpath();
        break;
    }
    }
    return path;
}

QPointF shapeBoundaryPoint(NodeShape shape, const QRectF& rect, const QPointF& toward) {
    const QPointF c = rect.center();
    QPointF d = toward - c;
    if (qFuzzyIsNull(d.x()) && qFuzzyIsNull(d.y())) {
        return c;
    }

    switch (shape) {
    case NodeShape::Circle: {
        // Ellipse intersection: scale to unit circle, intersect, scale back.
        const qreal a = rect.width() / 2.0;
        const qreal b = rect.height() / 2.0;
        if (a <= 0.0 || b <= 0.0) {
            return c;
        }
        const qreal k = 1.0 / std::sqrt(((d.x() * d.x()) / (a * a)) + ((d.y() * d.y()) / (b * b)));
        return c + d * k;
    }
    case NodeShape::Diamond: {
        const QList<QPointF> poly{QPointF(c.x(), rect.top()), QPointF(rect.right(), c.y()),
                                  QPointF(c.x(), rect.bottom()), QPointF(rect.left(), c.y())};
        return polyBoundary(poly, rect, toward);
    }
    case NodeShape::Hexagon: {
        const qreal inset = std::min<qreal>(rect.width() * 0.18, rect.height());
        const qreal midY = c.y();
        const QList<QPointF> poly{QPointF(rect.left() + inset, rect.top()),
                                  QPointF(rect.right() - inset, rect.top()),
                                  QPointF(rect.right(), midY),
                                  QPointF(rect.right() - inset, rect.bottom()),
                                  QPointF(rect.left() + inset, rect.bottom()),
                                  QPointF(rect.left(), midY)};
        return polyBoundary(poly, rect, toward);
    }
    default:
        return rectBoundary(rect, toward);
    }
}

} // namespace be::diagram
