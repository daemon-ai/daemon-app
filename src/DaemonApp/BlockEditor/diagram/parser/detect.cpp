#include "diagram/parser/detect.h"

#include <QStringList>

namespace be::diagram {

QString detectFamily(const QString& source) {
    for (const QString& raw : source.split(QLatin1Char('\n'))) {
        const QString line = raw.trimmed();
        if (line.isEmpty() || line.startsWith(QStringLiteral("%%"))) {
            continue;
        }
        if (line.startsWith(QStringLiteral("flowchart")) ||
            line.startsWith(QStringLiteral("graph"))) {
            return QStringLiteral("flowchart");
        }
        return {}; // first meaningful line decides
    }
    return {};
}

} // namespace be::diagram
