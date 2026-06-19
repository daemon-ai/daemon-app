#include "core/inline_projector.h"

#include "core/image_url.h"
#include "core/markdown_parser.h"

#include <QRegularExpression>

namespace be {

// Object replacement char: occupies one visual position for an inline image so
// the visual<->raw offset maps stay aligned with the rendered <img>.
static constexpr QChar kImagePlaceholder = QChar(0xFFFC);

// Return the url of the first ![alt](url) image inside [start, end), or empty.
static QString firstImageUrlIn(const QString &raw, qsizetype start, qsizetype end);

// Find the ']' that matches the '[' at openIndex, counting nested brackets so a
// label that contains images (![..]) or other bracket pairs resolves to its true
// close. Returns -1 if unbalanced within [openIndex, limit).
static qsizetype matchClosingBracket(const QString &raw, qsizetype openIndex, qsizetype limit)
{
    int depth = 0;
    for (qsizetype k = openIndex; k < limit; ++k) {
        const QChar c = raw.at(k);
        if (c == QLatin1Char('[')) {
            ++depth;
        } else if (c == QLatin1Char(']') && --depth == 0) {
            return k;
        }
    }
    return -1;
}

static QString firstImageUrlIn(const QString &raw, qsizetype start, qsizetype end)
{
    for (qsizetype k = start; k < end; ++k) {
        if (raw.at(k) == QLatin1Char('!') && k + 1 < end && raw.at(k + 1) == QLatin1Char('[')) {
            const qsizetype altClose = matchClosingBracket(raw, k + 1, end);
            if (altClose > k + 1 && altClose + 1 < end && raw.at(altClose + 1) == QLatin1Char('(')) {
                const qsizetype urlClose = raw.indexOf(QLatin1Char(')'), altClose + 2);
                if (urlClose > altClose + 2 && urlClose < end) {
                    return raw.mid(altClose + 2, urlClose - (altClose + 2));
                }
            }
        }
    }
    return QString();
}

qsizetype BlockProjection::visualToRaw(qsizetype visualOffset) const
{
    if (visualToRawUtf16.isEmpty()) {
        return 0;
    }
    visualOffset = qBound<qsizetype>(0, visualOffset, visualToRawUtf16.size() - 1);
    return visualToRawUtf16[visualOffset];
}

qsizetype BlockProjection::rawToVisual(qsizetype rawOffset) const
{
    if (rawToVisualUtf16.isEmpty()) {
        return 0;
    }
    rawOffset = qBound<qsizetype>(0, rawOffset, rawToVisualUtf16.size() - 1);
    return rawToVisualUtf16[rawOffset];
}

BlockProjection InlineProjector::project(const BlockRecord &block, bool revealMarkdown, qsizetype rawCursor,
                                         qreal contentWidth) const
{
    const QString raw = block.markdown();
    BlockProjection projection;
    projection.blockId = block.id;
    projection.blockRevision = block.revision;
    projection.rawToVisualUtf16.resize(raw.size() + 1);

    const bool revealAll = revealMarkdown || rawCursor >= 0;
    qsizetype contentStart = 0;
    qsizetype contentEnd = raw.size();

    if (!revealAll && block.type == BlockType::Heading) {
        const QRegularExpression heading(QStringLiteral("^(#{1,6})\\s+"));
        const QRegularExpressionMatch match = heading.match(raw);
        if (match.hasMatch()) {
            contentStart = match.capturedEnd(0);
        }
    } else if (!revealAll && block.type == BlockType::TaskListItem) {
        const QRegularExpression task(QStringLiteral("^\\s*[-*+]\\s+\\[([ xX])\\]\\s+"));
        const QRegularExpressionMatch match = task.match(raw);
        if (match.hasMatch()) {
            appendPresentation(projection, match.captured(1).trimmed().isEmpty() ? QStringLiteral("[ ] ") : QStringLiteral("[x] "), match.capturedEnd(0));
            contentStart = match.capturedEnd(0);
        }
    } else if (!revealAll && (block.type == BlockType::BulletListItem || block.type == BlockType::OrderedListItem)) {
        const QRegularExpression list(QStringLiteral("^\\s*((?:[-*+])|(?:\\d+[.)]))\\s+"));
        const QRegularExpressionMatch match = list.match(raw);
        if (match.hasMatch()) {
            appendPresentation(projection, QStringLiteral("- "), match.capturedEnd(0));
            contentStart = match.capturedEnd(0);
        }
    }

    while (contentEnd > contentStart && raw.at(contentEnd - 1).isSpace()) {
        --contentEnd;
    }

    for (qsizetype rawOffset = 0; rawOffset <= contentStart && rawOffset < projection.rawToVisualUtf16.size(); ++rawOffset) {
        projection.rawToVisualUtf16[rawOffset] = projection.visualText.size();
    }

    const auto hasWhitespace = [](const QString &s) {
        for (const QChar c : s) {
            if (c.isSpace()) {
                return true;
            }
        }
        return false;
    };

    qsizetype i = contentStart;
    while (i < contentEnd) {
        // Inline image ![alt](url): rendered as an <img> in display markup and
        // collapsed to a single placeholder char in the visual text. Matched
        // before the link branch so a bare image is not mistaken for a link.
        if (!revealAll && raw.at(i) == QLatin1Char('!') && i + 1 < contentEnd
            && raw.at(i + 1) == QLatin1Char('[')) {
            const qsizetype altClose = matchClosingBracket(raw, i + 1, contentEnd);
            if (altClose > i + 1 && altClose + 1 < contentEnd && raw.at(altClose + 1) == QLatin1Char('(')) {
                const qsizetype urlClose = raw.indexOf(QLatin1Char(')'), altClose + 2);
                if (urlClose > altClose + 2 && urlClose < contentEnd) {
                    const QString url = raw.mid(altClose + 2, urlClose - (altClose + 2));
                    if (!hasWhitespace(url)) {
                        // Consume an optional Pandoc attribute block: ){ width=.. }.
                        qsizetype spanEnd = urlClose + 1;
                        qreal width = 0;
                        qreal height = 0;
                        qsizetype scan = spanEnd;
                        while (scan < contentEnd && raw.at(scan).isSpace()) {
                            ++scan;
                        }
                        if (scan < contentEnd && raw.at(scan) == QLatin1Char('{')) {
                            const qsizetype attrClose = raw.indexOf(QLatin1Char('}'), scan + 1);
                            if (attrClose > scan && attrClose < contentEnd) {
                                const QString attrs = raw.mid(scan, attrClose - scan + 1);
                                // The rich-text engine renders <img> at absolute px
                                // (it ignores percent widths, collapsing them to ~0),
                                // so a percent width is resolved here against the flow
                                // column. Without a column basis it is dropped. A
                                // percent height has no basis at all and is ignored.
                                bool widthPct = false;
                                bool heightPct = false;
                                width = imageDimensionValue(imageAttribute(attrs, QStringLiteral("width")), &widthPct);
                                height = imageDimensionValue(imageAttribute(attrs, QStringLiteral("height")), &heightPct);
                                if (widthPct) {
                                    width = contentWidth > 0.0 ? contentWidth * width / 100.0 : 0.0;
                                }
                                if (heightPct) {
                                    height = 0;
                                }
                                spanEnd = attrClose + 1;
                            }
                        }
                        appendImage(projection, i, spanEnd, url, width, height);
                        i = spanEnd;
                        continue;
                    }
                }
            }
        }

        // Links are matched before emphasis/code so a '*' or '`' living inside a
        // label or URL cannot be consumed first. The label becomes the visual
        // text while the span's raw range covers the whole link, so selecting the
        // rendered label copies the full Markdown syntax.
        if (!revealAll && raw.at(i) == QLatin1Char('[') && !(i > 0 && raw.at(i - 1) == QLatin1Char('!'))) {
            const qsizetype labelClose = matchClosingBracket(raw, i, contentEnd);
            if (labelClose > i + 1 && labelClose + 1 < contentEnd && raw.at(labelClose + 1) == QLatin1Char('(')) {
                const qsizetype urlClose = raw.indexOf(QLatin1Char(')'), labelClose + 2);
                if (urlClose > labelClose + 2 && urlClose < contentEnd) {
                    const QString url = raw.mid(labelClose + 2, urlClose - (labelClose + 2));
                    if (!hasWhitespace(url)) {
                        // The label can itself contain images/emphasis; reduce it to
                        // visible text (images -> alt). An image-only label collapses
                        // to empty, so fall back to the url to keep the link clickable.
                        QString label = stripInlineMarkup(raw, i + 1, labelClose);
                        const QString favicon = firstImageUrlIn(raw, i + 1, labelClose);
                        if (label.isEmpty()) {
                            label = url;
                        }
                        appendLink(projection, i, urlClose + 1, label, url, favicon);
                        i = urlClose + 1;
                        continue;
                    }
                }
            }
        }

        if (!revealAll && raw.at(i) == QLatin1Char('<')) {
            const qsizetype close = raw.indexOf(QLatin1Char('>'), i + 1);
            if (close > i + 1 && close < contentEnd) {
                const QString inner = raw.mid(i + 1, close - (i + 1));
                if (!hasWhitespace(inner) && (inner.contains(QStringLiteral("://")) || inner.startsWith(QStringLiteral("mailto:")))) {
                    appendLink(projection, i, close + 1, inner, inner);
                    i = close + 1;
                    continue;
                }
            }
        }

        if (!revealAll && raw.mid(i, 2) == QStringLiteral("**")) {
            const qsizetype close = raw.indexOf(QStringLiteral("**"), i + 2);
            if (close > i + 2 && close < contentEnd) {
                appendVisual(projection, raw, i + 2, close - i - 2, SpanKind::Strong);
                i = close + 2;
                continue;
            }
        }

        if (!revealAll && raw.at(i) == QLatin1Char('*')) {
            const qsizetype close = raw.indexOf(QLatin1Char('*'), i + 1);
            if (close > i + 1 && close < contentEnd) {
                appendVisual(projection, raw, i + 1, close - i - 1, SpanKind::Emphasis);
                i = close + 1;
                continue;
            }
        }

        if (!revealAll && raw.at(i) == QLatin1Char('`')) {
            const qsizetype close = raw.indexOf(QLatin1Char('`'), i + 1);
            if (close > i + 1 && close < contentEnd) {
                appendVisual(projection, raw, i + 1, close - i - 1, SpanKind::Code);
                i = close + 1;
                continue;
            }
        }

        appendVisual(projection, raw, i, 1, SpanKind::Plain);
        ++i;
    }

    for (qsizetype rawOffset = 0; rawOffset < projection.rawToVisualUtf16.size(); ++rawOffset) {
        if (projection.rawToVisualUtf16[rawOffset] == 0 && rawOffset > 0) {
            projection.rawToVisualUtf16[rawOffset] = projection.rawToVisualUtf16[rawOffset - 1];
        }
    }

    projection.visualToRawUtf16.resize(projection.visualText.size() + 1);
    for (const InlineSpan &span : projection.spans) {
        const qsizetype rawLength = std::max<qsizetype>(1, span.rawEnd - span.rawStart);
        const qsizetype visualLength = std::max<qsizetype>(1, span.visualEnd - span.visualStart);
        for (qsizetype v = span.visualStart; v <= span.visualEnd && v < projection.visualToRawUtf16.size(); ++v) {
            const qsizetype rawDelta = qRound64(double(v - span.visualStart) / double(visualLength) * double(rawLength));
            projection.visualToRawUtf16[v] = qBound<qsizetype>(span.rawStart, span.rawStart + rawDelta, span.rawEnd);
        }
    }
    if (!projection.visualToRawUtf16.isEmpty()) {
        projection.visualToRawUtf16.last() = raw.size();
    }

    projection.displayMarkup = makeDisplayMarkup(block, projection);
    return projection;
}

QString InlineProjector::makeDisplayMarkup(const BlockRecord &block, const BlockProjection &projection) const
{
    QString escaped;
    qsizetype cursor = 0;
    for (const InlineSpan &span : projection.spans) {
        if (span.visualStart > cursor) {
            escaped += projection.visualText.mid(cursor, span.visualStart - cursor).toHtmlEscaped();
        }

        QString text = projection.visualText.mid(span.visualStart, span.visualEnd - span.visualStart).toHtmlEscaped();
        text.replace(QLatin1Char('\n'), QStringLiteral("<br/>"));
        switch (span.kind) {
        case SpanKind::Strong:
            escaped += QStringLiteral("<b>%1</b>").arg(text);
            break;
        case SpanKind::Emphasis:
            escaped += QStringLiteral("<i>%1</i>").arg(text);
            break;
        case SpanKind::Code: {
            QString codeStyle = QStringLiteral("font-family:%1; background-color:%2;")
                                    .arg(m_palette.monoFamily, m_palette.codeBackground.name());
            if (m_palette.codeText.isValid()) {
                codeStyle += QStringLiteral(" color:%1;").arg(m_palette.codeText.name());
            }
            escaped += QStringLiteral("<span style=\"%1\">%2</span>").arg(codeStyle, text);
            break;
        }
        case SpanKind::Link: {
            QString inner;
            if (!span.imageUrl.isEmpty()) {
                inner += QStringLiteral("<img src=\"%1\" height=\"16\" style=\"vertical-align:middle;\"> ")
                             .arg(be::resolveImageSource(span.imageUrl).toHtmlEscaped());
            }
            inner += QStringLiteral("<span style=\"color:%1; text-decoration:underline;\">%2</span>")
                         .arg(m_palette.link.name(), text);
            escaped += QStringLiteral("<a href=\"%1\">%2</a>").arg(span.url.toHtmlEscaped(), inner);
            break;
        }
        case SpanKind::Image: {
            QString img = QStringLiteral("<img src=\"%1\"")
                              .arg(be::resolveImageSource(span.imageUrl).toHtmlEscaped());
            if (span.imageWidth > 0) {
                img += QStringLiteral(" width=\"%1\"").arg(qRound(span.imageWidth));
            }
            if (span.imageHeight > 0) {
                img += QStringLiteral(" height=\"%1\"").arg(qRound(span.imageHeight));
            }
            img += QStringLiteral(">");
            escaped += img;
            break;
        }
        case SpanKind::Plain:
        case SpanKind::HiddenDelimiter:
            escaped += text;
            break;
        }
        cursor = span.visualEnd;
    }
    if (cursor < projection.visualText.size()) {
        escaped += projection.visualText.mid(cursor).toHtmlEscaped();
    }
    escaped.replace(QLatin1Char('\n'), QStringLiteral("<br/>"));

    if (block.type == BlockType::Heading) {
        const int size = qMax(18, 30 - int(block.headingLevel) * 2);
        QString headingStyle = QStringLiteral("font-size:%1px; font-weight:700;").arg(size);
        if (m_palette.text.isValid()) {
            headingStyle += QStringLiteral(" color:%1;").arg(m_palette.text.name());
        }
        return QStringLiteral("<span style=\"%1\">%2</span>").arg(headingStyle, escaped);
    }
    if (block.type == BlockType::CodeFence) {
        QString fenceStyle = QStringLiteral("font-family:%1;").arg(m_palette.monoFamily);
        if (m_palette.codeText.isValid()) {
            fenceStyle += QStringLiteral(" color:%1;").arg(m_palette.codeText.name());
        }
        return QStringLiteral("<span style=\"%1\">%2</span>").arg(fenceStyle, escaped);
    }
    return escaped;
}

QString InlineProjector::stripInlineMarkup(const QString &raw, qsizetype start, qsizetype end) const
{
    QString out;
    qsizetype k = start;
    while (k < end) {
        // Image ![alt](url) -> alt (recursively stripped). The favicon images in
        // a link label have empty alt, so they vanish from the visible text.
        if (raw.at(k) == QLatin1Char('!') && k + 1 < end && raw.at(k + 1) == QLatin1Char('[')) {
            const qsizetype altClose = matchClosingBracket(raw, k + 1, end);
            if (altClose > k + 1 && altClose + 1 < end && raw.at(altClose + 1) == QLatin1Char('(')) {
                const qsizetype urlClose = raw.indexOf(QLatin1Char(')'), altClose + 2);
                if (urlClose > altClose + 2 && urlClose < end) {
                    out += stripInlineMarkup(raw, k + 2, altClose);
                    k = urlClose + 1;
                    continue;
                }
            }
        }

        // Nested [text](url) -> text (CommonMark forbids nested links, but reduce
        // leniently so the visible text stays clean).
        if (raw.at(k) == QLatin1Char('[')) {
            const qsizetype labelClose = matchClosingBracket(raw, k, end);
            if (labelClose > k + 1 && labelClose + 1 < end && raw.at(labelClose + 1) == QLatin1Char('(')) {
                const qsizetype urlClose = raw.indexOf(QLatin1Char(')'), labelClose + 2);
                if (urlClose > labelClose + 2 && urlClose < end) {
                    out += stripInlineMarkup(raw, k + 1, labelClose);
                    k = urlClose + 1;
                    continue;
                }
            }
        }

        if (raw.mid(k, 2) == QStringLiteral("**")) {
            const qsizetype close = raw.indexOf(QStringLiteral("**"), k + 2);
            if (close > k + 2 && close < end) {
                out += stripInlineMarkup(raw, k + 2, close);
                k = close + 2;
                continue;
            }
        }

        if (raw.at(k) == QLatin1Char('*')) {
            const qsizetype close = raw.indexOf(QLatin1Char('*'), k + 1);
            if (close > k + 1 && close < end) {
                out += stripInlineMarkup(raw, k + 1, close);
                k = close + 1;
                continue;
            }
        }

        if (raw.at(k) == QLatin1Char('`')) {
            const qsizetype close = raw.indexOf(QLatin1Char('`'), k + 1);
            if (close > k + 1 && close < end) {
                out += raw.mid(k + 1, close - (k + 1));
                k = close + 1;
                continue;
            }
        }

        out += raw.at(k);
        ++k;
    }
    return out;
}

void InlineProjector::appendVisual(BlockProjection &projection, const QString &raw, qsizetype rawIndex, qsizetype length, SpanKind kind) const
{
    const qsizetype visualStart = projection.visualText.size();
    const QString text = raw.mid(rawIndex, length);
    projection.visualText += text;
    const qsizetype visualEnd = projection.visualText.size();

    InlineSpan span;
    span.kind = kind;
    span.rawStart = rawIndex;
    span.rawEnd = rawIndex + length;
    span.visualStart = visualStart;
    span.visualEnd = visualEnd;
    projection.spans.push_back(span);

    for (qsizetype rawOffset = rawIndex; rawOffset <= rawIndex + length && rawOffset < projection.rawToVisualUtf16.size(); ++rawOffset) {
        projection.rawToVisualUtf16[rawOffset] = visualStart + qBound<qsizetype>(0, rawOffset - rawIndex, text.size());
    }
}

void InlineProjector::appendImage(BlockProjection &projection,
                                  qsizetype rawStart, qsizetype rawEnd, const QString &url,
                                  qreal width, qreal height) const
{
    const qsizetype visualStart = projection.visualText.size();
    projection.visualText += kImagePlaceholder;
    const qsizetype visualEnd = projection.visualText.size();

    InlineSpan span;
    span.kind = SpanKind::Image;
    span.rawStart = rawStart;
    span.rawEnd = rawEnd;
    span.visualStart = visualStart;
    span.visualEnd = visualEnd;
    span.imageUrl = url;
    span.imageWidth = width;
    span.imageHeight = height;
    projection.spans.push_back(span);

    // The whole raw image range maps onto the single placeholder position.
    for (qsizetype rawOffset = rawStart; rawOffset <= rawEnd && rawOffset < projection.rawToVisualUtf16.size(); ++rawOffset) {
        projection.rawToVisualUtf16[rawOffset] = visualStart;
    }
}

void InlineProjector::appendLink(BlockProjection &projection,
                                 qsizetype linkRawStart, qsizetype linkRawEnd,
                                 const QString &label, const QString &url,
                                 const QString &imageUrl) const
{
    const qsizetype visualStart = projection.visualText.size();
    projection.visualText += label;
    const qsizetype visualEnd = projection.visualText.size();

    InlineSpan span;
    span.kind = SpanKind::Link;
    span.rawStart = linkRawStart;
    span.rawEnd = linkRawEnd;
    span.visualStart = visualStart;
    span.visualEnd = visualEnd;
    span.url = url;
    span.imageUrl = imageUrl;
    projection.spans.push_back(span);

    // Map the whole raw link range (brackets + hidden url) into the label's
    // visual range, clamped, so raw offsets inside the link resolve to the label
    // and the maps stay monotonic.
    const qsizetype visualLength = std::max<qsizetype>(1, visualEnd - visualStart);
    const qsizetype rawLength = std::max<qsizetype>(1, linkRawEnd - linkRawStart);
    for (qsizetype rawOffset = linkRawStart; rawOffset <= linkRawEnd && rawOffset < projection.rawToVisualUtf16.size(); ++rawOffset) {
        const qsizetype visualDelta = qRound64(double(rawOffset - linkRawStart) / double(rawLength) * double(visualLength));
        projection.rawToVisualUtf16[rawOffset] = visualStart + qBound<qsizetype>(0, visualDelta, visualLength);
    }
}

void InlineProjector::appendPresentation(BlockProjection &projection, const QString &text, qsizetype rawAffinity) const
{
    const qsizetype visualStart = projection.visualText.size();
    projection.visualText += text;
    const qsizetype visualEnd = projection.visualText.size();

    InlineSpan span;
    span.kind = SpanKind::Plain;
    span.rawStart = rawAffinity;
    span.rawEnd = rawAffinity;
    span.visualStart = visualStart;
    span.visualEnd = visualEnd;
    projection.spans.push_back(span);
}

} // namespace be
