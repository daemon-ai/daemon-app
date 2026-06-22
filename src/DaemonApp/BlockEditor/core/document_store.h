#pragma once

#include "core/block_record.h"
#include "core/coordinate_map.h"
#include "core/markdown_parser.h"
#include "core/piece_table.h"

#include <QHash>
#include <QVector>

namespace be {

// Incremental model delta produced by a streaming reparse. Describes a single
// contiguous structural change plus an optional dataChanged range so the model
// can emit precise begin/insert/remove/dataChanged signals instead of a reset.
struct BlockChangeSet {
    qsizetype changedFirst = -1; // inclusive dataChanged range; -1 == none
    qsizetype changedLast = -1;
    qsizetype structuralRow = 0; // row where rows are inserted/removed
    qsizetype removedCount = 0;
    qsizetype insertedCount = 0;
};

class DocumentStore
{
public:
    // Canonical nesting step (in spaces) for one list-indent level.
    static constexpr qsizetype kIndentUnit = 2;

    void loadMarkdown(const QString &markdown);
    void loadMarkdownUtf8(const QByteArray &markdown);

    qsizetype blockCount() const;
    const QVector<BlockRecord> &blocks() const;
    const BlockRecord *blockAt(qsizetype row) const;
    BlockRecord *mutableBlockAt(qsizetype row);
    qsizetype rowForBlock(BlockId id) const;

    // Resolve an in-document anchor fragment (e.g. "#table-of-contents", with or
    // without the leading '#') to the row of the first heading whose GitHub-style
    // slug matches. Returns -1 when no heading matches.
    qsizetype rowForHeadingAnchor(const QString &fragment) const;

    QByteArray toUtf8() const;
    QString toMarkdown() const;
    QString blockMarkdown(BlockId id) const;

    bool replaceBlockMarkdown(BlockId id, const QString &markdown);
    bool insertBlockAfter(BlockId id, const QString &markdown);
    bool splitBlock(BlockId id, qsizetype utf16Offset, BlockId *resultBlock = nullptr, qsizetype *resultCursor = nullptr, bool trimBoundary = false);
    bool mergeBlocks(BlockId previousId, BlockId currentId, qsizetype *resultCursor = nullptr);

    // Structural paste: replace [rawStart, rawEnd) of block `id` with `pasted`,
    // parsing `pasted` into its own block sequence and splicing it at the cursor
    // (the surrounding before/after halves stay their own blocks). A single-line
    // paste with no block boundary merges inline into the current block instead.
    // Reports the resulting caret block id and in-block offset.
    bool pasteMarkdown(BlockId id, qsizetype rawStart, qsizetype rawEnd, const QString &pasted,
                       BlockId *caretBlock = nullptr, qsizetype *caretOffset = nullptr);

    // List nesting. adjustListIndent shifts a list item by deltaUnits levels
    // (clamped to CommonMark depth rules) and reports the resulting caret shift
    // (signed leading-space delta) via cursorDelta. unlistItem strips a top-level
    // item's marker, turning it into a paragraph in place.
    bool adjustListIndent(BlockId id, int deltaUnits, qsizetype *cursorDelta = nullptr);
    bool unlistItem(BlockId id, qsizetype *resultCursor = nullptr);

    bool deleteBlocks(qsizetype firstRow, qsizetype count);
    bool moveBlocks(qsizetype firstRow, qsizetype count, qsizetype destinationRow);
    void markPersisted(BlockId id);

    // Streaming ingestion (phase 1: append-at-end only). beginStreamAtEnd opens a
    // volatile tail at the end of the document; streamAppend feeds raw markdown into
    // a bounded reparse window and returns an incremental BlockChangeSet; endStream
    // settles the tail and rebuilds derived state (piece table / coordinate map).
    void beginStreamAtEnd();
    BlockChangeSet streamAppend(const QString &text);
    void endStream();
    bool isStreaming() const;

