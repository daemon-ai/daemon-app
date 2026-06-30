// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "core/document_store_detail.h"

#include "core/agent_block.h"
#include "core/markdown_table.h"

namespace be::docstore {

namespace {

const QRegularExpression& taskRe() {
    static const QRegularExpression re(QStringLiteral("^(\\s*)[-*+]\\s+\\[[ xX]\\]\\s+"));
    return re;
}

const QRegularExpression& bulletRe() {
    static const QRegularExpression re(QStringLiteral("^(\\s*)[-*+]\\s+"));
    return re;
}

const QRegularExpression& orderedRe() {
    static const QRegularExpression re(QStringLiteral("^(\\s*)\\d+[.)]\\s+"));
    return re;
}

// Raw text of `line` in `text` (without its trailing newline), or an empty view
// when out of range.
QStringView lineViewAt(const CoordinateMap& map, const QString& text, qsizetype line) {
    if (line < 0 || line >= map.lineCount()) {
        return {};
    }
    const qsizetype start = map.lineColumnToUtf16(line, 0);
    qsizetype end = text.indexOf(QLatin1Char('\n'), start);
    if (end < 0) {
        end = text.size();
    }
    return QStringView(text).mid(start, end - start);
}

// UTF-16 offset just past the last (non-newline) character of `line`.
qsizetype lineEndUtf16(const CoordinateMap& map, const QString& text, qsizetype line) {
    const qsizetype start = map.lineColumnToUtf16(line, 0);
    const qsizetype nl = text.indexOf(QLatin1Char('\n'), start);
    return nl < 0 ? text.size() : nl;
}

} // namespace

void applyAgentBlockFromFence(BlockRecord& record, const QString& info) {
    const BlockType type = agentBlockTypeForFence(info);
    if (type == BlockType::Unknown) {
        return;
    }
    record.type = type;
    const QVariantMap meta = parseAgentBlockMetadata(fencedBodyOf(record.markdown()));
    for (auto it = meta.constBegin(); it != meta.constEnd(); ++it) {
        record.metadata.insert(it.key(), it.value());
    }
}

bool isListType(BlockType type) {
    return type == BlockType::BulletListItem || type == BlockType::OrderedListItem ||
           type == BlockType::TaskListItem;
}

const QRegularExpression& headingRe() {
    static const QRegularExpression re(QStringLiteral("^(#{1,6})\\s+"));
    return re;
}

QString firstLineOf(const QString& content) {
    const qsizetype newline = content.indexOf(QLatin1Char('\n'));
    return newline < 0 ? content : content.left(newline);
}

QString headingSlug(const QString& headingText) {
    QString slug;
    slug.reserve(headingText.size());
    for (const QChar c : headingText) {
        if (c.isLetterOrNumber()) {
            slug += c.toLower();
        } else if (c == QLatin1Char('-') || c == QLatin1Char('_')) {
            slug += c;
        } else if (c.isSpace()) {
            slug += QLatin1Char(' ');
        }
        // All other punctuation is dropped.
    }
    slug = slug.simplified(); // collapse internal whitespace, trim ends
    slug.replace(QLatin1Char(' '), QLatin1Char('-'));
    return slug;
}

QString fenceLanguageOf(const QString& content) {
    const QString line = firstLineOf(content).trimmed();
    if (!line.startsWith(QStringLiteral("```")) && !line.startsWith(QStringLiteral("~~~"))) {
        return {};
    }
    const QString rest = line.mid(3).trimmed();
    const qsizetype sp = rest.indexOf(QLatin1Char(' '));
    return sp < 0 ? rest : rest.left(sp);
}

void applyImageMetadata(QVariantMap& metadata, const QString& content) {
    ImageBlockInfo info;
    if (!parseImageBlock(content, &info)) {
        return;
    }
    metadata.insert(QStringLiteral("imageUrl"), info.url);
    metadata.insert(QStringLiteral("imageAlt"), info.alt);
    metadata.insert(QStringLiteral("imageTitle"), info.title);
    metadata.insert(QStringLiteral("imageLink"), info.link);
    metadata.insert(QStringLiteral("imageWidth"), info.width);
    metadata.insert(QStringLiteral("imageHeight"), info.height);
}

qsizetype leadingMarkerLength(const QString& content, BlockType type) {
    const QString line = firstLineOf(content);
    const QRegularExpression* re = nullptr;
    switch (type) {
    case BlockType::TaskListItem:
        re = &taskRe();
        break;
    case BlockType::OrderedListItem:
        re = &orderedRe();
        break;
    case BlockType::BulletListItem:
        re = &bulletRe();
        break;
    default:
        return 0;
    }

    const QRegularExpressionMatch match = re->match(line);
    return match.hasMatch() ? match.capturedLength(0) : 0;
}

QString strippedLineText(const QString& content, BlockType type) {
    if (isListType(type)) {
        return content.mid(leadingMarkerLength(content, type));
    }
    if (type == BlockType::Heading) {
        const QRegularExpressionMatch match = headingRe().match(content);
        if (match.hasMatch()) {
            return content.mid(match.capturedLength(0));
        }
    }
    return content;
}

QString continuationMarker(const QString& content, BlockType type) {
    const qsizetype len = leadingMarkerLength(content, type);
    QString marker = content.left(len);
    if (type == BlockType::TaskListItem) {
        static const QRegularExpression checkbox(QStringLiteral("\\[[ xX]\\]"));
        marker.replace(checkbox, QStringLiteral("[ ]"));
    }
    return marker;
}

QByteArray separatorBetween(const BlockRecord& prev, const BlockRecord& cur) {
    if (isListType(prev.type) && isListType(cur.type)) {
        return QByteArrayLiteral("\n");
    }
    return QByteArrayLiteral("\n\n");
}

qsizetype fenceSafeLastBoundary(const QString& buffer) {
    bool inFence = false;
    bool afterBlank = false;
    qsizetype groupStart = 0;
    qsizetype lineStart = 0;
    const qsizetype n = buffer.size();

    for (qsizetype i = 0; i <= n; ++i) {
        if (i != n && buffer[i] != QLatin1Char('\n')) {
            continue;
        }

        const QStringView line = QStringView(buffer).mid(lineStart, i - lineStart);
        const QStringView trimmed = line.trimmed();
        const bool isFenceMarker =
            trimmed.startsWith(QLatin1String("```")) || trimmed.startsWith(QLatin1String("~~~"));

        if (isFenceMarker) {
            if (!inFence && afterBlank) {
                groupStart = lineStart;
                afterBlank = false;
            }
            inFence = !inFence;
        } else if (!inFence && trimmed.isEmpty()) {
            afterBlank = true;
        } else if (!inFence) {
            if (afterBlank) {
                groupStart = lineStart;
                afterBlank = false;
            }
        }

        lineStart = i + 1;
    }

    return groupStart;
}

QString withOrderedNumber(const QString& content, int number) {
    static const QRegularExpression re(QStringLiteral("^(\\s*)(\\d+)([.)])"));
    const QRegularExpressionMatch match = re.match(content);
    if (!match.hasMatch()) {
        return content;
    }
    QString result = content;
    result.replace(match.capturedStart(2), match.capturedLength(2), QString::number(number));
    return result;
}

bool isFenceMarkerLine(QStringView line) {
    const QStringView trimmed = line.trimmed();
    return trimmed.startsWith(QLatin1String("```")) || trimmed.startsWith(QLatin1String("~~~"));
}

// md4qt reports a fenced code block's span as the body *between* the delimiters
// (the opening fence line and its info string are consumed into Code::syntax()),
// so a plain slice drops the ``` / ~~~ markers and the block can no longer
// round-trip. Recover the full fenced span from md4qt's authoritative delimiter
// positions (ParsedBlock::fenceStartLine/fenceEndLine, lifted from
// Code::startDelim()/endDelim()): the open is that whole line (column 0, so any
// leading indentation is kept) and the close is the end of the closing fence
// line. An unterminated fence (open with no close, e.g. mid-stream) has no end
// delimiter and runs to the end of `text` so the open ``` is preserved.
//
// Only when md4qt gives no usable open position (degenerate/streamed parse) do we
// fall back to a raw line scan: anchor on the nearest fence marker at/before the
// reported body start, take the first marker after it as the close, and when the
// anchor itself is the closing fence (empty-body) re-find the open before it.
// Returns nullopt when no fence span can be recovered at all.
std::optional<FenceSpan> fenceSpanUtf16(const CoordinateMap& map, const QString& text,
                                        const ParsedBlock& pb) {
    const qsizetype lineCount = map.lineCount();
    if (lineCount <= 0) {
        return std::nullopt;
    }

    // Preferred path: md4qt's exact delimiter line positions.
    if (pb.fenceStartLine >= 0 && pb.fenceStartLine < lineCount) {
        FenceSpan span;
        span.start = map.lineColumnToUtf16(pb.fenceStartLine, 0);
        span.end = (pb.fenceEndLine >= 0 && pb.fenceEndLine < lineCount)
                       ? lineEndUtf16(map, text, pb.fenceEndLine)
                       : text.size();
        return span;
    }

    // Fallback: raw line scan when delimiter positions are unavailable.
    qsizetype openLine = -1;
    for (qsizetype l = qMin(pb.startLine, lineCount - 1); l >= 0; --l) {
        if (isFenceMarkerLine(lineViewAt(map, text, l))) {
            openLine = l;
            break;
        }
    }
    if (openLine < 0) {
        return std::nullopt;
    }

    qsizetype closeLine = -1;
    for (qsizetype l = openLine + 1; l < lineCount; ++l) {
        if (isFenceMarkerLine(lineViewAt(map, text, l))) {
            closeLine = l;
            break;
        }
    }

    if (closeLine < 0) {
        // No marker after `openLine`: the anchor was the closing fence of an
        // empty-body block. Re-find the real opening fence before it.
        for (qsizetype l = openLine - 1; l >= 0; --l) {
            if (isFenceMarkerLine(lineViewAt(map, text, l))) {
                closeLine = openLine;
                openLine = l;
                break;
            }
        }
    }

    FenceSpan span;
    span.start = map.lineColumnToUtf16(openLine, 0);
    // A terminated fence ends at its closing marker; an unterminated one runs to
    // the end of the buffer so the open ``` is preserved.
    span.end = closeLine >= 0 ? lineEndUtf16(map, text, closeLine) : text.size();
    return span;
}

QString sliceBlockContent(const CoordinateMap& map, const QString& text, const ParsedBlock& pb) {
    qsizetype startUtf16 = map.lineColumnToUtf16(pb.startLine, pb.startColumn);
    qsizetype endUtf16 = map.lineColumnToUtf16(pb.endLine, pb.endColumn) + 1;
    // Only fenced code needs delimiter recovery; indented code blocks have no
    // ``` / ~~~ lines and slice straight from md4qt's body span.
    if (pb.fenced) {
        if (const std::optional<FenceSpan> span = fenceSpanUtf16(map, text, pb)) {
            startUtf16 = span->start;
            endUtf16 = span->end;
        }
    }
    QString content = text.mid(startUtf16, qMax<qsizetype>(0, endUtf16 - startUtf16));
    while (content.endsWith(QLatin1Char('\n'))) {
        content.chop(1);
    }
    if (isListType(pb.type)) {
        qsizetype lead = 0;
        while (lead < content.size() && content.at(lead) == QLatin1Char(' ')) {
            ++lead;
        }
        content = content.mid(lead);
        if (pb.indent > 0) {
            content.prepend(QString(pb.indent, QLatin1Char(' ')));
        }
    }
    return content;
}

int orderedNumberOf(const QString& content, int fallback) {
    static const QRegularExpression re(QStringLiteral("^\\s*(\\d+)[.)]"));
    const QRegularExpressionMatch match = re.match(content);
    if (!match.hasMatch()) {
        return fallback;
    }
    bool ok = false;
    const int value = match.captured(1).toInt(&ok);
    return ok ? value : fallback;
}

BlockType classifyListItem(const QString& line, quint16* indent) {
    const QRegularExpressionMatch task = taskRe().match(line);
    if (task.hasMatch()) {
        if (indent) {
            *indent = static_cast<quint16>(task.capturedLength(1));
        }
        return BlockType::TaskListItem;
    }

    const QRegularExpressionMatch ordered = orderedRe().match(line);
    if (ordered.hasMatch()) {
        if (indent) {
            *indent = static_cast<quint16>(ordered.capturedLength(1));
        }
        return BlockType::OrderedListItem;
    }

    const QRegularExpressionMatch bullet = bulletRe().match(line);
    if (bullet.hasMatch()) {
        if (indent) {
            *indent = static_cast<quint16>(bullet.capturedLength(1));
        }
        return BlockType::BulletListItem;
    }

    return BlockType::Unknown;
}

BlockType classifyBlockShape(const QString& content, const QString& line) {
    if (parseImageBlock(content, nullptr)) {
        return BlockType::Image;
    }
    if (looksLikeTable(content)) {
        return BlockType::Table;
    }
    if (line.startsWith(QStringLiteral("```"))) {
        return BlockType::CodeFence;
    }
    if (line.startsWith(QStringLiteral("{{")) || line.startsWith(QStringLiteral(":::"))) {
        return BlockType::Custom;
    }
    if (line.startsWith(QLatin1Char('>'))) {
        return BlockType::Quote;
    }
    return BlockType::Paragraph;
}

} // namespace be::docstore
