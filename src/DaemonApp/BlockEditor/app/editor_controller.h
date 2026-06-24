#pragma once

#include "app/active_block_text_controller.h"
#include "app/block_model.h"
#include "core/agent_ingest.h"
#include "core/block_height_index.h"
#include "core/command_stack.h"
#include "core/inline_projector.h"
#include "core/persistence.h"
#include "core/selection.h"
#include "core/transcript_search.h"

#include <QColor>
#include <QObject>
#include <QSizeF>
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
    // In-transcript find engine, kept bound to this controller's DocumentStore and
    // re-collected as the document changes. QML binds query/matchCount/currentMatch
    // and connects navigateTo to scroll the matched block into view.
    Q_PROPERTY(be::TranscriptSearchController *search READ search CONSTANT)
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
    be::TranscriptSearchController *search() { return &m_search; }
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
    // Decode UTF-8 bytes (e.g. a file read through the Fs seam) and load them as
    // markdown, read-first (no block activated). Mirrors CodeEditorController's
    // loadBytes for the rich block editor path.
    Q_INVOKABLE void loadMarkdownBytes(const QByteArray &bytes);
    Q_INVOKABLE void beginStream();
    Q_INVOKABLE void streamAppend(const QString &text);
    Q_INVOKABLE void endStream();

    // Typed-block injection for agent transcripts (the runtime, structured path).
    // appendTypedBlock(kind, metadata) appends a Reasoning/ToolCall/Content block
    // (kind = "reasoning"/"tool"/"content") and returns its id; updateTypedBlock
    // merges a metadata patch into an existing block (e.g. flipping a tool from
    // running to ok by callId); blockIdForCallId resolves a tool block by its
    // callId. ingestEvent/ingestEvents drive these from daemon-shaped event maps.
    Q_INVOKABLE qulonglong appendTypedBlock(const QString &kind, const QVariantMap &metadata);
    Q_INVOKABLE void updateTypedBlock(qulonglong blockId, const QVariantMap &patch);
    Q_INVOKABLE qulonglong blockIdForCallId(const QString &callId) const;
    Q_INVOKABLE void ingestEvent(const QVariantMap &event);
    Q_INVOKABLE void ingestEvents(const QVariantList &events);
    // Interactive-block callback channel (the seam Hermes' $approvalRequest /
    // $clarifyRequest stores fill). Each invokable patches the block's metadata
    // in place (local echo + markdown round-trip) and then emits a host-bound
    // signal the agent runtime subscribes to. requestImagePreview is fire-and-
    // forget: it just asks the host to open a lightbox for the given image.
    Q_INVOKABLE void requestImagePreview(const QString &url, const QString &alt = {});
    // `answers` maps each question id to the chosen value (a string for single-
    // select/freeform, or a string list for a multi-select question). A legacy
    // single-question clarify uses the single id "q".
    Q_INVOKABLE void answerClarify(qulonglong blockId, const QString &requestId, const QVariantMap &answers);
    Q_INVOKABLE void answerToolApproval(qulonglong blockId, const QString &callId, const QString &decision, bool permanent = false);

    // Message/role layer host API. appendUserMessage parses `text` and appends it
    // as a fresh user message (the runtime alternative to the markdown-reload
    // path); appendSystemMessage adds a centered system notice. editUserMessage
    // truncates the document at the given message and replaces it with `text`
    // (a fresh user message), then emits userMessageEdited so the host can re-run
    // the assistant turn; requestRegenerate drops the assistant reply that
    // follows `messageId` and emits regenerateRequested for the host to re-run.
    Q_INVOKABLE void appendUserMessage(const QString &text);
    Q_INVOKABLE void appendSystemMessage(const QString &text, const QString &variant = {});
    Q_INVOKABLE void editUserMessage(const QString &messageId, const QString &text);
    Q_INVOKABLE void requestRegenerate(const QString &messageId);
    // Rewind affordances built on the shared DocumentStore primitives.
    // restoreToMessage re-runs the turn from `messageId` with its existing text
    // (the "restore checkpoint" action: truncate inclusive, re-add the same text,
    // emit userMessageEdited so the host re-runs). editFromMessage truncates from
    // `messageId` inclusive and returns its text so a slash-command (/edit) can
    // seed the composer; undoToMessage truncates inclusive and discards the text
    // (/undo). lastUserMessageId is the id of the most recent user message, "" if
    // none - the anchor /retry, /edit and /undo act on.
    Q_INVOKABLE void restoreToMessage(const QString &messageId);
    Q_INVOKABLE QString editFromMessage(const QString &messageId);
    Q_INVOKABLE void undoToMessage(const QString &messageId);
    Q_INVOKABLE QString lastUserMessageId() const;
    // The combined markdown of every block in `messageId` (for a footer copy /
    // an edit composer's initial text), and a clipboard helper for it.
    Q_INVOKABLE QString messageText(const QString &messageId) const;
    Q_INVOKABLE void copyMessageToClipboard(const QString &messageId) const;
    // Notify the host that an inline message editor opened. The transcript view
    // escapes stick-to-bottom in response so the growing edit composer is not
    // yanked to the bottom (mirrors Hermes' beginEditHold -> stopScroll).
    Q_INVOKABLE void notifyInlineEditOpen();
    // Sub-renderer parsers, surfaced for the tool/content blocks: ANSI SGR text
    // -> styled spans, and a unified diff -> typed lines (see core/agent_block).
    Q_INVOKABLE QVariantList ansiSpans(const QString &text) const;
    Q_INVOKABLE QVariantList parseDiff(const QString &diff) const;
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
    Q_INVOKABLE void reportBlockHeight(qulonglong blockId, qreal height);
    Q_INVOKABLE int rowAtContentY(qreal y) const;
    // Resolve an in-document anchor fragment (e.g. "#table-of-contents") to the
    // row of the matching heading, or -1 if none. Used by the QML link handler to
    // scroll to a heading instead of opening the fragment externally.
    Q_INVOKABLE int rowForAnchor(const QString &fragment) const;
    Q_INVOKABLE qreal prefixHeight(int row) const;
    Q_INVOKABLE bool openPersistence(const QString &path);
    Q_INVOKABLE bool flushChangedBlocks();
    Q_INVOKABLE QString exportMarkdown() const;
    // UTF-8 bytes of exportMarkdown(), for writing a markdown file through the Fs seam.
    Q_INVOKABLE QByteArray exportMarkdownBytes() const { return exportMarkdown().toUtf8(); }
    Q_INVOKABLE void importMarkdown(const QString &markdown);
    Q_INVOKABLE QString normalizeClipboardText(const QString &plainText, const QString &markdownText = {}, const QString &htmlText = {}) const;
    // Build an image://math URL for the given LaTeX with the live theme palette,
    // so a block (MathBlock.qml) renders the same way inline math does and a
    // theme/font-size change re-renders it.
    Q_INVOKABLE QString mathImageUrl(const QString &latex, bool displayMode) const;
    // Logical (device-independent) size MicroTeX lays the formula out at, so a
    // block (MathBlock.qml) sizes its Image exactly like inline math does
    // (same measurer), rather than from the supersampled texture's implicit
    // size. Returns an invalid QSizeF when parsing fails.
    Q_INVOKABLE QSizeF mathLogicalSize(const QString &latex, bool displayMode) const;

