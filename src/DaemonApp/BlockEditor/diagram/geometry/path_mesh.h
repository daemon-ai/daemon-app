#pragma once

#include "diagram/snapshot/render_snapshot.h"

#include <QColor>
#include <QPainterPath>
#include <QPointF>
#include <QVector>

namespace be::diagram {

// Triangulate a filled path (no interior holes assumed) into the mesh.
void addFilledPath(const QPainterPath& path, const QColor& color, Mesh& mesh);

// Stroke a (closed) path outline of the given width and triangulate the ring,
// correctly excluding the interior via earcut holes.
void addStrokedPath(const QPainterPath& path, qreal width, const QColor& color, Mesh& mesh);

// Triangulate an open polyline as a thick line (quad per segment). When dash > 0
// the line is broken into dashes of that length.
void addThickPolyline(const QVector<QPointF>& points, qreal width, const QColor& color, Mesh& mesh,
                      qreal dash = 0.0);

// Evaluate a uniform cubic B-spline (D3 curveBasis) through `knots`. Returns the
// densified polyline; the first and last knots are reached exactly (matching D3).
QVector<QPointF> sampleCurveBasis(const QVector<QPointF>& knots);

// fixCorners + sampleCurveBasis: the exact polyline the renderer strokes for an
// edge. Exposed so the engine can derive the arrowhead tangent from the curve.
QVector<QPointF> curvedPath(const QVector<QPointF>& knots);

// Stroke a curveBasis curve through `knots` as a thick polyline (dashed if
// dash > 0). Thin wrapper over sampleCurveBasis + addThickPolyline.
void addCurvedPolyline(const QVector<QPointF>& knots, qreal width, const QColor& color, Mesh& mesh,
                       qreal dash = 0.0);

// Append a filled arrowhead triangle at tip pointing along dir (unit not required).
void addArrowhead(const QPointF& tip, const QPointF& dir, qreal size, const QColor& color,
                  Mesh& mesh);

} // namespace be::diagram
