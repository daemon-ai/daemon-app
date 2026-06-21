#pragma once

#include <Tui/ZColor.h>
#include <Tui/ZCommon.h>

#include <QString>
#include <QVector>

namespace be {
class DocumentStore;
}

// A single styled run on one screen row: text plus its terminal colors and
// attributes. RenderLine is a fully wrapped row, ready to paint span-by-span via
// ZPainter::writeWithColors / writeWithAttributes.
struct Span {
    QString text;
    Tui::ZColor fg;
    Tui::ZColor bg;
    Tui::ZTextAttributes attr;
};

using RenderLine = QVector<Span>;

// The TUI analog of the GUI's QML block delegates: turns a parsed be::DocumentStore
// into wrapped, styled terminal rows. Pure (no widget/painter state) so it unit-
// tests headlessly against a DocumentStore loaded from markdown.
//
// Per BlockType it mirrors the matching QML component (MessageHeader, ReasoningBlock,
// ToolCallBlock, ContentBlock, CodeBlock, ...), reusing be::buildToolView /
// buildReasoningView / buildContentView / ansiToSpans / parseUnifiedDiff so both
// front ends share the exact same view-model logic.
class TranscriptLayout {
public:
    // Build the full transcript at the given content width (columns). A width <= 0
    // is treated as a sane minimum so callers can build before the first resize.
    static QVector<RenderLine> build(const be::DocumentStore &doc, int width);
};
