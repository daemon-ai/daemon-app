#include "core/markdown_table.h"

#include "core/coordinate_map.h"

#include <doc.h>
#include <parser.h>
#include <QRegularExpression>
#include <QTextStream>

namespace be {

namespace {

const QRegularExpression& delimiterRe() {
    static const QRegularExpression re(
        QStringLiteral("^\\s*\\|?\\s*:?-{1,}:?\\s*(\\|\\s*:?-{1,}:?\\s*)*\\|?\\s*$"));
    return re;
}

} // namespace

bool looksLikeTable(const QString& content) {
    const qsizetype firstBreak = content.indexOf(QLatin1Char('\n'));
    if (firstBreak < 0) {
        return false;
    }

    const QString firstLine = content.left(firstBreak);
    if (!firstLine.contains(QLatin1Char('|'))) {
        return false;
    }

    const qsizetype secondBreak = content.indexOf(QLatin1Char('\n'), firstBreak + 1);
    const QString secondLine = secondBreak < 0
                                   ? content.mid(firstBreak + 1)
                                   : content.mid(firstBreak + 1, secondBreak - firstBreak - 1);

    return delimiterRe().match(secondLine).hasMatch();
}

TableData parseTable(const QString& content) {
    TableData data;

    QString input = content;
    QTextStream stream(&input, QIODeviceBase::ReadOnly);

    MD::Parser parser;
    const auto document = parser.parse(stream, QString(), QStringLiteral("memory.md"));
    if (!document) {
        return data;
    }

    const MD::Table* table = nullptr;
    for (const auto& itemPtr : document->items()) {
        const MD::Item* item = itemPtr.data();
        if (item && item->type() == MD::ItemType::Table) {
            table = static_cast<const MD::Table*>(item);
            break;
        }
    }
    if (!table || table->isEmpty()) {
        return data;
    }

    CoordinateMap map;
    map.rebuild(content);
    const QString& text = map.text();

    data.columns = table->columnsCount();
    data.alignments.reserve(data.columns);
    for (int c = 0; c < data.columns; ++c) {
        data.alignments.push_back(static_cast<int>(table->columnAlignment(c)));
    }

    // Slice a cell to its trimmed raw text and the UTF-16 range of that trimmed
    // content within the block markdown. `fallbackOffset` is the insertion point
    // used for empty/null cells so they still produce a usable zero-length range.
    const auto sliceCell = [&](const MD::TableCell* cell, qsizetype fallbackOffset) -> TableCell {
        if (!cell || cell->isNullPositions()) {
            return TableCell{QString(), fallbackOffset, fallbackOffset};
        }
        const qsizetype startUtf16 = map.lineColumnToUtf16(cell->startLine(), cell->startColumn());
        // An empty cell carries no inline children. md4qt may still report
        // degenerate positions for it (e.g. spanning a delimiter pipe), so trust
        // emptiness here and collapse to a stable zero-length point.
        if (cell->isEmpty()) {
            return TableCell{QString(), startUtf16, startUtf16};
        }
        const qsizetype endUtf16 = map.lineColumnToUtf16(cell->endLine(), cell->endColumn()) + 1;
        const qsizetype rawLen = qMax<qsizetype>(0, endUtf16 - startUtf16);
        const QString untrimmed = text.mid(startUtf16, rawLen);

        qsizetype leading = 0;
        while (leading < untrimmed.size() && untrimmed.at(leading).isSpace()) {
            ++leading;
        }
        const QString trimmed = untrimmed.trimmed();
        if (trimmed.isEmpty()) {
            // Empty cell: collapse to a stable zero-length point inside the cell.
            return TableCell{QString(), startUtf16, startUtf16};
        }
        const qsizetype rawStart = startUtf16 + leading;
        return TableCell{trimmed, rawStart, rawStart + trimmed.size()};
    };

    const auto& rows = table->rows();
    for (qsizetype r = 0; r < rows.size(); ++r) {
        const MD::TableRow* row = rows[r].data();
        QVector<TableCell> cells;
        qsizetype runningOffset = 0;
        if (row) {
            for (const auto& cellPtr : row->cells()) {
                const TableCell cell = sliceCell(cellPtr.data(), runningOffset);
                runningOffset = cell.rawEnd;
                cells.push_back(cell);
            }
        }
        // Pad short rows with empty cells anchored at the last known offset.
        while (cells.size() < data.columns) {
            cells.push_back(TableCell{QString(), runningOffset, runningOffset});
        }
        if (r == 0) {
            data.header = cells;
        } else {
            data.rows.push_back(cells);
        }
    }

    return data;
}

} // namespace be
