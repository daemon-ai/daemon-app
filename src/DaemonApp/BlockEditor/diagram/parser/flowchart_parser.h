// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "diagram/ir/diagram_model.h"

#include <QString>

namespace be::diagram {

// Hand-written flowchart parser. Accepts the common Mermaid flowchart/graph
// subset: directions, node shapes, edge kinds with optional labels, subgraphs,
// and classDef/class styling. Returns a DiagramModel with valid=false and an
// error message on failure.
DiagramModel parseFlowchart(const QString& source);

} // namespace be::diagram