    // Typed-block injection (agent transcript runtime). The daemon orchestrator
    // emits structured events, not markdown; these are the seam those events drive.
    // appendTypedBlock appends a structured block (metadata is the live render
    // source; its canonical fenced markdown is kept in sync for persistence) and
    // returns the new id, reporting the inserted row via `changeSet`. An open
    // stream window is committed first so the typed block lands after the streamed
    // text. updateBlockMetadata merges `patch` into an existing block's metadata,
    // re-serializes its markdown, and returns the dataChanged row. blockIdForMetadata
    // finds the first block whose metadata[key] == value (resolves a tool block by
    // callId for an in-place ToolFinished update); returns 0 when none match.
    BlockId appendTypedBlock(BlockType type, const QVariantMap &metadata, BlockChangeSet *changeSet = nullptr);
    BlockChangeSet updateBlockMetadata(BlockId id, const QVariantMap &patch);
    BlockId blockIdForMetadata(const QString &key, const QVariant &value) const;

    // Message/role layer (Strategy C). beginMessage opens a new message: it
    // allocates a fresh stable message id, latches it as the "current message",
    // and returns the id. Subsequent typed/stream appends tag their blocks with
    // the current role/messageId, so a turn's blocks group together and a
    // boundary marker is re-emitted on serialize. appendUserBlocks parses
    // `markdown` and appends the resulting blocks under a fresh user message
    // (the runtime path for a typed-in prompt); it returns the new message id.
    QString beginMessage(MessageRole role);
    QString appendMessageBlocks(MessageRole role, const QString &markdown);
    MessageRole currentMessageRole() const;
    QString currentMessageId() const;
    // First row whose block belongs to `messageId`, or -1 if none.
    qsizetype rowForMessage(const QString &messageId) const;

    // Rewind/edit primitives (shared by both front ends). rewindToMessage hard-
    // truncates from the message's first block to the end (the message itself and
    // everything after it are dropped) and returns the message's concatenated text
    // (so the caller can resubmit it or prefill an edit composer); it no-ops and
    // returns "" when the id is unknown. regenerateFromMessage performs the same
    // truncation but is named for the regenerate path - the caller passes the
    // assistant reply's id so the prior user turn is kept - and returns whether it
    // truncated anything. Both leave re-running the turn to the caller.
    QString rewindToMessage(const QString &messageId);
    bool regenerateFromMessage(const QString &messageId);

    // Splice a contiguous run of blocks (used by scoped stream undo/redo).
    void spliceBlocks(qsizetype firstRow, qsizetype removeCount, const QVector<BlockRecord> &insert);

    // Whole-block-list snapshot/restore, used for atomic structural undo.
    QVector<BlockRecord> snapshot() const;
    void restore(const QVector<BlockRecord> &blocks);

private:
    void loadFromParse(const QString &markdown);
    // Parse arbitrary markdown into a sequence of unowned (id == 0) block records.
    QVector<BlockRecord> recordsFromParse(const QString &markdown) const;
    // Build one unowned (id == 0) block record, typed from its first line.
    BlockRecord makeClassifiedRecord(const QString &content) const;
    void reserialize();
    void ensureSerialized() const;
    BlockChangeSet reparseWindow();
    void rebuildRowIndex();
    // Indent (in spaces) of the list item immediately above `row`, or -1 when the
    // previous row is not a list item (i.e. `row` starts a list run).
    qsizetype previousListIndentSpaces(qsizetype row) const;
    BlockId allocateId();
    void setBlockContent(BlockRecord &block, const QString &content) const;
    BlockType classifyContent(const QString &content, quint16 *headingLevel, quint16 *indent) const;

    PieceTable m_pieceTable;
    CoordinateMap m_coordinateMap;
    MarkdownParser m_parser;
    QVector<BlockRecord> m_blocks;
    QHash<BlockId, qsizetype> m_rowsById;
    BlockId m_nextBlockId = 1;

    // Current-message latch (the role layer's runtime tagging seam). Set by
    // beginMessage; applied to blocks created on the typed/stream path.
    MessageRole m_currentRole = MessageRole::None;
    QString m_currentMessageId;
    quint64 m_nextMessageSeq = 1;

    // Streaming session state (valid only between beginStreamAtEnd/endStream).
    bool m_streaming = false;
    qsizetype m_streamFirstRow = 0; // first volatile row in m_blocks
    QString m_windowBuffer;         // raw markdown of the open tail
    mutable bool m_serializeDirty = false; // piece table / coordinate map stale
};

} // namespace be
