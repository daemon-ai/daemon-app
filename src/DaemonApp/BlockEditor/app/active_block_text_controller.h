#pragma once

#include "app/active_markdown_highlighter.h"

#include <QObject>
#include <QPointer>
#include <QtQmlIntegration/qqmlintegration.h>

QT_BEGIN_NAMESPACE
class QQuickTextDocument;
class QTextBlock;
class QTextDocument;
QT_END_NAMESPACE

namespace be::app {

class ActiveBlockTextController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("ActiveBlockTextController is owned by EditorController")

public:
    explicit ActiveBlockTextController(QObject* parent = nullptr);

    Q_INVOKABLE void attach(QObject* textDocument);
    void attachPlainDocument(QTextDocument* document);
    Q_INVOKABLE void detach(QObject* textDocument);
    Q_INVOKABLE bool hasDocument() const;
    Q_INVOKABLE bool isOnFirstVisualLine(int cursorPosition) const;
    Q_INVOKABLE bool isOnLastVisualLine(int cursorPosition) const;
    Q_INVOKABLE qreal cursorVisualX(int cursorPosition) const;
    Q_INVOKABLE int cursorAtFirstVisualLine(qreal visualX) const;
    Q_INVOKABLE int cursorAtLastVisualLine(qreal visualX) const;
    // Stateless geometry query for passive selection painting: returns one
    // {x, y, width, height} rectangle per wrapped line covered by
    // [visualStart, visualEnd] in the given block's own QQuickTextDocument.
    // Coordinates are in document-layout space and include only the document
    // margin; the caller adds the TextEdit's leftPadding/topPadding. Does not
    // touch the active attach state.
    Q_INVOKABLE QVariantList lineRectsForRange(QObject* textDocument, int visualStart,
                                               int visualEnd) const;
    // Same computation as lineRectsForRange, on a raw QTextDocument (testable
    // without a QQuickTextDocument).
    static QVariantList lineRects(QTextDocument* doc, int visualStart, int visualEnd);
    Q_INVOKABLE void applyMarkdownFormatting(const QString& markdown, int blockType,
                                             int headingLevel);
    Q_INVOKABLE void clearFormatting();
    Q_INVOKABLE int rehighlightCount() const;

private:
    struct LineInfo {
        int blockPosition = 0;
        int lineIndex = 0;
        int start = 0;
        int length = 0;
        qreal x = 0.0;
        qreal y = 0.0;
        qreal width = 0.0;
        qreal height = 0.0;
    };

    QTextDocument* document() const;
    bool attachHighlighter(QTextDocument* document);
    static QVector<LineInfo> visualLinesOf(QTextDocument* doc);
    QVector<LineInfo> visualLines() const;
    int lineIndexForCursor(int cursorPosition) const;
    int cursorAtLineVisualX(const LineInfo& line, qreal visualX) const;

    QPointer<QQuickTextDocument> m_textDocument;
    QPointer<QTextDocument> m_plainDocument;
    ActiveMarkdownHighlighter m_highlighter;
    int m_highlightBlockType = -1;
    int m_highlightHeadingLevel = -1;
    int m_rehighlightCount = 0;
    bool m_highlightDocumentDirty = false;
};

} // namespace be::app
