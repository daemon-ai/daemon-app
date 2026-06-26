#pragma once

#include <memory>
#include <optional>
#include <QFont>
#include <QString>
#include <QTextLayout>

namespace be::diagram {

struct TextMetrics {
    qreal width = 0.0;
    qreal height = 0.0;
    int lineCount = 0;
};

// Headless replacement for the browser getBBox()/getComputedTextLength() that
// mermaid.js relies on. Measures with the exact QFont that will be drawn, so
// layout boxes and rendered glyphs always agree.
class TextMeasurer {
public:
    TextMetrics measure(const QString& text, const QFont& font,
                        std::optional<qreal> maxWidth = std::nullopt) const;

    // Builds a positioned QTextLayout reused directly by a QSGTextNode label.
    std::shared_ptr<QTextLayout> layout(const QString& text, const QFont& font,
                                        std::optional<qreal> maxWidth = std::nullopt) const;
};

} // namespace be::diagram