signals:
    void activeBlockIdChanged();
    void activeBlockRowChanged();
    void activeCursorOffsetChanged();
    void editorFocusRequested(qulonglong blockId, int placement, int cursorOffset, qreal visualX);
    void selectionRevisionChanged();
    void streamingChanged();
    void streamContentAppended();
    // Interactive-block host signals: the user answered a clarify prompt or
    // decided a tool approval in the transcript, or asked to preview an image.
    // A host (Session / agent gateway) connects these to the runtime.
    void imagePreviewRequested(const QString &url, const QString &alt);
    void clarifyAnswered(qulonglong blockId, const QString &requestId, const QVariantMap &answers);
    void toolApprovalAnswered(qulonglong blockId, const QString &callId, const QString &decision, bool permanent);
    // The user edited a prior message (the document was truncated to it and the
    // new text re-added), or asked to regenerate the assistant reply for a
    // message. The host (Session) re-runs the assistant turn in response.
    void userMessageEdited(const QString &messageId, const QString &text);
    void regenerateRequested(const QString &messageId);
    // An inline message editor opened; the transcript escapes stick-to-bottom.
    void inlineEditOpened();
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
    // Shared tail of every rewind/truncate op: drop the active block + selection,
    // rebuild the model/height index, and emit the change signals.
    void afterStructuralRewind();
    void performSplit(qulonglong blockId, int rawOffset, bool trimBoundary);
    void applyListIndent(qulonglong blockId, int deltaUnits);
    void scheduleFlush();
    void clearActiveSelection();
    void syncActiveBlockAfterUndo();
    void flushStreamBuffer();

    be::DocumentStore m_store;
    be::TranscriptSearchController m_search;
    be::TranscriptIngest m_ingest;
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
