#include "app/active_block_text_controller.h"

#include <QQuickTextDocument>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>
#include <QVariantList>
#include <QVariantMap>

namespace be::app {

ActiveBlockTextController::ActiveBlockTextController(QObject *parent)
    : QObject(parent)
{
}

void ActiveBlockTextController::attach(QObject *textDocument)
{
    m_textDocument = qobject_cast<QQuickTextDocument *>(textDocument);
    m_plainDocument.clear();
    attachHighlighter(document());
}

void ActiveBlockTextController::attachPlainDocument(QTextDocument *document)
{
    m_plainDocument = document;
    m_textDocument.clear();
    attachHighlighter(document);
}

void ActiveBlockTextController::detach(QObject *textDocument)
{
    if (!textDocument || textDocument == m_textDocument) {
        clearFormatting();
        m_textDocument.clear();
        m_plainDocument.clear();
    }
}

bool ActiveBlockTextController::hasDocument() const
{
    return document() != nullptr;
}

bool ActiveBlockTextController::isOnFirstVisualLine(int cursorPosition) const
{
    const QVector<LineInfo> lines = visualLines();
    if (lines.isEmpty()) {
        return true;
    }
    return lineIndexForCursor(cursorPosition) == 0;
}

bool ActiveBlockTextController::isOnLastVisualLine(int cursorPosition) const
{
    const QVector<LineInfo> lines = visualLines();
    if (lines.isEmpty()) {
        return true;
    }
    return lineIndexForCursor(cursorPosition) == lines.size() - 1;
}

qreal ActiveBlockTextController::cursorVisualX(int cursorPosition) const
{
    QTextDocument *doc = document();
    if (!doc) {
        return 0.0;
    }

    const QTextBlock block = doc->findBlock(cursorPosition);
    if (!block.isValid() || !block.layout()) {
        return 0.0;
    }

    const int localPosition = cursorPosition - block.position();
    QTextLayout *layout = block.layout();
    for (int i = 0; i < layout->lineCount(); ++i) {
        const QTextLine line = layout->lineAt(i);
        if (localPosition >= line.textStart() && localPosition <= line.textStart() + line.textLength()) {
            return line.cursorToX(localPosition);
        }
    }

    return 0.0;
}

int ActiveBlockTextController::cursorAtFirstVisualLine(qreal visualX) const
{
    const QVector<LineInfo> lines = visualLines();
    if (lines.isEmpty()) {
        return 0;
    }
    return cursorAtLineVisualX(lines.first(), visualX);
}

int ActiveBlockTextController::cursorAtLastVisualLine(qreal visualX) const
{
    const QVector<LineInfo> lines = visualLines();
    if (lines.isEmpty()) {
        QTextDocument *doc = document();
        return doc ? qMax(0, doc->characterCount() - 1) : 0;
    }
    return cursorAtLineVisualX(lines.last(), visualX);
}

void ActiveBlockTextController::applyMarkdownFormatting(const QString &markdown, int blockType, int headingLevel)
{
    Q_UNUSED(markdown)

    QTextDocument *doc = document();
    if (!doc) {
        return;
    }

    const bool documentChanged = attachHighlighter(doc) || m_highlightDocumentDirty;
    const bool contextChanged = m_highlightBlockType != blockType || m_highlightHeadingLevel != headingLevel;
    if (!documentChanged && !contextChanged) {
        return;
    }

    m_highlightBlockType = blockType;
    m_highlightHeadingLevel = headingLevel;
    m_highlighter.setContext(blockType, headingLevel);
    m_highlighter.rehighlight();
    m_highlightDocumentDirty = false;
    ++m_rehighlightCount;
}

void ActiveBlockTextController::clearFormatting()
{
    m_highlighter.setDocument(nullptr);
    m_highlightBlockType = -1;
    m_highlightHeadingLevel = -1;
    m_highlightDocumentDirty = false;
}

int ActiveBlockTextController::rehighlightCount() const
{
    return m_rehighlightCount;
}

QVariantList ActiveBlockTextController::lineRectsForRange(QObject *textDocument, int visualStart, int visualEnd) const
{
    auto *quickDocument = qobject_cast<QQuickTextDocument *>(textDocument);
    QTextDocument *doc = quickDocument ? quickDocument->textDocument() : nullptr;
    return lineRects(doc, visualStart, visualEnd);
}

QVariantList ActiveBlockTextController::lineRects(QTextDocument *doc, int visualStart, int visualEnd)
{
    QVariantList rects;
    if (!doc) {
        return rects;
    }

    int start = qMin(visualStart, visualEnd);
    int end = qMax(visualStart, visualEnd);
    const int maxPosition = qMax(0, doc->characterCount() - 1);
    start = qBound(0, start, maxPosition);
    end = qBound(0, end, maxPosition);
    if (start >= end) {
        return rects;
    }

    const qreal margin = doc->documentMargin();

    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        QTextLayout *layout = block.layout();
        if (!layout) {
            continue;
        }

        const int blockStart = block.position();
        const qreal layoutY = layout->position().y();
        for (int i = 0; i < layout->lineCount(); ++i) {
            const QTextLine line = layout->lineAt(i);
            const int lineGlobalStart = blockStart + line.textStart();
            const int lineGlobalEnd = lineGlobalStart + line.textLength();

            const int from = qMax(start, lineGlobalStart);
            const int to = qMin(end, lineGlobalEnd);
            if (from >= to) {
                continue;
            }

            // cursorToX is computed on this specific line, so a selection end at
            // a soft-wrap boundary resolves to this line's right edge (not the
            // next line's start, as TextEdit.positionToRectangle would).
            const qreal x0 = line.cursorToX(from - blockStart);
            const qreal x1 = line.cursorToX(to - blockStart);
            const qreal left = qMin(x0, x1);
            const qreal right = qMax(x0, x1);

            QVariantMap rect;
            rect.insert(QStringLiteral("x"), left + margin);
            rect.insert(QStringLiteral("y"), layoutY + line.y() + margin);
            rect.insert(QStringLiteral("width"), qMax<qreal>(1.0, right - left));
            rect.insert(QStringLiteral("height"), line.height());
            rects.push_back(rect);
        }
    }

