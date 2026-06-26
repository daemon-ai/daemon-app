#include "core/markdown_parser.h"

#include <doc.h>
#include <parser.h>
#include <QRegularExpression>
#include <QTextStream>

namespace be {

namespace {

// True when a parsed link's description carries no visible text (only images and
// whitespace), i.e. it is a standalone linked image rather than a favicon+label.
bool linkLabelIsImageOnly(const MD::Link* link) {
    const auto p = link->p();
    if (!p) {
        return true;
    }
    for (const auto& itemPtr : p->items()) {
        const MD::Item* item = itemPtr.data();
        if (item && item->type() == MD::ItemType::Text) {
            if (!static_cast<const MD::Text*>(item)->text().trimmed().isEmpty()) {
                return false;
            }
        }
    }
    return true;
}

// True when `text` is a lone Pandoc attribute block, e.g. "{ width=50% }".
bool isAttributeBlock(const QString& text) {
    const QString t = text.trimmed();
    return t.size() >= 2 && t.startsWith(QLatin1Char('{')) && t.endsWith(QLatin1Char('}'));
}

// If the paragraph is solely a standalone image (or a standalone linked image),
// optionally followed by a Pandoc attribute block, promote `block` to
// BlockType::Image and fill its image fields (url/alt/title/link + width/height).
void detectStandaloneImage(const MD::Paragraph* para, ParsedBlock& block) {
    if (!para) {
        return;
    }
    const auto& items = para->items();
    if (items.isEmpty() || items.size() > 2) {
        return;
    }

    // Allow exactly one trailing text item, and only when it is an attribute block
    // (md4qt parses `![a](u){width=..}` as Image + Text("{width=..}")).
    QString attrs;
    if (items.size() == 2) {
        const MD::Item* second = items[1].data();
        if (!second || second->type() != MD::ItemType::Text) {
            return;
        }
        const QString text = static_cast<const MD::Text*>(second)->text();
        if (!isAttributeBlock(text)) {
            return;
        }
        attrs = text.trimmed();
    }

    const MD::Item* only = items.first().data();
    if (!only) {
        return;
    }

    const auto applyAttrs = [&attrs](ParsedBlock& b) {
        if (attrs.isEmpty()) {
            return;
        }
        b.imageWidth = imageAttribute(attrs, QStringLiteral("width"));
        b.imageHeight = imageAttribute(attrs, QStringLiteral("height"));
    };

    if (only->type() == MD::ItemType::Image) {
        const auto* image = static_cast<const MD::Image*>(only);
        block.type = BlockType::Image;
        block.imageUrl = image->url();
        block.imageAlt = image->text();
        block.imageTitle = image->title();
        applyAttrs(block);
        return;
    }

    if (only->type() == MD::ItemType::Link) {
        const auto* link = static_cast<const MD::Link*>(only);
        const auto image = link->img();
        if (image && !image->url().isEmpty() && linkLabelIsImageOnly(link)) {
            block.type = BlockType::Image;
            block.imageUrl = image->url();
            block.imageAlt = image->text();
            block.imageTitle = image->title();
            block.imageLink = link->url();
            applyAttrs(block);
        }
    }
}

ParsedBlock makeParsedBlock(const MD::Item* item) {
    ParsedBlock block;
    block.type = MarkdownParser::blockTypeForItem(item);
    block.startLine = item->startLine();
    block.startColumn = item->startColumn();
    block.endLine = item->endLine();
    block.endColumn = item->endColumn();

    if (item->type() == MD::ItemType::Heading) {
        block.headingLevel = static_cast<quint16>(static_cast<const MD::Heading*>(item)->level());
    } else if (item->type() == MD::ItemType::Code) {
        const auto* code = static_cast<const MD::Code*>(item);
        block.info = code->syntax();
        // md4qt's start/end span only covers the body between the delimiters; the
        // ``` / ~~~ lines live in startDelim()/endDelim(). Capture those so the
        // store can recover the full delimiter-inclusive span without guessing.
        // Indented code blocks are not fenced and have no delimiter positions.
        block.fenced = code->isFensedCode();
        if (block.fenced) {
            const MD::WithPosition& startDelim = code->startDelim();
            const MD::WithPosition& endDelim = code->endDelim();
            if (!startDelim.isNullPositions()) {
                block.fenceStartLine = startDelim.startLine();
            }
            if (!endDelim.isNullPositions()) {
                block.fenceEndLine = endDelim.endLine();
            }
        }
    } else if (item->type() == MD::ItemType::Paragraph) {
        detectStandaloneImage(static_cast<const MD::Paragraph*>(item), block);
    }

    return block;
}

// Normalized indentation step (in spaces) applied per nesting level. The raw
// source indentation is discarded in favour of this canonical unit so the block
// model carries consistent, classify-friendly leading whitespace.
constexpr quint16 kListIndentUnit = 2;

// Flatten a md4qt List into one ParsedBlock per ListItem, descending into nested
// MD::List children so each nested item becomes its own editable block. A list
// item's own text span ends just before its first nested list (multi-paragraph
// content after a nested list is not split out and is left for a later pass).
void appendListItems(const MD::List* list, QVector<ParsedBlock>& out, quint16 indentSpaces) {
    if (!list) {
        return;
    }

    for (const auto& childPtr : list->items()) {
        const MD::Item* child = childPtr.data();
        if (!child || child->isNullPositions() || child->type() != MD::ItemType::ListItem) {
            continue;
        }

        const auto* item = static_cast<const MD::ListItem*>(child);

        // Partition the item's children into its own content (everything before
        // the first nested list) and the nested lists themselves.
        const MD::Item* lastOwn = nullptr;
        QVector<const MD::List*> nested;
        for (const auto& subPtr : item->items()) {
            const MD::Item* sub = subPtr.data();
            if (!sub || sub->isNullPositions()) {
                continue;
            }
            if (sub->type() == MD::ItemType::List) {
                nested.push_back(static_cast<const MD::List*>(sub));
            } else if (nested.isEmpty()) {
                lastOwn = sub;
            }
        }

        ParsedBlock block;
        if (item->isTaskList()) {
            block.type = BlockType::TaskListItem;
        } else if (item->listType() == MD::ListItem::Ordered) {
            block.type = BlockType::OrderedListItem;
        } else {
            block.type = BlockType::BulletListItem;
        }
        block.indent = indentSpaces;
        block.startLine = item->startLine();
        block.startColumn = item->startColumn();
        if (lastOwn) {
            block.endLine = lastOwn->endLine();
            block.endColumn = lastOwn->endColumn();
        } else {
            block.endLine = item->endLine();
            block.endColumn = item->endColumn();
        }
        out.push_back(block);

        for (const MD::List* nestedList : nested) {
            appendListItems(nestedList, out, static_cast<quint16>(indentSpaces + kListIndentUnit));
        }
    }
}

} // namespace

ParsedMarkdown MarkdownParser::parse(const QString& markdown) const {
    QString input = markdown;
    QTextStream stream(&input, QIODeviceBase::ReadOnly);

    MD::Parser parser;
    ParsedMarkdown result;
    result.document = parser.parse(stream, QString(), QStringLiteral("memory.md"));

    if (!result.document) {
        return result;
    }

    const auto& items = result.document->items();
    result.blocks.reserve(items.size());

    for (const auto& itemPtr : items) {
        const MD::Item* item = itemPtr.data();
        if (!item || item->isNullPositions()) {
            continue;
        }

        if (item->type() == MD::ItemType::List) {
            appendListItems(static_cast<const MD::List*>(item), result.blocks, 0);
            continue;
        }

        result.blocks.push_back(makeParsedBlock(item));
    }

    return result;
}

BlockType MarkdownParser::blockTypeForItem(const MD::Item* item) {
    if (!item) {
        return BlockType::Unknown;
    }

    switch (item->type()) {
    case MD::ItemType::Heading:
        return BlockType::Heading;
    case MD::ItemType::Paragraph:
        return BlockType::Paragraph;
    case MD::ItemType::Blockquote:
        return BlockType::Quote;
    case MD::ItemType::ListItem:
        return BlockType::BulletListItem;
    case MD::ItemType::List:
        return BlockType::BulletListItem;
    case MD::ItemType::Code:
        return BlockType::CodeFence;
    case MD::ItemType::Table:
        return BlockType::Table;
    case MD::ItemType::HorizontalLine:
        return BlockType::HorizontalRule;
    case MD::ItemType::Image:
        return BlockType::Image;
    case MD::ItemType::Math:
        return BlockType::Math;
    case MD::ItemType::RawHtml:
        return BlockType::RawHtml;
    case MD::ItemType::Text:
    case MD::ItemType::LineBreak:
    case MD::ItemType::Link:
    case MD::ItemType::TableCell:
    case MD::ItemType::TableRow:
    case MD::ItemType::FootnoteRef:
    case MD::ItemType::Footnote:
    case MD::ItemType::Document:
    case MD::ItemType::PageBreak:
    case MD::ItemType::Anchor:
    case MD::ItemType::UserDefined:
        return BlockType::Unknown;
    }

    return BlockType::Unknown;
}

bool parseImageBlock(const QString& content, ImageBlockInfo* out) {
    const QString line = content.trimmed();
    if (line.isEmpty() || line.contains(QLatin1Char('\n'))) {
        return false;
    }

    // An optional trailing Pandoc attribute block: `{ width=.. height=.. }`.
    static const QString attrSuffix = QStringLiteral("(?:\\s*\\{([^}]*)\\})?");

    // ![alt](url "optional title") — the alt and title allow any chars except the
    // closing delimiter; the url is a whitespace-free token.
    static const QRegularExpression standalone(
        QStringLiteral("^!\\[([^\\]]*)\\]\\((\\S+?)(?:\\s+\"([^\"]*)\")?\\)") + attrSuffix +
        QStringLiteral("$"));
    // [![alt](img "t")](page "t") — a link whose label is solely an image.
    static const QRegularExpression linked(
        QStringLiteral("^\\[!\\[([^\\]]*)\\]\\((\\S+?)(?:\\s+\"([^\"]*)\")?\\)\\]\\((\\S+?)(?:\\s+"
                       "\"([^\"]*)\")?\\)") +
        attrSuffix + QStringLiteral("$"));

    const QRegularExpressionMatch linkedMatch = linked.match(line);
    if (linkedMatch.hasMatch()) {
        if (out) {
            out->alt = linkedMatch.captured(1);
            out->url = linkedMatch.captured(2);
            out->title = linkedMatch.captured(3);
            out->link = linkedMatch.captured(4);
            const QString attrs = linkedMatch.captured(6);
            out->width = imageAttribute(attrs, QStringLiteral("width"));
            out->height = imageAttribute(attrs, QStringLiteral("height"));
        }
        return true;
    }

    const QRegularExpressionMatch standaloneMatch = standalone.match(line);
    if (standaloneMatch.hasMatch()) {
        if (out) {
            out->alt = standaloneMatch.captured(1);
            out->url = standaloneMatch.captured(2);
            out->title = standaloneMatch.captured(3);
            out->link.clear();
            const QString attrs = standaloneMatch.captured(4);
            out->width = imageAttribute(attrs, QStringLiteral("width"));
            out->height = imageAttribute(attrs, QStringLiteral("height"));
        }
        return true;
    }

    return false;
}

QString imageAttribute(const QString& attrs, const QString& key) {
    if (attrs.isEmpty()) {
        return QString();
    }
    const QRegularExpression re(QStringLiteral("(?:^|[\\s{])") + QRegularExpression::escape(key) +
                                QStringLiteral("\\s*=\\s*\"?([^\\s\"}]+)\"?"));
    const QRegularExpressionMatch match = re.match(attrs);
    return match.hasMatch() ? match.captured(1) : QString();
}

qreal imageDimensionValue(const QString& raw, bool* percent) {
    if (percent) {
        *percent = false;
    }
    const QString trimmed = raw.trimmed();
    if (trimmed.isEmpty()) {
        return 0.0;
    }

    static const QRegularExpression re(
        QStringLiteral("^([0-9]*\\.?[0-9]+)\\s*(px|cm|mm|in|inch|%)?$"));
    const QRegularExpressionMatch match = re.match(trimmed);
    if (!match.hasMatch()) {
        return 0.0;
    }

    bool ok = false;
    const double value = match.captured(1).toDouble(&ok);
    if (!ok) {
        return 0.0;
    }
    const QString unit = match.captured(2);

    if (unit == QStringLiteral("%")) {
        if (percent) {
            *percent = true;
        }
        return value;
    }

    // Physical units -> logical pixels at the CSS-standard 96 dpi.
    if (unit == QStringLiteral("cm")) {
        return value / 2.54 * 96.0;
    }
    if (unit == QStringLiteral("mm")) {
        return value / 25.4 * 96.0;
    }
    if (unit == QStringLiteral("in") || unit == QStringLiteral("inch")) {
        return value * 96.0;
    }
    return value; // unitless or "px"
}

DirtyWindow MarkdownParser::dirtyWindowForEdit(qsizetype blockIndex, const QString& inserted,
                                               const QString& removed, qsizetype blockCount) {
    const bool boundaryEdit =
        inserted.contains(QLatin1Char('\n')) || removed.contains(QLatin1Char('\n'));
    const bool markerEdit =
        inserted.contains(QLatin1Char('#')) || inserted.contains(QLatin1Char('>')) ||
        inserted.contains(QLatin1Char('`')) || inserted.contains(QStringLiteral("- ")) ||
        removed.contains(QLatin1Char('#')) || removed.contains(QLatin1Char('>')) ||
        removed.contains(QLatin1Char('`'));

    DirtyWindow window;
    window.structural = boundaryEdit || markerEdit;
    window.startBlock = window.structural ? qMax<qsizetype>(0, blockIndex - 2) : blockIndex;
    window.endBlock = window.structural ? qMin<qsizetype>(blockCount, blockIndex + 3)
                                        : qMin<qsizetype>(blockCount, blockIndex + 1);
    return window;
}

} // namespace be
