#pragma once

#include "diagram/ir/diagram_model.h"

namespace be::diagram {

// Spacing parameters for the layout (diagram-space pixels), matching mermaid's
// dagre defaults so output spacing is comparable.
struct LayoutOptions {
    qreal rankSep = 50.0;   // gap between ranks (along flow direction)
    qreal nodeSep = 50.0;   // gap between sibling real nodes within a rank
    qreal edgeSep = 20.0;   // gap between dummy (edge) nodes within a rank
    qreal clusterPad = 8.0; // padding around subgraph members
    qreal clusterTitle = 22.0;
    qreal labelOffset = 10.0; // edge-label offset from the edge midpoint
    qreal margin = 8.0;       // outer margin around the whole diagram
};

// Assigns node centers (x, y; width/height already set by the caller from text
// measurement), edge polylines (with bend points), cluster bounds, and returns
// the overall bounds. A pragmatic dagre subset: longest-path+tighten ranking,
// dummy-node insertion for long edges, barycenter ordering, Brandes-Koepf
// x-assignment, and cluster-aware compound layout.
QRectF layoutFlowchart(DiagramModel& model, const LayoutOptions& opts = {});

} // namespace be::diagram
