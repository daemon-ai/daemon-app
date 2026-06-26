#pragma once

#include <QString>
#include <QVector>

namespace be {

// One table cell: its raw markdown (trimmed of surrounding whitespace, inline
// rendering applied later by the InlineProjector) plus the UTF-16 offset range
// of that trimmed content within the table block's raw markdown. The range is
// aligned to the first/last non-whitespace char so cellRawStart + inCellRaw
// indexes the block markdown directly. Empty cells get rawStart == rawEnd at a
// stable insertion point inside the cell, so hit-testing and span intersection
// degenerate to a zero-length range instead of failing.
struct TableCell {
    QString raw;
    qsizetype rawStart = 0;
    qsizetype rawEnd = 0;
};

// Structured view of a GFM table block, derived from its raw markdown.
// Alignment values match md4qt's enum: 0 = left, 1 = right, 2 = center.
struct TableData {
    int columns = 0;
    QVector<int> alignments;
    QVector<TableCell> header;
    QVector<QVector<TableCell>> rows;
};

// Cheap structural test used on the per-keystroke classification path: a line
// containing a pipe followed by a GFM delimiter row. Does not parse the table.
bool looksLikeTable(const QString& content);

// Parse block-local table markdown into structured data using md4qt (so pipes
// inside inline code and escaped pipes are handled correctly). Returns an empty
// TableData (columns == 0) when no table is found.
TableData parseTable(const QString& content);

} // namespace be
