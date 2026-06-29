// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "diagram/ir/diagram_model.h"

#include <QPainterPath>
#include <QRectF>

namespace be::diagram {

// Build the outline of a node shape filling the given rect (diagram space).
QPainterPath shapeOutline(NodeShape shape, const QRectF& rect);

// Intersection of the segment from the rect center toward `toward` with the
// shape boundary. Analytic for ellipses, polygon ray-cast for diamond/hexagon,
// rectangle clip otherwise. Used to terminate edges on the node border.
QPointF shapeBoundaryPoint(NodeShape shape, const QRectF& rect, const QPointF& toward);

} // namespace be::diagram
