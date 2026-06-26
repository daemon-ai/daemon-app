#include "diagram/geometry/path_mesh.h"

#include <array>
#include <cmath>
#include <mapbox/earcut.hpp>
#include <QPainterPathStroker>
#include <util/numeric.h>
#include <utility>
#include <vector>

// Teach earcut how to read a QPointF (specialize after the primary template).

namespace mapbox::util {
template <>
struct nth<0, QPointF> {
    static double get(const QPointF& p) { return p.x(); }
};
template <>
struct nth<1, QPointF> {
    static double get(const QPointF& p) { return p.y(); }
};
} // namespace mapbox::util

namespace be::diagram {

namespace {

Vertex makeVertex(const QPointF& p, const QColor& c) {
    Vertex v;
    v.x = static_cast<float>(p.x());
    v.y = static_cast<float>(p.y());
    v.r = static_cast<unsigned char>(c.red());
    v.g = static_cast<unsigned char>(c.green());
    v.b = static_cast<unsigned char>(c.blue());
    v.a = static_cast<unsigned char>(c.alpha());
    return v;
}

void growBounds(Mesh& mesh, const QPointF& p) {
    if (mesh.bounds.isNull()) {
        mesh.bounds = QRectF(p, QSizeF(0.0001, 0.0001));
    } else {
        if (p.x() < mesh.bounds.left()) {
            mesh.bounds.setLeft(p.x());
        }
        if (p.x() > mesh.bounds.right()) {
            mesh.bounds.setRight(p.x());
        }
        if (p.y() < mesh.bounds.top()) {
            mesh.bounds.setTop(p.y());
        }
        if (p.y() > mesh.bounds.bottom()) {
            mesh.bounds.setBottom(p.y());
        }
    }
}

double polygonArea(const std::vector<QPointF>& ring) {
    double area = 0.0;
    for (size_t i = 0, n = ring.size(); i < n; ++i) {
        const QPointF& a = ring[i];
        const QPointF& b = ring[(i + 1) % n];
        area += (a.x() * b.y()) - (b.x() * a.y());
    }
    return area * 0.5;
}

std::vector<QPointF> toRing(const QPolygonF& poly) {
    std::vector<QPointF> ring;
    ring.reserve(poly.size());
    for (const QPointF& p : poly) {
        ring.push_back(p);
    }
    // Drop a duplicated closing vertex.
    if (ring.size() >= 2 && ring.front() == ring.back()) {
        ring.pop_back();
    }
    return ring;
}

// Triangulate a single polygon (outer ring + optional holes) into mesh.
void triangulate(const std::vector<std::vector<QPointF>>& polygon, const QColor& color,
                 Mesh& mesh) {
    size_t total = 0;
    for (const auto& ring : polygon) {
        total += ring.size();
    }
    if (total < 3) {
        return;
    }
    const std::vector<uint32_t> indices = mapbox::earcut<uint32_t>(polygon);
    const int base = daemon_app::to_int(mesh.vertices.size());
    for (const auto& ring : polygon) {
        for (const QPointF& p : ring) {
            mesh.vertices.push_back(makeVertex(p, color));
            growBounds(mesh, p);
        }
    }
    mesh.indices.reserve(mesh.indices.size() + static_cast<qsizetype>(indices.size()));
    for (uint32_t i : indices) {
        mesh.indices.push_back(static_cast<quint16>(base + i));
    }
}

void addQuad(const QPointF& a, const QPointF& b, const QPointF& c, const QPointF& d,
             const QColor& color, Mesh& mesh) {
    const int base = daemon_app::to_int(mesh.vertices.size());
    for (const QPointF& p : {a, b, c, d}) {
        mesh.vertices.push_back(makeVertex(p, color));
        growBounds(mesh, p);
    }
    const auto i0 = static_cast<quint16>(base);
    mesh.indices.push_back(i0);
    mesh.indices.push_back(i0 + 1);
    mesh.indices.push_back(i0 + 2);
    mesh.indices.push_back(i0);
    mesh.indices.push_back(i0 + 2);
    mesh.indices.push_back(i0 + 3);
}

void addSegment(const QPointF& p0, const QPointF& p1, qreal halfW, const QColor& color,
                Mesh& mesh) {
    QPointF d = p1 - p0;
    const qreal len = std::hypot(d.x(), d.y());
    if (len < 1e-6) {
        return;
    }
    d /= len;
    const QPointF n(-d.y() * halfW, d.x() * halfW);
    addQuad(p0 + n, p1 + n, p1 - n, p0 - n, color, mesh);
}

} // namespace

void addFilledPath(const QPainterPath& path, const QColor& color, Mesh& mesh) {
    const QList<QPolygonF> polys = path.toFillPolygons();
    for (const QPolygonF& poly : polys) {
        std::vector<std::vector<QPointF>> polygon;
        polygon.push_back(toRing(poly));
        triangulate(polygon, color, mesh);
    }
}

void addStrokedPath(const QPainterPath& path, qreal width, const QColor& color, Mesh& mesh) {
    QPainterPathStroker stroker;
    stroker.setWidth(width);
    stroker.setJoinStyle(Qt::MiterJoin);
    stroker.setCapStyle(Qt::FlatCap);
    const QPainterPath outline = stroker.createStroke(path);
    const QList<QPolygonF> polys = outline.toFillPolygons();
    if (polys.isEmpty()) {
        return;
    }

    // Nest contours: the largest |area| polygon is the outer ring, the rest are
    // holes. Correct for a single stroked closed shape (outer + inner contour).
    std::vector<std::vector<QPointF>> rings;
    rings.reserve(polys.size());
    int outer = 0;
    double bestArea = -1.0;
    for (int i = 0; i < polys.size(); ++i) {
        rings.push_back(toRing(polys[i]));
        const double area = std::abs(polygonArea(rings.back()));
        if (area > bestArea) {
            bestArea = area;
            outer = i;
        }
    }
    std::vector<std::vector<QPointF>> polygon;
    polygon.push_back(rings[outer]);
    for (int i = 0; std::cmp_less(i, rings.size()); ++i) {
        if (i != outer) {
            polygon.push_back(rings[i]);
        }
    }
    triangulate(polygon, color, mesh);
}

void addThickPolyline(const QVector<QPointF>& points, qreal width, const QColor& color, Mesh& mesh,
                      qreal dash) {
    if (points.size() < 2) {
        return;
    }
    const qreal halfW = width / 2.0;
    // Dash phase is carried across segments so a densely sampled curve still
    // shows gaps (each segment may be much shorter than the dash length).
    qreal phase = 0.0; // distance into the current on/off cell
    bool on = true;
    for (int i = 1; i < points.size(); ++i) {
        const QPointF a = points[i - 1];
        const QPointF b = points[i];
        if (dash <= 0.0) {
            addSegment(a, b, halfW, color, mesh);
            continue;
        }
        QPointF d = b - a;
        const qreal len = std::hypot(d.x(), d.y());
        if (len < 1e-6) {
            continue;
        }
        d /= len;
        qreal t = 0.0;
        while (t < len) {
            const qreal step = qMin(dash - phase, len - t);
            if (on) {
                addSegment(a + d * t, a + d * (t + step), halfW, color, mesh);
            }
            t += step;
            phase += step;
            if (phase >= dash - 1e-9) {
                phase = 0.0;
                on = !on;
            }
        }
    }
}

namespace {

QPointF cubicBezier(const QPointF& p0, const QPointF& p1, const QPointF& p2, const QPointF& p3,
                    qreal t) {
    const qreal u = 1.0 - t;
    const qreal a = u * u * u;
    const qreal b = 3.0 * u * u * t;
    const qreal c = 3.0 * u * t * t;
    const qreal d = t * t * t;
    return {(a * p0.x()) + (b * p1.x()) + (c * p2.x()) + (d * p3.x()),
            (a * p0.y()) + (b * p1.y()) + (c * p2.y()) + (d * p3.y())};
}

// Round near-orthogonal corners by replacing the corner knot with two knots a
// small radius back along each incident segment (mermaid's fixCorners).
QVector<QPointF> fixCorners(const QVector<QPointF>& knots) {
    if (knots.size() < 3) {
        return knots;
    }
    QVector<QPointF> out;
    out.push_back(knots.first());
    for (int i = 1; i + 1 < knots.size(); ++i) {
        const QPointF a = knots[i - 1];
        const QPointF b = knots[i];
        const QPointF c = knots[i + 1];
        QPointF din = b - a;
        QPointF dout = c - b;
        const qreal lin = std::hypot(din.x(), din.y());
        const qreal lout = std::hypot(dout.x(), dout.y());
        if (lin < 1e-6 || lout < 1e-6) {
            out.push_back(b);
            continue;
        }
        din /= lin;
        dout /= lout;
        const qreal cosang = (din.x() * dout.x()) + (din.y() * dout.y());
        if (cosang > 0.96) { // nearly collinear: keep the knot
            out.push_back(b);
            continue;
        }
        const qreal r = std::min({5.0, lin / 3.0, lout / 3.0});
        out.push_back(b - din * r);
        out.push_back(b + dout * r);
    }
    out.push_back(knots.last());
    return out;
}

} // namespace

QVector<QPointF> sampleCurveBasis(const QVector<QPointF>& knots) {
    QVector<QPointF> out;
    const int n = daemon_app::to_int(knots.size());
    if (n == 0) {
        return out;
    }
    if (n <= 2) {
        return knots;
    }

    constexpr int kSub = 16;
    QPointF cur;
    const auto moveTo = [&](qreal x, qreal y) {
        cur = QPointF(x, y);
        out.push_back(cur);
    };
    const auto lineTo = [&](qreal x, qreal y) {
        cur = QPointF(x, y);
        out.push_back(cur);
    };

    qreal x0 = 0;
    qreal y0 = 0;
    qreal x1 = 0;
    qreal y1 = 0;
    const auto basisPoint = [&](qreal x, qreal y) {
        const QPointF c1((2 * x0 + x1) / 3.0, (2 * y0 + y1) / 3.0);
        const QPointF c2((x0 + 2 * x1) / 3.0, (y0 + 2 * y1) / 3.0);
        const QPointF e((x0 + 4 * x1 + x) / 6.0, (y0 + 4 * y1 + y) / 6.0);
        const QPointF start = cur;
        for (int i = 1; i <= kSub; ++i) {
            out.push_back(cubicBezier(start, c1, c2, e, qreal(i) / kSub));
        }
        cur = e;
    };

    int point = 0;
    for (const QPointF& k : knots) {
        const qreal x = k.x();
        const qreal y = k.y();
        switch (point) {
        case 0:
            point = 1;
            moveTo(x, y);
            break;
        case 1:
            point = 2;
            break;
        case 2:
            point = 3;
            lineTo((5 * x0 + x1) / 6.0, (5 * y0 + y1) / 6.0);
            basisPoint(x, y);
            break;
        default:
            basisPoint(x, y);
            break;
        }
        x0 = x1;
        y0 = y1;
        x1 = x;
        y1 = y;
    }
    if (point == 3) {
        basisPoint(x1, y1);
        lineTo(x1, y1);
    } else if (point == 2) {
        lineTo(x1, y1);
    }
    return out;
}

QVector<QPointF> curvedPath(const QVector<QPointF>& knots) {
    return sampleCurveBasis(fixCorners(knots));
}

void addCurvedPolyline(const QVector<QPointF>& knots, qreal width, const QColor& color, Mesh& mesh,
                       qreal dash) {
    addThickPolyline(curvedPath(knots), width, color, mesh, dash);
}

void addArrowhead(const QPointF& tip, const QPointF& dir, qreal size, const QColor& color,
                  Mesh& mesh) {
    QPointF d = dir;
    const qreal len = std::hypot(d.x(), d.y());
    if (len < 1e-6) {
        return;
    }
    d /= len;
    const QPointF n(-d.y(), d.x());
    const QPointF base = tip - d * size;
    const QPointF left = base + n * (size * 0.5);
    const QPointF right = base - n * (size * 0.5);
    const int b = daemon_app::to_int(mesh.vertices.size());
    for (const QPointF& p : {tip, left, right}) {
        mesh.vertices.push_back(makeVertex(p, color));
        growBounds(mesh, p);
    }
    mesh.indices.push_back(static_cast<quint16>(b));
    mesh.indices.push_back(static_cast<quint16>(b + 1));
    mesh.indices.push_back(static_cast<quint16>(b + 2));
}

} // namespace be::diagram
