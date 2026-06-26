#pragma once

#include "diagram/ir/diagram_model.h"
#include "diagram/snapshot/render_snapshot.h"
#include "diagram/text/text_measurer.h"

#include <QString>

namespace be::diagram {

// Orchestrates the pipeline: detect -> parse -> measure -> layout -> geometry ->
// snapshot. Pure (no Qt Quick / GUI thread dependency) so it can run on a worker
// thread. Returns an error snapshot (hasError=true) when the source is invalid.
class DiagramEngine {
public:
    RenderSnapshotPtr buildSnapshot(const QString& source, const Style& style, qreal maxWidth,
                                    quint64 revision) const;

    // Exposed for tests: parse + layout without geometry.
    DiagramModel buildModel(const QString& source, const Style& style, qreal maxWidth) const;

private:
    TextMeasurer m_measurer;
};

} // namespace be::diagram
