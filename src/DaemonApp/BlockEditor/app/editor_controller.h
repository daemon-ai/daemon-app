#pragma once

#include "app/active_block_text_controller.h"
#include "app/block_model.h"
#include "core/block_height_index.h"
#include "core/command_stack.h"
#include "core/inline_projector.h"
#include "core/persistence.h"
#include "core/selection.h"

#include <QColor>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVector>
#include <QtQmlIntegration/qqmlintegration.h>

namespace be::app {

class EditorController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(be::app::BlockModel *blockModel READ blockModel CONSTANT)
    Q_PROPERTY(be::app::ActiveBlockTextController *activeTextController READ activeTextController CONSTANT)
    Q_PROPERTY(qulonglong activeBlockId READ activeBlockId NOTIFY activeBlockIdChanged)
    Q_PROPERTY(int activeBlockRow READ activeBlockRow NOTIFY activeBlockRowChanged)
    Q_PROPERTY(int activeCursorOffset READ activeCursorOffset NOTIFY activeCursorOffsetChanged)
    Q_PROPERTY(qulonglong selectionRevision READ selectionRevision NOTIFY selectionRevisionChanged)
    Q_PROPERTY(bool streaming READ streaming NOTIFY streamingChanged)

    // Theme palette for the rendered RichText. Bind these from QML to Theme.*;
    // each setter reprojects all blocks so a theme switch recolors live.
    Q_PROPERTY(QColor codeBackgroundColor READ codeBackgroundColor WRITE setCodeBackgroundColor NOTIFY paletteChanged)
    Q_PROPERTY(QColor linkColor READ linkColor WRITE setLinkColor NOTIFY paletteChanged)
    Q_PROPERTY(QColor bodyTextColor READ bodyTextColor WRITE setBodyTextColor NOTIFY paletteChanged)
    Q_PROPERTY(QString monoFamily READ monoFamily WRITE setMonoFamily NOTIFY paletteChanged)
    // Editor body font (driven by the Style / Font size settings). The family
    // does not enter the HTML (the block TextEdit's font carries it, so code
    // spans keep monoFamily); the size feeds heading scaling in the projector.
    Q_PROPERTY(QString bodyFontFamily READ bodyFontFamily WRITE setBodyFontFamily NOTIFY bodyFontChanged)
    Q_PROPERTY(int bodyFontSize READ bodyFontSize WRITE setBodyFontSize NOTIFY bodyFontChanged)

public:
    enum FocusPlacement {
        ExactOffset = 0,
        Start,
        End,
        FirstVisualLineAtX,
        LastVisualLineAtX,
    };
    Q_ENUM(FocusPlacement)

    explicit EditorController(QObject *parent = nullptr);

    BlockModel *blockModel();
    ActiveBlockTextController *activeTextController();
    qulonglong activeBlockId() const;
    int activeBlockRow() const;
    int activeCursorOffset() const;
    qulonglong selectionRevision() const;
    bool streaming() const;

    QColor codeBackgroundColor() const { return m_palette.codeBackground; }
    QColor linkColor() const { return m_palette.link; }
    QColor bodyTextColor() const { return m_palette.text; }
    QString monoFamily() const { return m_palette.monoFamily; }
    QString bodyFontFamily() const { return m_bodyFontFamily; }
    int bodyFontSize() const { return m_palette.bodyPixelSize; }
    void setCodeBackgroundColor(const QColor &color);
    void setLinkColor(const QColor &color);
    void setBodyTextColor(const QColor &color);
    void setMonoFamily(const QString &family);
    void setBodyFontFamily(const QString &family);
    void setBodyFontSize(int pixelSize);

