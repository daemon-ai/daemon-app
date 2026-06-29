// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "core/document_store.h"

#include "core/agent_block.h"
#include "core/markdown_table.h"

#include <QRegularExpression>

namespace be {

namespace {

// Retype a freshly built code-fence record whose info string names an agent
// block kind ("tool"/"reasoning"/"content") into the matching typed block and
// hydrate its metadata from the fenced JSON body. This is the markdown -> typed
// step of the round-trip; the inverse (typed -> markdown) lives in
// serializeAgentBlock, kept in sync by appendTypedBlock/updateBlockMetadata.
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

QString firstLineOf(const QString& content) {
    const qsizetype newline = content.indexOf(QLatin1Char('\n'));
    return newline < 0 ? content : content.left(newline);
}

// GitHub-style heading slug: lowercase, drop characters other than letters,
// digits, space and hyphen, then turn runs of spaces into single hyphens.
// "## Foo Bar!" -> "foo-bar", matching the "#foo-bar" fragment a TOC links to.
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

// Language token after a leading ``` / ~~~ fence on the first line, or empty.
QString fenceLanguageOf(const QString& content) {
    const QString line = firstLineOf(content).trimmed();
    if (!line.startsWith(QStringLiteral("```")) && !line.startsWith(QStringLiteral("~~~"))) {
        return {};
    }
    const QString rest = line.mid(3).trimmed();
    const qsizetype sp = rest.indexOf(QLatin1Char(' '));
    return sp < 0 ? rest : rest.left(sp);
}

// Store image attributes (url/alt/title/link) from a single-line standalone
// image block into `metadata`, mirroring how fenceLanguage is captured.
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

// Length (in UTF-16 code units) of the leading block marker for a list item,
// including indentation and trailing whitespace. Returns 0 when absent.
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

// Strip a leading block-level marker (list bullet/number/checkbox or heading
// hashes) so the inline text can be concatenated onto another block.
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

// Produce the marker a freshly continued list item should carry. Bullets are
// copied verbatim, tasks reset to unchecked, ordered numbers are normalized by
// reserialize() so the literal number here is a placeholder.
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

// Offset (UTF-16) of the start of the last block-group in `buffer`, where a
// block-group boundary is a blank line that is NOT inside a fenced code block.
// Everything before this offset is structurally settled and safe to commit;
// everything at/after it stays volatile. Computed by a raw line scan because
// md4qt reports a fenced code block's start *after* its opening fence, so parsed
// positions cannot be used to locate (and preserve) the fence marker.
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

// True when `line` (after trimming) opens or closes a fenced code block.
bool isFenceMarkerLine(QStringView line) {
    const QStringView trimmed = line.trimmed();
    return trimmed.startsWith(QLatin1String("```")) || trimmed.startsWith(QLatin1String("~~~"));
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
// Returns false when no fence span can be recovered at all.
bool fenceSpanUtf16(const CoordinateMap& map, const QString& text, const ParsedBlock& pb,
                    qsizetype& startUtf16, qsizetype& endUtf16) {
    const qsizetype lineCount = map.lineCount();
    if (lineCount <= 0) {
        return false;
    }

    // Preferred path: md4qt's exact delimiter line positions.
    if (pb.fenceStartLine >= 0 && pb.fenceStartLine < lineCount) {
        startUtf16 = map.lineColumnToUtf16(pb.fenceStartLine, 0);
        endUtf16 = (pb.fenceEndLine >= 0 && pb.fenceEndLine < lineCount)
                       ? lineEndUtf16(map, text, pb.fenceEndLine)
                       : text.size();
        return true;
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
        return false;
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

    startUtf16 = map.lineColumnToUtf16(openLine, 0);
    // A terminated fence ends at its closing marker; an unterminated one runs to
    // the end of the buffer so the open ``` is preserved.
    endUtf16 = closeLine >= 0 ? lineEndUtf16(map, text, closeLine) : text.size();
    return true;
}

// Slice a parsed block's raw text out of `text` via `map`, trimming trailing
// newlines. For list items the leading indentation is normalized to `pb.indent`
// spaces so the stored markdown's leading whitespace always matches the block's
// indent depth (and reclassifies consistently when edited). Code fences are
// expanded to include their ``` / ~~~ delimiters (see fenceSpanUtf16).
QString sliceBlockContent(const CoordinateMap& map, const QString& text, const ParsedBlock& pb) {
    qsizetype startUtf16 = map.lineColumnToUtf16(pb.startLine, pb.startColumn);
    qsizetype endUtf16 = map.lineColumnToUtf16(pb.endLine, pb.endColumn) + 1;
    // Only fenced code needs delimiter recovery; indented code blocks have no
    // ``` / ~~~ lines and slice straight from md4qt's body span.
    if (pb.fenced) {
        fenceSpanUtf16(map, text, pb, startUtf16, endUtf16);
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

} // namespace

void DocumentStore::loadMarkdown(const QString& markdown) {
    loadFromParse(markdown);
}

void DocumentStore::loadMarkdownUtf8(const QByteArray& markdown) {
    loadFromParse(QString::fromUtf8(markdown));
}

void DocumentStore::loadFromParse(const QString& markdown) {
    m_nextBlockId = 1;
    m_blocks.clear();

    QVector<BlockRecord> records = recordsFromParse(markdown);
    m_blocks.reserve(records.size());
    for (BlockRecord& record : records) {
        record.id = allocateId();
        m_blocks.push_back(record);
    }

    // Re-arm the current-message latch from the loaded tail so a runtime append
    // that omits beginMessage continues the last message rather than starting an
    // un-roled run. A fresh turn always calls beginMessage first and overrides.
    // Also advance the message sequence past any loaded "m<n>" ids so a fresh
    // beginMessage never collides with an id already present in the document.
    static const QRegularExpression messageSeqRe(QStringLiteral("^m(\\d+)$"));
    quint64 maxSeq = 0;
    for (const BlockRecord& block : m_blocks) {
        const QRegularExpressionMatch match = messageSeqRe.match(block.messageId);
        if (match.hasMatch()) {
            maxSeq = qMax(maxSeq, match.captured(1).toULongLong());
        }
    }
    m_nextMessageSeq = maxSeq + 1;
    if (!m_blocks.isEmpty()) {
        m_currentRole = m_blocks.last().role;
        m_currentMessageId = m_blocks.last().messageId;
    } else {
        m_currentRole = MessageRole::None;
        m_currentMessageId.clear();
    }

    reserialize();
}

QVector<BlockRecord> DocumentStore::recordsFromParse(const QString& markdown) const {
    QVector<BlockRecord> records;

    const ParsedMarkdown parsed = m_parser.parse(markdown);

    CoordinateMap input;
    input.rebuild(markdown);
    const QString& text = input.text();

    if (!parsed.blocks.isEmpty()) {
        records.reserve(parsed.blocks.size());
        // Last UTF-16 offset consumed by a code fence's recovered span. When a
        // fence has an empty body md4qt can emit the orphaned closing ``` as its
        // own follow-on block; that line is already part of the fence's span, so
        // skip any parsed block that starts inside it to avoid a duplicate fence.
        qsizetype fenceConsumedThrough = -1;
        // Message boundary derivation: a ```msg marker sets the role/messageId
        // applied to every following content block until the next marker. The
        // marker itself is dropped (never becomes a row).
        MessageRole currentRole = MessageRole::None;
        QString currentMessageId;
        for (const ParsedBlock& pb : parsed.blocks) {
            const qsizetype pbStart = input.lineColumnToUtf16(pb.startLine, pb.startColumn);
            if (pbStart < fenceConsumedThrough) {
                continue;
            }

            const QString content = sliceBlockContent(input, text, pb);
            if (pb.fenced) {
                qsizetype fenceStart = 0;
                qsizetype fenceEnd = 0;
                if (fenceSpanUtf16(input, text, pb, fenceStart, fenceEnd)) {
                    fenceConsumedThrough = fenceEnd;
                }
            }

            if (pb.type == BlockType::CodeFence && isMessageMarkerFence(pb.info)) {
                parseMessageMarker(fencedBodyOf(content), &currentRole, &currentMessageId);
                continue;
            }

            BlockRecord record;
            record.role = currentRole;
            record.messageId = currentMessageId;
            record.type = pb.type;
            record.headingLevel = pb.headingLevel;
            record.indent = pb.indent;
            record.markdownUtf8 = content.toUtf8();
            if (pb.type == BlockType::CodeFence && !pb.info.isEmpty()) {
                record.metadata.insert(QStringLiteral("fenceLanguage"), pb.info);
                applyAgentBlockFromFence(record, pb.info);
            }
            if (pb.type == BlockType::Image) {
                record.metadata.insert(QStringLiteral("imageUrl"), pb.imageUrl);
                record.metadata.insert(QStringLiteral("imageAlt"), pb.imageAlt);
                record.metadata.insert(QStringLiteral("imageTitle"), pb.imageTitle);
                record.metadata.insert(QStringLiteral("imageLink"), pb.imageLink);
                record.metadata.insert(QStringLiteral("imageWidth"), pb.imageWidth);
                record.metadata.insert(QStringLiteral("imageHeight"), pb.imageHeight);
            }
            record.revision = 1;
            record.parseRevision = 1;
            record.renderRevision = 1;
            record.heightHint = pb.type == BlockType::Heading ? 42.0f : 28.0f;
            record.dirtyPersistence = true;
            records.push_back(record);
        }
    } else if (!markdown.trimmed().isEmpty()) {
        QString content = markdown;
        while (content.endsWith(QLatin1Char('\n'))) {
            content.chop(1);
        }
        records.push_back(makeClassifiedRecord(content));
    }

    return records;
}

BlockRecord DocumentStore::makeClassifiedRecord(const QString& content) const {
    BlockRecord record;
    quint16 level = 0;
    quint16 indent = 0;
    record.type = classifyContent(content, &level, &indent);
    record.headingLevel = level;
    record.indent = indent;
    record.markdownUtf8 = content.toUtf8();
    if (record.type == BlockType::CodeFence) {
        const QString lang = fenceLanguageOf(content);
        if (!lang.isEmpty()) {
            record.metadata.insert(QStringLiteral("fenceLanguage"), lang);
            applyAgentBlockFromFence(record, lang);
        }
    } else if (record.type == BlockType::Image) {
        applyImageMetadata(record.metadata, content);
    }
    record.revision = 1;
    record.parseRevision = 1;
    record.renderRevision = 1;
    record.heightHint = record.type == BlockType::Heading ? 42.0f : 28.0f;
    record.dirtyPersistence = true;
    return record;
}

qsizetype DocumentStore::blockCount() const {
    return m_blocks.size();
}

const QVector<BlockRecord>& DocumentStore::blocks() const {
    return m_blocks;
}

const BlockRecord* DocumentStore::blockAt(qsizetype row) const {
    if (row < 0 || row >= m_blocks.size()) {
        return nullptr;
    }
    return &m_blocks[row];
}

BlockRecord* DocumentStore::mutableBlockAt(qsizetype row) {
    if (row < 0 || row >= m_blocks.size()) {
        return nullptr;
    }
    return &m_blocks[row];
}

qsizetype DocumentStore::rowForBlock(BlockId id) const {
    return m_rowsById.value(id, -1);
}

qsizetype DocumentStore::rowForHeadingAnchor(const QString& fragment) const {
    QString target = fragment;
    if (target.startsWith(QLatin1Char('#'))) {
        target = target.mid(1);
    }
    target = target.toLower();
    if (target.isEmpty()) {
        return -1;
    }

    // Reduce link/image syntax to the visible label/alt before slugifying, so a
    // heading like "## See [Qt](https://qt.io)" slugs from "See Qt", not the url.
    static const QRegularExpression linkRe(QStringLiteral("!?\\[([^\\]]*)\\]\\([^)]*\\)"));

    for (qsizetype row = 0; row < m_blocks.size(); ++row) {
        const BlockRecord& block = m_blocks[row];
        if (block.type != BlockType::Heading) {
            continue;
        }
        QString text = block.markdown();
        const QRegularExpressionMatch marker = headingRe().match(text);
        if (marker.hasMatch()) {
            text = text.mid(marker.capturedEnd(0));
        }
        text.replace(linkRe, QStringLiteral("\\1"));
        if (headingSlug(text) == target) {
            return row;
        }
    }
    return -1;
}

QByteArray DocumentStore::toUtf8() const {
    ensureSerialized();
    return m_pieceTable.toUtf8();
}

QString DocumentStore::toMarkdown() const {
    return QString::fromUtf8(toUtf8());
}

QString DocumentStore::blockMarkdown(BlockId id) const {
    const qsizetype row = rowForBlock(id);
    const BlockRecord* block = blockAt(row);
    return block ? block->markdown() : QString();
}

bool DocumentStore::replaceBlockMarkdown(BlockId id, const QString& markdown) {
    const qsizetype row = rowForBlock(id);
    BlockRecord* block = mutableBlockAt(row);
    if (!block) {
        return false;
    }

    setBlockContent(*block, markdown);
    reserialize();
    return true;
}

bool DocumentStore::insertBlockAfter(BlockId id, const QString& markdown) {
    const qsizetype row = rowForBlock(id);
    if (row < 0) {
        return false;
    }

    const BlockRecord* anchor = blockAt(row);
    BlockRecord record;
    record.id = allocateId();
    if (anchor) {
        record.role = anchor->role;
        record.messageId = anchor->messageId;
    }
    setBlockContent(record, markdown);
    m_blocks.insert(row + 1, record);
    reserialize();
    return true;
}

bool DocumentStore::splitBlock(BlockId id, qsizetype utf16Offset, BlockId* resultBlock,
                               qsizetype* resultCursor, bool trimBoundary) {
    const qsizetype row = rowForBlock(id);
    BlockRecord* block = mutableBlockAt(row);
    if (!block) {
        return false;
    }

    const QString content = block->markdown();
    const qsizetype offset = qBound<qsizetype>(0, utf16Offset, content.size());
    const BlockType type = block->type;
    // New blocks born from a split stay in the same message as their source.
    const MessageRole splitRole = block->role;
    const QString splitMessageId = block->messageId;

    // Code fence: Enter inserts a literal newline so multi-line code editing is
    // unaffected. The exception is "Enter Enter" at the end of a closed fence: the
    // first Enter opens a trailing blank line, and a second Enter on that blank
    // line (when the last content line is a valid closing fence) exits the block by
    // splitting off a new empty paragraph below it.
    if (type == BlockType::CodeFence) {
        const bool atEnd = offset == content.size();
        if (atEnd && content.endsWith(QLatin1Char('\n'))) {
            QString fenceBody = content;
            fenceBody.chop(1);
            const QString lastLine = fenceBody.mid(fenceBody.lastIndexOf(QLatin1Char('\n')) + 1);
            if (isFenceMarkerLine(lastLine)) {
                setBlockContent(*block, fenceBody);

                BlockRecord paragraph;
                paragraph.id = allocateId();
                paragraph.role = splitRole;
                paragraph.messageId = splitMessageId;
                setBlockContent(paragraph, QString());
                m_blocks.insert(row + 1, paragraph);
                reserialize();
                if (resultBlock) {
                    *resultBlock = paragraph.id;
                }
                if (resultCursor) {
                    *resultCursor = 0;
                }
                return true;
            }
        }

        QString updated = content;
        updated.insert(offset, QLatin1Char('\n'));
        setBlockContent(*block, updated);
        reserialize();
        if (resultBlock) {
            *resultBlock = id;
        }
        if (resultCursor) {
            *resultCursor = offset + 1;
        }
        return true;
    }

    QString beforeContent;
    QString afterContent;

    if (isListType(type)) {
        const qsizetype markerLen = leadingMarkerLength(content, type);
        const QString marker = content.left(markerLen);
        const QString body = content.mid(markerLen);

        // Enter on an empty list item exits the list (becomes an empty paragraph).
        if (body.isEmpty()) {
            setBlockContent(*block, QString());
            reserialize();
            if (resultBlock) {
                *resultBlock = id;
            }
            if (resultCursor) {
                *resultCursor = 0;
            }
            return true;
        }

        const qsizetype bodyOffset = qBound<qsizetype>(0, offset - markerLen, body.size());
        beforeContent = marker + body.left(bodyOffset);
        afterContent = continuationMarker(content, type) + body.mid(bodyOffset);
    } else {
        // Headings, paragraphs, quotes: plain split. The trailing half loses any
        // leading marker, so it reclassifies (e.g. heading tail becomes paragraph).
        beforeContent = content.left(offset);
        afterContent = content.mid(offset);

        // Paragraph break: the just-typed blank line or trailing double-space that
        // triggered the split is boundary whitespace, not content. Drop it so the
        // two blocks are clean (e.g. "foo\n"/"foo  " -> "foo" + ""). Internal soft
        // breaks survive because a block never holds a blank line.
        if (trimBoundary) {
            while (!beforeContent.isEmpty() && beforeContent.back().isSpace()) {
                beforeContent.chop(1);
            }
            qsizetype lead = 0;
            while (lead < afterContent.size() && afterContent.at(lead).isSpace()) {
                ++lead;
            }
            afterContent = afterContent.mid(lead);
        }
    }

    setBlockContent(*block, beforeContent);

    BlockRecord inserted;
    inserted.id = allocateId();
    inserted.role = splitRole;
    inserted.messageId = splitMessageId;
    setBlockContent(inserted, afterContent);
    m_blocks.insert(row + 1, inserted);
    reserialize();

    if (resultBlock) {
        *resultBlock = inserted.id;
    }
    if (resultCursor) {
        // Place the caret after any continued list marker so typing lands in text.
        *resultCursor =
            isListType(inserted.type)
                ? leadingMarkerLength(QString::fromUtf8(inserted.markdownUtf8), inserted.type)
                : 0;
    }
    return true;
}

bool DocumentStore::mergeBlocks(BlockId previousId, BlockId currentId, qsizetype* resultCursor) {
    const qsizetype previousRow = rowForBlock(previousId);
    const qsizetype currentRow = rowForBlock(currentId);
    BlockRecord* previous = mutableBlockAt(previousRow);
    const BlockRecord* current = blockAt(currentRow);
    if (!previous || !current || previousRow == currentRow) {
        return false;
    }

    const qsizetype seam = previous->markdown().size();
    const QString tail = strippedLineText(current->markdown(), current->type);
    setBlockContent(*previous, previous->markdown() + tail);
    m_blocks.remove(currentRow);
    reserialize();

    if (resultCursor) {
        *resultCursor = seam;
    }
    return true;
}

bool DocumentStore::pasteMarkdown(BlockId id, qsizetype rawStart, qsizetype rawEnd,
                                  const QString& pasted, BlockId* caretBlock,
                                  qsizetype* caretOffset) {
    const qsizetype row = rowForBlock(id);
    BlockRecord* block = mutableBlockAt(row);
    if (!block) {
        return false;
    }

    const QString md = block->markdown();
    rawStart = qBound<qsizetype>(0, rawStart, md.size());
    rawEnd = qBound<qsizetype>(rawStart, rawEnd, md.size());
    const QString before = md.left(rawStart);
    const QString after = md.mid(rawEnd);

    QString normalized = pasted;
    normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalized.replace(QLatin1Char('\r'), QLatin1Char('\n'));

    const QVector<BlockRecord> pastedBlocks = recordsFromParse(normalized);

    // Inline merge only for a truly inline fragment: a single parsed block with
    // no newline. Anything multi-line/multi-block is a structural paste so a
    // pasted heading never merges into the current block. Whitespace-only pastes
    // (no parsed blocks) also merge inline so they never create a stray block.
    const bool inlinePaste = pastedBlocks.isEmpty() ||
                             (pastedBlocks.size() == 1 && !normalized.contains(QLatin1Char('\n')));
    if (inlinePaste) {
        setBlockContent(*block, before + normalized + after);
        reserialize();
        if (caretBlock) {
            *caretBlock = id;
        }
        if (caretOffset) {
            *caretOffset = before.size() + normalized.size();
        }
        return true;
    }

    QVector<BlockRecord> result;
    if (!before.isEmpty()) {
        result.push_back(makeClassifiedRecord(before));
    }
    const qsizetype pastedStart = result.size();
    result += pastedBlocks;
    if (!after.isEmpty()) {
        result.push_back(makeClassifiedRecord(after));
    }

    // Reuse the edited block's identity for the first resulting block so the
    // active block reference stays valid across the splice; all spliced blocks
    // inherit the edited block's message so a paste stays in the same turn.
    const quint32 baseRevision = block->revision;
    const MessageRole pasteRole = block->role;
    const QString pasteMessageId = block->messageId;
    for (qsizetype i = 0; i < result.size(); ++i) {
        result[i].role = pasteRole;
        result[i].messageId = pasteMessageId;
        if (i == 0) {
            result[i].id = id;
            result[i].revision = baseRevision + 1;
        } else {
            result[i].id = allocateId();
        }
    }

    // Caret lands at the end of the last pasted block (before any suffix block).
    const qsizetype caretIdx = pastedStart + pastedBlocks.size() - 1;
    if (caretBlock) {
        *caretBlock = result[caretIdx].id;
    }
    if (caretOffset) {
        *caretOffset = pastedBlocks.last().markdown().size();
    }

    spliceBlocks(row, 1, result);
    return true;
}

qsizetype DocumentStore::previousListIndentSpaces(qsizetype row) const {
    if (row <= 0) {
        return -1;
    }
    const BlockRecord* previous = blockAt(row - 1);
    if (!previous || !isListType(previous->type)) {
        return -1;
    }
    return previous->indent;
}

bool DocumentStore::adjustListIndent(BlockId id, int deltaUnits, qsizetype* cursorDelta) {
    if (cursorDelta) {
        *cursorDelta = 0;
    }
    if (deltaUnits == 0) {
        return false;
    }

    const qsizetype row = rowForBlock(id);
    BlockRecord* block = mutableBlockAt(row);
    if (!block || !isListType(block->type)) {
        return false;
    }

    const qsizetype currentSpaces = block->indent;
    qsizetype newSpaces = currentSpaces + (deltaUnits * kIndentUnit);

    if (deltaUnits > 0) {
        // CommonMark: an item may be at most one level deeper than the item above
        // it; the first item of a run cannot indent at all.
        const qsizetype prevSpaces = previousListIndentSpaces(row);
        if (prevSpaces < 0) {
            return false;
        }
        const qsizetype maxSpaces = prevSpaces + kIndentUnit;
        if (newSpaces > maxSpaces) {
            return false;
        }
    } else {
        if (currentSpaces == 0) {
            return false;
        }
        newSpaces = qMax<qsizetype>(0, newSpaces);
    }

    const QString content = block->markdown();
    qsizetype lead = 0;
    while (lead < content.size() && content.at(lead) == QLatin1Char(' ')) {
        ++lead;
    }
    const QString rewritten = QString(newSpaces, QLatin1Char(' ')) + content.mid(lead);
    setBlockContent(*block, rewritten);
    reserialize();

    if (cursorDelta) {
        *cursorDelta = newSpaces - currentSpaces;
    }
    return true;
}

bool DocumentStore::unlistItem(BlockId id, qsizetype* resultCursor) {
    const qsizetype row = rowForBlock(id);
    BlockRecord* block = mutableBlockAt(row);
    if (!block || !isListType(block->type)) {
        return false;
    }

    setBlockContent(*block, strippedLineText(block->markdown(), block->type));
    reserialize();

    if (resultCursor) {
        *resultCursor = 0;
    }
    return true;
}

bool DocumentStore::deleteBlocks(qsizetype firstRow, qsizetype count) {
    if (firstRow < 0 || count <= 0 || firstRow >= m_blocks.size()) {
        return false;
    }

    count = std::min(count, m_blocks.size() - firstRow);
    m_blocks.remove(firstRow, count);
    reserialize();
    return true;
}

bool DocumentStore::moveBlocks(qsizetype firstRow, qsizetype count, qsizetype destinationRow) {
    if (firstRow < 0 || count <= 0 || firstRow >= m_blocks.size() || destinationRow < 0 ||
        destinationRow > m_blocks.size()) {
        return false;
    }

    count = std::min(count, m_blocks.size() - firstRow);
    const QVector<BlockRecord> moved = m_blocks.mid(firstRow, count);
    m_blocks.remove(firstRow, count);

    if (destinationRow > firstRow) {
        destinationRow -= count;
    }
    destinationRow = qBound<qsizetype>(0, destinationRow, m_blocks.size());

    for (qsizetype i = 0; i < moved.size(); ++i) {
        m_blocks.insert(destinationRow + i, moved[i]);
    }
    reserialize();
    return true;
}

void DocumentStore::markPersisted(BlockId id) {
    BlockRecord* block = mutableBlockAt(rowForBlock(id));
    if (block) {
        block->dirtyPersistence = false;
    }
}

void DocumentStore::beginStreamAtEnd() {
    // Settle any pending derived state, then open a fresh (empty) volatile tail at
    // the end of the document. Streamed content forms new blocks appended after the
    // existing ones; the canonical separator is regenerated by reserialize().
    ensureSerialized();
    m_streaming = true;
    m_streamFirstRow = m_blocks.size();
    m_windowBuffer.clear();
}

BlockChangeSet DocumentStore::streamAppend(const QString& text) {
    if (!m_streaming) {
        return {};
    }
    m_windowBuffer += text;
    m_serializeDirty = true;
    return reparseWindow();
}

BlockChangeSet DocumentStore::reparseWindow() {
    // Cap on the raw tail buffer; bounds per-token cost when a single construct
    // (e.g. a long paragraph or list with no blank line) grows without a boundary.
    static constexpr qsizetype kMaxWindowBytes = 1 << 16;

    const ParsedMarkdown parsed = m_parser.parse(m_windowBuffer);

    CoordinateMap windowMap;
    windowMap.rebuild(m_windowBuffer);
    const QString& text = windowMap.text();

    struct NewBlock {
        BlockType type = BlockType::Paragraph;
        quint16 headingLevel = 0;
        quint16 indent = 0;
        QByteArray content;
        QString info; // fence info string, so an agent fence retypes mid-stream
    };

    QVector<NewBlock> news;
    if (!parsed.blocks.isEmpty()) {
        news.reserve(parsed.blocks.size());
        for (const ParsedBlock& pb : parsed.blocks) {
            const QString content = sliceBlockContent(windowMap, text, pb);
            news.push_back({pb.type, pb.headingLevel, pb.indent, content.toUtf8(), pb.info});
        }
    } else if (!m_windowBuffer.trimmed().isEmpty()) {
        QString content = m_windowBuffer;
        while (content.endsWith(QLatin1Char('\n'))) {
            content.chop(1);
        }
        quint16 level = 0;
        quint16 indent = 0;
        const BlockType type = classifyContent(content, &level, &indent);
        const QString info = type == BlockType::CodeFence ? fenceLanguageOf(content) : QString();
        news.push_back({type, level, indent, content.toUtf8(), info});
    }

    const qsizetype base = m_streamFirstRow;
    const qsizetype oldCount = m_blocks.size() - base;
    const qsizetype newCount = news.size();
    const qsizetype overlap = qMin(oldCount, newCount);

    BlockChangeSet cs;

    // Update overlapping volatile rows in place, preserving stable ids/revisions.
    for (qsizetype i = 0; i < overlap; ++i) {
        BlockRecord& block = m_blocks[base + i];
        const NewBlock& nb = news[i];
        if (block.markdownUtf8 != nb.content || block.type != nb.type) {
            block.markdownUtf8 = nb.content;
            block.type = nb.type;
            block.headingLevel = nb.headingLevel;
            block.indent = nb.indent;
            if (nb.type == BlockType::CodeFence && !nb.info.isEmpty()) {
                block.metadata.insert(QStringLiteral("fenceLanguage"), nb.info);
                applyAgentBlockFromFence(block, nb.info);
            }
            block.revision += 1;
            block.renderRevision += 1;
            block.dirtyText = true;
            block.dirtyRender = true;
            block.dirtyPersistence = true;
            if (cs.changedFirst < 0) {
                cs.changedFirst = base + i;
            }
            cs.changedLast = base + i;
        }
    }

    cs.structuralRow = base + overlap;
    if (newCount > oldCount) {
        cs.insertedCount = newCount - oldCount;
        for (qsizetype i = oldCount; i < newCount; ++i) {
            const NewBlock& nb = news[i];
            BlockRecord record;
            record.id = allocateId();
            record.role = m_currentRole;
            record.messageId = m_currentMessageId;
            record.type = nb.type;
            record.headingLevel = nb.headingLevel;
            record.indent = nb.indent;
            record.markdownUtf8 = nb.content;
            if (nb.type == BlockType::CodeFence && !nb.info.isEmpty()) {
                record.metadata.insert(QStringLiteral("fenceLanguage"), nb.info);
                applyAgentBlockFromFence(record, nb.info);
            }
            record.revision = 1;
            record.parseRevision = 1;
            record.renderRevision = 1;
            record.heightHint = nb.type == BlockType::Heading ? 42.0f : 28.0f;
            record.dirtyPersistence = true;
            m_blocks.insert(base + i, record);
        }
    } else if (oldCount > newCount) {
        cs.removedCount = oldCount - newCount;
        m_blocks.remove(base + newCount, oldCount - newCount);
    }

    // Commit settled blocks: only the trailing block-group (everything after the
    // last blank line outside a fence) stays volatile; the rest leave the window.
    // A fenced code block stays one open block until it closes and a following
    // block appears; list items in a run commit together once a blank line ends
    // the run. Committed blocks remain in m_blocks; only the open tail stays in
    // m_windowBuffer.
    const qsizetype trimOffset = fenceSafeLastBoundary(m_windowBuffer);
    if (trimOffset > 0) {
        const QString volatileText = m_windowBuffer.mid(trimOffset);
        qsizetype volatileCount = m_parser.parse(volatileText).blocks.size();
        if (volatileCount == 0 && !volatileText.trimmed().isEmpty()) {
            volatileCount = 1;
        }
        const qsizetype commitCount = qMax<qsizetype>(0, newCount - volatileCount);
        m_streamFirstRow = base + commitCount;
        m_windowBuffer = volatileText;
    }

    // Force-commit guard: the volatile tail grew past the cap with no internal
    // boundary (e.g. a single long soft-wrapped paragraph). Commit it and reopen
    // an empty tail to bound per-token cost; this inserts a hard boundary.
    if (m_windowBuffer.size() > kMaxWindowBytes) {
        m_streamFirstRow = m_blocks.size();
        m_windowBuffer.clear();
    }

    rebuildRowIndex();
    return cs;
}

void DocumentStore::endStream() {
    if (!m_streaming) {
        return;
    }
    m_streaming = false;
    m_streamFirstRow = m_blocks.size();
    m_windowBuffer.clear();
    reserialize();
}

bool DocumentStore::isStreaming() const {
    return m_streaming;
}

QString DocumentStore::beginMessage(MessageRole role) {
    m_currentRole = role;
    m_currentMessageId = QStringLiteral("m%1").arg(m_nextMessageSeq++);
    return m_currentMessageId;
}

QString DocumentStore::appendMessageBlocks(MessageRole role, const QString& markdown,
                                           const QString& explicitId) {
    QString id;
    if (!explicitId.isEmpty()) {
        // Preserve the authored message id (the transcript-log applier path).
        m_currentRole = role;
        m_currentMessageId = explicitId;
        id = explicitId;
    } else if (role == MessageRole::None) {
        // An un-roled run: no message id, no boundary marker on serialize.
        m_currentRole = MessageRole::None;
        m_currentMessageId.clear();
    } else {
        id = beginMessage(role);
    }
    QVector<BlockRecord> records = recordsFromParse(markdown);
    for (BlockRecord& record : records) {
        record.id = allocateId();
        record.role = role;
        record.messageId = id;
        m_blocks.push_back(record);
    }
    reserialize();
    return id;
}

qsizetype DocumentStore::rowForMessage(const QString& messageId) const {
    if (messageId.isEmpty()) {
        return -1;
    }
    for (qsizetype row = 0; row < m_blocks.size(); ++row) {
        if (m_blocks[row].messageId == messageId) {
            return row;
        }
    }
    return -1;
}

QString DocumentStore::rewindToMessage(const QString& messageId) {
    const qsizetype row = rowForMessage(messageId);
    if (row < 0) {
        return {};
    }
    // Gather the message's own text before truncating, so the caller can resubmit
    // it (restore) or seed an edit composer with it (edit).
    QStringList parts;
    for (qsizetype r = row; r < m_blocks.size(); ++r) {
        if (m_blocks[r].messageId == messageId) {
            parts << m_blocks[r].markdown();
        }
    }
    deleteBlocks(row, m_blocks.size() - row);
    return parts.join(QStringLiteral("\n\n"));
}

bool DocumentStore::regenerateFromMessage(const QString& messageId) {
    const qsizetype row = rowForMessage(messageId);
    if (row < 0) {
        return false;
    }
    return deleteBlocks(row, m_blocks.size() - row);
}

MessageRole DocumentStore::currentMessageRole() const {
    return m_currentRole;
}

QString DocumentStore::currentMessageId() const {
    return m_currentMessageId;
}

BlockId DocumentStore::appendTypedBlock(BlockType type, const QVariantMap& metadata,
                                        BlockChangeSet* changeSet) {
    // An open text-stream window is committed first: a typed block is a hard
    // boundary, so the streamed paragraph above it settles and the volatile tail
    // restarts after the injected block.
    if (m_streaming) {
        m_streamFirstRow = m_blocks.size();
        m_windowBuffer.clear();
    }

    BlockRecord record;
    record.id = allocateId();
    record.type = type;
    record.role = m_currentRole;
    record.messageId = m_currentMessageId;
    record.metadata = metadata;
    // Keep the canonical fenced markdown in sync so save/export round-trips even
    // though the live render reads metadata. Non-agent types keep their raw form.
    const QByteArray serialized = serializeAgentBlock(type, metadata);
    if (!serialized.isEmpty()) {
        record.markdownUtf8 = serialized;
    }
    record.revision = 1;
    record.parseRevision = 1;
    record.renderRevision = 1;
    record.heightHint = 28.0f;
    record.dirtyPersistence = true;

    const qsizetype row = m_blocks.size();
    m_blocks.push_back(record);
    if (m_streaming) {
        m_streamFirstRow = m_blocks.size();
    }
    m_serializeDirty = true;
    rebuildRowIndex();

    if (changeSet) {
        changeSet->structuralRow = row;
        changeSet->insertedCount = 1;
    }
    return record.id;
}

BlockChangeSet DocumentStore::updateBlockMetadata(BlockId id, const QVariantMap& patch) {
    BlockChangeSet cs;
    const qsizetype row = rowForBlock(id);
    if (row < 0) {
        return cs;
    }

    BlockRecord& block = m_blocks[row];
    for (auto it = patch.constBegin(); it != patch.constEnd(); ++it) {
        block.metadata.insert(it.key(), it.value());
    }
    if (isAgentBlockType(block.type)) {
        const QByteArray serialized = serializeAgentBlock(block.type, block.metadata);
        if (!serialized.isEmpty()) {
            block.markdownUtf8 = serialized;
        }
    }
    block.revision += 1;
    block.renderRevision += 1;
    block.dirtyRender = true;
    block.dirtyPersistence = true;
    m_serializeDirty = true;

    cs.changedFirst = row;
    cs.changedLast = row;
    return cs;
}

BlockId DocumentStore::blockIdForMetadata(const QString& key, const QVariant& value) const {
    for (const BlockRecord& block : m_blocks) {
        if (block.metadata.value(key) == value) {
            return block.id;
        }
    }
    return 0;
}

void DocumentStore::spliceBlocks(qsizetype firstRow, qsizetype removeCount,
                                 const QVector<BlockRecord>& insert) {
    firstRow = qBound<qsizetype>(0, firstRow, m_blocks.size());
    removeCount = qBound<qsizetype>(0, removeCount, m_blocks.size() - firstRow);
    m_blocks.remove(firstRow, removeCount);
    for (qsizetype i = 0; i < insert.size(); ++i) {
        m_blocks.insert(firstRow + i, insert[i]);
    }
    reserialize();
}

QVector<BlockRecord> DocumentStore::snapshot() const {
    return m_blocks;
}

void DocumentStore::restore(const QVector<BlockRecord>& blocks) {
    m_blocks = blocks;
    reserialize();
}

void DocumentStore::reserialize() {
    // Normalize ordered-list numbering, tracking an independent counter per
    // nesting depth. Returning to a shallower depth drops deeper counters (so
    // re-entering a level restarts it), a bullet/task item resets the ordered
    // counter at its depth, and any non-list block ends all runs.
    QVector<int> orderedCounters;
    for (BlockRecord& block : m_blocks) {
        if (!isListType(block.type)) {
            orderedCounters.clear();
            continue;
        }

        const int depth = block.indent / int(kIndentUnit);
        orderedCounters.resize(depth + 1);

        if (block.type == BlockType::OrderedListItem) {
            // A fresh top-level run honours its written start number; a fresh
            // nested run always restarts at 1 (its inherited literal is stale).
            const int freshStart = depth == 0 ? orderedNumberOf(block.markdown(), 1) : 1;
            const int number =
                orderedCounters[depth] == 0 ? freshStart : orderedCounters[depth] + 1;
            const QString renumbered = withOrderedNumber(block.markdown(), number);
            if (renumbered.toUtf8() != block.markdownUtf8) {
                block.markdownUtf8 = renumbered.toUtf8();
            }
            orderedCounters[depth] = number;
        } else {
            orderedCounters[depth] = 0;
        }
    }

    QByteArray joined;
    MessageRole prevRole = MessageRole::None;
    QString prevMessageId;
    for (qsizetype row = 0; row < m_blocks.size(); ++row) {
        BlockRecord& block = m_blocks[row];
        // A message boundary: the first roled block, or any block whose role or
        // message id differs from the block above it. Emit a ```msg marker (a
        // blank line on each side so it always parses as its own fenced block,
        // even between list items whose normal separator is a single newline).
        const bool boundary =
            block.role != MessageRole::None &&
            (row == 0 || block.role != prevRole || block.messageId != prevMessageId);
        if (row > 0) {
            joined +=
                boundary ? QByteArrayLiteral("\n\n") : separatorBetween(m_blocks[row - 1], block);
        }
        if (boundary) {
            joined += serializeMessageMarker(block.role, block.messageId);
            joined += "\n\n";
        }
        block.source = {joined.size(), block.markdownUtf8.size()};
        joined += block.markdownUtf8;
        prevRole = block.role;
        prevMessageId = block.messageId;
    }
    if (!m_blocks.isEmpty()) {
        joined += '\n';
    }

    m_pieceTable.setOriginal(joined);
    m_coordinateMap.rebuild(joined);
    rebuildRowIndex();
    m_serializeDirty = false;
}

void DocumentStore::ensureSerialized() const {
    if (m_serializeDirty) {
        const_cast<DocumentStore*>(this)->reserialize();
    }
}

void DocumentStore::rebuildRowIndex() {
    m_rowsById.clear();
    for (qsizetype row = 0; row < m_blocks.size(); ++row) {
        m_rowsById.insert(m_blocks[row].id, row);
    }
}

BlockId DocumentStore::allocateId() {
    return m_nextBlockId++;
}

void DocumentStore::setBlockContent(BlockRecord& block, const QString& content) const {
    quint16 level = 0;
    quint16 indent = 0;
    block.markdownUtf8 = content.toUtf8();
    block.type = classifyContent(content, &level, &indent);
    block.headingLevel = level;
    block.indent = indent;
    if (block.type == BlockType::CodeFence) {
        const QString lang = fenceLanguageOf(content);
        if (!lang.isEmpty()) {
            block.metadata.insert(QStringLiteral("fenceLanguage"), lang);
            applyAgentBlockFromFence(block, lang);
        }
    } else if (block.type == BlockType::Image) {
        applyImageMetadata(block.metadata, content);
    }
    block.revision += 1;
    block.renderRevision += 1;
    block.dirtyText = true;
    block.dirtyRender = true;
    block.dirtyPersistence = true;
}

BlockType DocumentStore::classifyContent(const QString& content, quint16* headingLevel,
                                         quint16* indent) const {
    if (headingLevel) {
        *headingLevel = 0;
    }
    if (indent) {
        *indent = 0;
    }

    const QString line = firstLineOf(content);

    const QRegularExpressionMatch heading = headingRe().match(line);
    if (heading.hasMatch()) {
        if (headingLevel) {
            *headingLevel = static_cast<quint16>(heading.capturedLength(1));
        }
        return BlockType::Heading;
    }

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

} // namespace be
