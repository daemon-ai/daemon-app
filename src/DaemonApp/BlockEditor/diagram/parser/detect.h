#pragma once

#include <QString>

namespace be::diagram {

// Returns the diagram family id from the leading keyword ("flowchart" for
// flowchart/graph), or an empty string if unrecognized.
QString detectFamily(const QString& source);

} // namespace be::diagram