    return rects;
}

QTextDocument *ActiveBlockTextController::document() const
{
    if (m_plainDocument) {
        return m_plainDocument;
    }
    return m_textDocument ? m_textDocument->textDocument() : nullptr;
}

bool ActiveBlockTextController::attachHighlighter(QTextDocument *document)
{
    if (m_highlighter.document() != document) {
        m_highlighter.setDocument(document);
        m_highlightDocumentDirty = document != nullptr;
        return true;
    }
    return false;
}

QVector<ActiveBlockTextController::LineInfo> ActiveBlockTextController::visualLines() const
{
    return visualLinesOf(document());
}

QVector<ActiveBlockTextController::LineInfo> ActiveBlockTextController::visualLinesOf(QTextDocument *doc)
{
    QVector<LineInfo> lines;
    if (!doc) {
        return lines;
    }

    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        QTextLayout *layout = block.layout();
        if (!layout) {
            continue;
        }

        for (int i = 0; i < layout->lineCount(); ++i) {
            const QTextLine line = layout->lineAt(i);
            LineInfo info;
            info.blockPosition = block.position();
            info.lineIndex = i;
            info.start = line.textStart();
            info.length = line.textLength();
            info.x = line.x();
            info.y = block.layout()->position().y() + line.y();
            info.width = line.naturalTextWidth();
            info.height = line.height();
            lines.push_back(info);
        }
    }
    return lines;
}

int ActiveBlockTextController::lineIndexForCursor(int cursorPosition) const
{
    const QVector<LineInfo> lines = visualLines();
    if (lines.isEmpty()) {
        return 0;
    }

    QTextDocument *doc = document();
    const int maxPosition = doc ? qMax(0, doc->characterCount() - 1) : 0;
    cursorPosition = qBound(0, cursorPosition, maxPosition);

    for (int i = 0; i < lines.size(); ++i) {
        const LineInfo &line = lines[i];
        const int globalStart = line.blockPosition + line.start;
        const int globalEnd = globalStart + line.length;
        if (cursorPosition >= globalStart && cursorPosition <= globalEnd) {
            return i;
        }
    }

    return cursorPosition <= lines.first().blockPosition + lines.first().start ? 0 : lines.size() - 1;
}

int ActiveBlockTextController::cursorAtLineVisualX(const LineInfo &line, qreal visualX) const
{
    QTextDocument *doc = document();
    if (!doc) {
        return 0;
    }

    const QTextBlock block = doc->findBlock(line.blockPosition);
    if (!block.isValid() || !block.layout() || line.lineIndex >= block.layout()->lineCount()) {
        return qBound(0, line.blockPosition + line.start, qMax(0, doc->characterCount() - 1));
    }

    const QTextLine textLine = block.layout()->lineAt(line.lineIndex);
    const int localPosition = textLine.xToCursor(qMax<qreal>(0.0, visualX));
    return qBound(0, block.position() + localPosition, qMax(0, doc->characterCount() - 1));
}

} // namespace be::app
