#include "app/active_markdown_highlighter.h"

#include "core/block_record.h"

#include <QColor>

namespace be::app {

namespace {

// Find the ']' matching the '[' at openIndex, counting nested brackets so a link
// label containing images (![..]) resolves to its true close. -1 if unbalanced.
qsizetype matchClosingBracket(const QString& text, qsizetype openIndex) {
    int depth = 0;
    for (qsizetype k = openIndex; k < text.size(); ++k) {
        const QChar c = text.at(k);
        if (c == QLatin1Char('[')) {
            ++depth;
        } else if (c == QLatin1Char(']') && --depth == 0) {
            return k;
        }
    }
    return -1;
}

} // namespace

ActiveMarkdownHighlighter::ActiveMarkdownHighlighter(QObject* parent) : QSyntaxHighlighter(parent) {
    m_markerFormat.setForeground(QColor(QStringLiteral("#8a8a8a")));

    m_headingFormat.setFontWeight(QFont::Bold);

    m_strongFormat.setFontWeight(QFont::Bold);

    m_emphasisFormat.setFontItalic(true);

    m_codeFormat.setFontFamilies({QStringLiteral("monospace")});
    m_codeFormat.setBackground(QColor(QStringLiteral("#f1f1ef")));

    m_linkFormat.setForeground(QColor(QStringLiteral("#63b3ed")));
    m_linkFormat.setFontUnderline(true);
}

void ActiveMarkdownHighlighter::setContext(int blockType, int headingLevel) {
    if (m_blockType == blockType && m_headingLevel == headingLevel) {
        return;
    }

    m_blockType = blockType;
    m_headingLevel = headingLevel;
}

void ActiveMarkdownHighlighter::highlightBlock(const QString& text) {
    const bool isHeading = m_blockType == static_cast<int>(be::BlockType::Heading);
    if (isHeading && currentBlock().blockNumber() == 0) {
        setFormat(0, text.size(), m_headingFormat);

        const QRegularExpression heading(QStringLiteral("^(#{1,6})\\s+"));
        const QRegularExpressionMatch match = heading.match(text);
        if (match.hasMatch()) {
            setFormat(match.capturedStart(1), match.capturedLength(1), m_markerFormat);
        }
    }

    const QRegularExpression listMarker(
        QStringLiteral("^\\s*((?:[-*+])|(?:\\d+[.)])|(?:[-*+]\\s+\\[[ xX]\\]))\\s+"));
    const QRegularExpressionMatch listMatch = listMarker.match(text);
    if (listMatch.hasMatch()) {
        setFormat(listMatch.capturedStart(1), listMatch.capturedLength(1), m_markerFormat);
    }

    applyInlineFormats(text);
}

void ActiveMarkdownHighlighter::applyInlineFormats(const QString& text) {
    applyRegexFormat(text, QRegularExpression(QStringLiteral("(\\*\\*)([^*]+)(\\*\\*)")),
                     m_strongFormat, m_markerFormat);
    applyRegexFormat(text, QRegularExpression(QStringLiteral("(?<!\\*)\\*([^*]+)\\*(?!\\*)")),
                     m_emphasisFormat, m_markerFormat);
    applyRegexFormat(text, QRegularExpression(QStringLiteral("(`)([^`]+)(`)")), m_codeFormat,
                     m_markerFormat);
    // Applied last so the link label/url formatting wins over any stray emphasis
    // or code match inside a URL.
    applyLinkFormats(text);
}

void ActiveMarkdownHighlighter::applyLinkFormats(const QString& text) {
    // Inline link [label](url). Balanced-bracket scan (not a regex) so a label
    // containing images (![..]) resolves to its true close; '[' preceded by '!'
    // is skipped so image syntax is not treated as a link.
    qsizetype i = 0;
    while (i < text.size()) {
        if (text.at(i) != QLatin1Char('[') || (i > 0 && text.at(i - 1) == QLatin1Char('!'))) {
            ++i;
            continue;
        }
        const qsizetype labelClose = matchClosingBracket(text, i);
        if (labelClose > i + 1 && labelClose + 1 < text.size() &&
            text.at(labelClose + 1) == QLatin1Char('(')) {
            const qsizetype urlClose = text.indexOf(QLatin1Char(')'), labelClose + 2);
            if (urlClose > labelClose + 2) {
                setFormat(i, 1, m_markerFormat);                                        // '['
                setFormat(i + 1, labelClose - (i + 1), m_linkFormat);                   // label
                setFormat(labelClose, 2, m_markerFormat);                               // ']('
                setFormat(labelClose + 2, urlClose - (labelClose + 2), m_markerFormat); // url
                setFormat(urlClose, 1, m_markerFormat);                                 // ')'
                i = urlClose + 1;
                continue;
            }
        }
        ++i;
    }

    // Autolink <scheme://...> or <mailto:...>.
    const QRegularExpression autolink(
        QStringLiteral("(<)([a-zA-Z][\\w+.-]*://[^>\\s]+|mailto:[^>\\s]+)(>)"));
    QRegularExpressionMatchIterator autoIt = autolink.globalMatch(text);
    while (autoIt.hasNext()) {
        const QRegularExpressionMatch match = autoIt.next();
        setFormat(match.capturedStart(1), match.capturedLength(1), m_markerFormat);
        setFormat(match.capturedStart(2), match.capturedLength(2), m_linkFormat);
        setFormat(match.capturedStart(3), match.capturedLength(3), m_markerFormat);
    }
}

void ActiveMarkdownHighlighter::applyRegexFormat(const QString& text,
                                                 const QRegularExpression& regex,
                                                 const QTextCharFormat& contentFormat,
                                                 const QTextCharFormat& delimiterFormat) {
    QRegularExpressionMatchIterator it = regex.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        if (match.lastCapturedIndex() >= 1) {
            setFormat(match.capturedStart(1), match.capturedLength(1), delimiterFormat);
        }
        if (match.lastCapturedIndex() >= 2) {
            setFormat(match.capturedStart(2), match.capturedLength(2), contentFormat);
        }
        if (match.lastCapturedIndex() >= 3) {
            setFormat(match.capturedStart(3), match.capturedLength(3), delimiterFormat);
        }
    }
}

} // namespace be::app