    Q_INVOKABLE void loadMarkdown(const QString &markdown, bool activateFirstBlock = true);
    Q_INVOKABLE void beginStream();
    Q_INVOKABLE void streamAppend(const QString &text);
    Q_INVOKABLE void endStream();
    Q_INVOKABLE void activateBlock(qulonglong blockId);
    Q_INVOKABLE void activateBlockAt(int row, int cursorOffset = 0);
    Q_INVOKABLE void requestFocusForActiveBlock(int placement, int cursorOffset, qreal visualX);
    Q_INVOKABLE void moveActiveBlock(int delta, int placement, qreal visualX);
    Q_INVOKABLE void updateActiveCursorOffset(int cursorOffset);
    Q_INVOKABLE void replaceBlockMarkdown(qulonglong blockId, const QString &markdown);
    Q_INVOKABLE QString projectionText(qulonglong blockId, bool revealMarkdown = false, int rawCursor = -1) const;
    Q_INVOKABLE void replaceVisualRange(qulonglong blockId, int visualStart, int visualEnd, const QString &insertedText);
    Q_INVOKABLE void insertBlockAfter(qulonglong blockId, const QString &markdown);
    Q_INVOKABLE void splitBlock(qulonglong blockId, int rawOffset);
    Q_INVOKABLE void splitParagraph(qulonglong blockId, int rawOffset);
    Q_INVOKABLE void mergeWithPrevious(qulonglong blockId);
    Q_INVOKABLE void pasteMarkdownAtRange(qulonglong blockId, int rawStart, int rawEnd, const QString &text);
    Q_INVOKABLE void pasteFromClipboard(qulonglong blockId, int rawStart, int rawEnd);
    Q_INVOKABLE void indentListItem(qulonglong blockId);
    Q_INVOKABLE void outdentListItem(qulonglong blockId);
    Q_INVOKABLE void backspaceAtStart(qulonglong blockId);
    Q_INVOKABLE void deleteBlock(qulonglong blockId);
    Q_INVOKABLE void moveBlock(int fromRow, int toRow);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void beginSelection(int row, int visualOffset);
    Q_INVOKABLE void updateSelection(int row, int visualOffset);
    Q_INVOKABLE void beginSelectionAtRaw(int row, int rawOffset);
    Q_INVOKABLE void updateSelectionAtRaw(int row, int rawOffset);
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void selectWord(int row, int visualOffset);
    Q_INVOKABLE void selectLine(int row, int visualOffset);
    Q_INVOKABLE void selectWordAtRaw(int row, int rawOffset);
    Q_INVOKABLE void selectLineAtRaw(int row, int rawOffset);
    Q_INVOKABLE void extendSelectionFromActive(int row, int offset, bool offsetIsRaw);
    Q_INVOKABLE void setActiveNativeSelection(int rawStart, int rawEnd);
    Q_INVOKABLE QVariantMap selectionSpanForBlock(qulonglong blockId, int row, int visualLength) const;
    // Table cell mapping. rowIndex 0 = header, rowIndex >= 1 = body row (rowIndex-1),
    // matching TableBlock.qml's flattened cell list.
    Q_INVOKABLE int tableRawOffsetForCell(qulonglong blockId, int rowIndex, int col, int inCellVisualOffset) const;
    Q_INVOKABLE QVariantMap tableCellSelectionSpan(qulonglong blockId, int rowIndex, int col) const;
    Q_INVOKABLE QString copySelectionMarkdown() const;
    Q_INVOKABLE void copySelectionToClipboard() const;
    Q_INVOKABLE QVariantList search(const QString &needle) const;
    Q_INVOKABLE void reportBlockHeight(qulonglong blockId, qreal height);
    Q_INVOKABLE int rowAtContentY(qreal y) const;
    Q_INVOKABLE qreal prefixHeight(int row) const;
    Q_INVOKABLE bool openPersistence(const QString &path);
    Q_INVOKABLE bool flushChangedBlocks();
    Q_INVOKABLE QString exportMarkdown() const;
    Q_INVOKABLE void importMarkdown(const QString &markdown);
    Q_INVOKABLE QString normalizeClipboardText(const QString &plainText, const QString &markdownText = {}, const QString &htmlText = {}) const;

signals:
    void activeBlockIdChanged();
    void activeBlockRowChanged();
    void activeCursorOffsetChanged();
    void editorFocusRequested(qulonglong blockId, int placement, int cursorOffset, qreal visualX);
    void selectionRevisionChanged();
    void streamingChanged();
    void streamContentAppended();
    void paletteChanged();
    void bodyFontChanged();
    // Emitted after any change to the document content (load/edit/stream) so a
    // host (Transcript) can debounce and persist the exported markdown.
    void documentChanged();

private:
    void applyPalette();
    void reprojectAll();
    // Per-cell projection for a table block: the cell's raw start offset in the
    // block markdown plus a Paragraph projection of the cell's raw text (for
    // in-cell visual<->raw mapping). rawLen is the cell's raw UTF-16 length.
    struct TableCellProjection {
        qsizetype rawStart = 0;
        qsizetype rawLen = 0;
        be::BlockProjection projection;
    };
    // Flattened grid of cell projections for one table: grid[0] = header,
    // grid[rowIndex>=1] = body row (rowIndex-1). Cached as a single entry keyed
    // by (blockId, revision); rebuilt on miss.
    struct TableCellMap {
        be::BlockId blockId = 0;
        quint32 revision = 0;
        int columns = 0;
        QVector<QVector<TableCellProjection>> grid;
    };
    const TableCellMap *tableCellMap(qulonglong blockId) const;

    void resetModel();
    void rebuildHeightIndex();
    void performSplit(qulonglong blockId, int rawOffset, bool trimBoundary);
    void applyListIndent(qulonglong blockId, int deltaUnits);
    void scheduleFlush();
    void clearActiveSelection();
    void syncActiveBlockAfterUndo();
    void flushStreamBuffer();

    be::DocumentStore m_store;
    be::BlockHeightIndex m_heightIndex;
    BlockModel m_model;
    ActiveBlockTextController m_activeTextController;
    be::SelectionControllerCore m_selection;
    be::CommandStack m_commands;
    be::ChangedBlockStore m_persistence;
    be::InlineProjector m_projector;
    be::Palette m_palette;
    QString m_bodyFontFamily;
    QTimer m_flushTimer;
    be::BlockId m_activeBlockId = 0;
    int m_activeCursorOffset = 0;
    int m_activeNativeStart = 0;
    int m_activeNativeEnd = 0;
    mutable TableCellMap m_tableCellCache;

    bool m_streaming = false;
    QString m_streamPending;
    QTimer m_streamTimer;
    qsizetype m_streamStartRow = 0;
};

} // namespace be::app
