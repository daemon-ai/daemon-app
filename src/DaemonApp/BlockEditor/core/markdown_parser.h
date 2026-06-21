#pragma once

#include "core/block_record.h"

#include <QSharedPointer>
#include <QString>
#include <QVector>

namespace MD {
class Document;
class Item;
}

namespace be {

struct ParsedBlock {
    BlockType type = BlockType::Unknown;
    quint16 headingLevel = 0;
    quint16 indent = 0;
    qsizetype startLine = 0;
    qsizetype startColumn = 0;
    qsizetype endLine = 0;
    qsizetype endColumn = 0;
    QString info; // fenced code language ("mermaid", "cpp", ...)

    // Fenced code only. md4qt reports a Code block's start/end as the body
    // *between* the ``` / ~~~ delimiters, so the delimiter lines are recovered
    // separately from md4qt's authoritative startDelim()/endDelim() positions.
    // `fenced` is false for indented code blocks (which have no delimiter lines).
    // A -1 fence*Line means md4qt gave no position (e.g. an unterminated fence,
    // whose recovered span then runs to end-of-text).
    bool fenced = false;
    qsizetype fenceStartLine = -1;
    qsizetype fenceEndLine = -1;

    // Populated for BlockType::Image (a paragraph that is solely an image, or a
    // link whose label is solely an image). imageLink is the click-through URL
    // for the linked-standalone case, empty otherwise. imageWidth/imageHeight are
    // the raw Pandoc {width= height=} attribute values (e.g. "200", "50%"), if any.
    QString imageUrl;
    QString imageAlt;
    QString imageTitle;
    QString imageLink;
    QString imageWidth;
    QString imageHeight;
};

// Image attributes extracted from a single-line standalone image block.
struct ImageBlockInfo {
    QString url;
    QString alt;
    QString title;
    QString link; // click-through page URL for [![alt](img)](page), else empty
    QString width; // raw Pandoc dimension ("200", "200px", "50%", "2cm"), else empty
    QString height;
};

// Parse a line that is solely a standalone image (`![alt](url "title")`) or a
// linked standalone image (`[![alt](img)](page)`), each optionally followed by a
// Pandoc attribute block (`{ width=.. height=.. }`). Returns true and fills `out`
// (when non-null) on a match; returns false for anything else.
bool parseImageBlock(const QString &content, ImageBlockInfo *out);

// Parse a Pandoc image dimension ("200", "200px", "50%", "2cm", ...). Returns the
// value in logical pixels for absolute units; for "%" returns the percent value
// and sets *percent=true (when non-null). Returns 0 for empty/invalid input.
qreal imageDimensionValue(const QString &raw, bool *percent = nullptr);

// Extract a `key=value` attribute (value optionally quoted) from a Pandoc-style
// attribute string such as "{ width=50% height=200 }". Returns empty if absent.
QString imageAttribute(const QString &attrs, const QString &key);

struct ParsedMarkdown {
    QSharedPointer<MD::Document> document;
    QVector<ParsedBlock> blocks;
};

struct DirtyWindow {
    qsizetype startBlock = 0;
    qsizetype endBlock = 0;
    bool structural = false;
};

class MarkdownParser
{
public:
    ParsedMarkdown parse(const QString &markdown) const;

    static BlockType blockTypeForItem(const MD::Item *item);
    static DirtyWindow dirtyWindowForEdit(qsizetype blockIndex, const QString &inserted, const QString &removed, qsizetype blockCount);
};

} // namespace be
