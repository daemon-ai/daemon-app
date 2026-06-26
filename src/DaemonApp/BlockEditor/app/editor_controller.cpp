#include "app/editor_controller.h"

#include "app/math_image_provider.h"
#include "app/transcript_log.h"
#include "core/agent_block.h"
#include "core/markdown_table.h"
#include "core/math_url.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>
#include <QRegularExpression>
#include <QVariantList>
#include <QVariantMap>

namespace be::app {

namespace {

QString sampleDocument() {
    return QStringLiteral(
        "# QML Markdown Block Editor\n\n"
        "This shell is backed by a C++ `DocumentStore`, `BlockModel`, and md4qt parser "
        "plumbing.\n\n"
        "- Virtualized blocks are rendered by a `ListView` with `reuseItems`.\n"
        "- Click a block to activate its raw Markdown editor.\n"
        "- Persistence, undo, selection, and projection code now have core homes.\n\n"
        "Try editing **bold text** or `inline code` in this block.\n\n"
        "| Feature | Status | Notes |\n"
        "| :--- | :---: | :--- |\n"
        "| Streaming | done | coalesced per frame |\n"
        "| Selection | **active** | spans blocks and cells |\n"
        "| Tables | new | in-cell text selection |\n\n"
        "Mermaid diagrams render natively (no HTML), with pan/zoom and node picking:\n\n"
        "```mermaid\n"
        "flowchart TD\n"
        "  A[Edit Markdown] --> B{Fenced as mermaid?}\n"
        "  B -->|yes| C([Parse flowchart])\n"
        "  B -->|no| D[Show code block]\n"
        "  C --> E[Layout + geometry]\n"
        "  E --> F[(Render snapshot)]\n"
        "  F --> G((Draw))\n"
        "```\n");
}

} // namespace

EditorController::EditorController(QObject* parent)
    : QObject(parent), m_ingest(&m_store), m_model(this) {
    m_model.setStore(&m_store);
    m_flushTimer.setSingleShot(true);
    m_flushTimer.setInterval(750);
    connect(&m_flushTimer, &QTimer::timeout, this, &EditorController::flushChangedBlocks);

    // Coalesce streamed tokens to at most one reparse per frame (~60Hz) so md4qt
    // runs at a bounded rate regardless of token arrival rate.
    m_streamTimer.setSingleShot(true);
    m_streamTimer.setInterval(16);
    connect(&m_streamTimer, &QTimer::timeout, this, &EditorController::flushStreamBuffer);

    // The active row is derived from the active block id and the current row
    // layout. Track id changes here; structural row shifts (reset/stream) emit
    // activeBlockRowChanged from their own sites.
    connect(this, &EditorController::activeBlockIdChanged, this,
            &EditorController::activeBlockRowChanged);

    // Keep the in-transcript find engine bound to this controller's store and
    // re-collected as the document mutates. loadMarkdown() refreshes directly (it
    // does not emit documentChanged, to avoid a persist feedback loop); every
    // other mutation path (typed-block ingest, message edits, rewinds) routes
    // through documentChanged.
    m_search.setDocument(&m_store);
    connect(this, &EditorController::documentChanged, this, [this] { m_search.refresh(); });

    loadMarkdown(sampleDocument());
}

BlockModel* EditorController::blockModel() {
    return &m_model;
}

ActiveBlockTextController* EditorController::activeTextController() {
    return &m_activeTextController;
}

qulonglong EditorController::activeBlockId() const {
    return m_activeBlockId;
}

int EditorController::activeBlockRow() const {
    return static_cast<int>(m_store.rowForBlock(m_activeBlockId));
}

int EditorController::activeCursorOffset() const {
    return m_activeCursorOffset;
}

qulonglong EditorController::selectionRevision() const {
    return m_selection.selection().revision;
}

bool EditorController::streaming() const {
    return m_streaming;
}

void EditorController::applyPalette() {
    m_projector.setPalette(m_palette);
    m_model.setPalette(m_palette);
    emit paletteChanged();
}

void EditorController::reprojectAll() {
    // The display markup is read live from the model role, so a palette refresh
    // is enough to re-render every block; no document mutation is involved.
    m_model.setPalette(m_palette);
}

void EditorController::setCodeBackgroundColor(const QColor& color) {
    if (m_palette.codeBackground == color) {
        return;
    }
    m_palette.codeBackground = color;
    applyPalette();
}

void EditorController::setLinkColor(const QColor& color) {
    if (m_palette.link == color) {
        return;
    }
    m_palette.link = color;
    applyPalette();
}

void EditorController::setBodyTextColor(const QColor& color) {
    if (m_palette.text == color) {
        return;
    }
    m_palette.text = color;
    applyPalette();
}

void EditorController::setMonoFamily(const QString& family) {
    if (m_palette.monoFamily == family) {
        return;
    }
    m_palette.monoFamily = family;
    applyPalette();
}

void EditorController::setBodyFontFamily(const QString& family) {
    if (m_bodyFontFamily == family) {
        return;
    }
    m_bodyFontFamily = family;
    // Family is applied by the block TextEdit's font, not the projected HTML;
    // just notify so QML rebinds.
    emit bodyFontChanged();
}

void EditorController::setBodyFontSize(int pixelSize) {
    if (pixelSize <= 0 || m_palette.bodyPixelSize == pixelSize) {
        return;
    }
    m_palette.bodyPixelSize = pixelSize;
    // Reproject so headings re-scale to the new base size.
    applyPalette();
    emit bodyFontChanged();
}

void EditorController::loadMarkdownBytes(const QByteArray& bytes) {
    loadMarkdown(QString::fromUtf8(bytes), false);
}

void EditorController::loadMarkdown(const QString& markdown, bool activateFirstBlock) {
    m_store.loadMarkdown(markdown);
    m_commands.clear();
    // A read-first host (the Transcript) passes false so no block is activated on
    // load: every block renders passively and nothing arms a focus callLater, so a
    // subsequent reset (session switch) cannot invalidate a pending one.
    m_activeBlockId = activateFirstBlock && m_store.blockCount() > 0 ? m_store.blockAt(0)->id : 0;
    m_activeCursorOffset = 0;
    resetModel();
    // Re-anchor find matches to the freshly loaded document (loadMarkdown is the
    // session-switch / reload path and does not emit documentChanged).
    m_search.refresh();
    emit activeBlockIdChanged();
    emit activeCursorOffsetChanged();
    if (activateFirstBlock && m_activeBlockId != 0) {
        emit editorFocusRequested(m_activeBlockId, ExactOffset, m_activeCursorOffset, 0.0);
    }
}

void EditorController::loadTranscript(QObject* store, const QString& sessionId) {
    // Fetch the session's stored markdown via the store's Q_INVOKABLE content(QString)
    // (kept dynamic so the block editor needs no link to the persistence layer).
    QString markdown;
    if (store != nullptr && !sessionId.isEmpty()) {
        QMetaObject::invokeMethod(store, "content", Q_RETURN_ARG(QString, markdown),
                                  Q_ARG(QString, sessionId));
    }

    // Rebuild the document from the decomposed entry sequence (the P4 render path),
    // then mirror loadMarkdown's read-first post-load (no block activation, re-anchor
    // search; loadTranscript is a session-switch/reload path, not an edit).
    be::applyTranscriptLog(m_store, be::decomposeMarkdown(markdown));
    m_commands.clear();
    m_activeBlockId = 0;
    m_activeCursorOffset = 0;
    resetModel();
    m_search.refresh();
    emit activeBlockIdChanged();
    emit activeCursorOffsetChanged();
}

void EditorController::beginStream() {
    if (m_streaming) {
        return;
    }
    m_streaming = true;
    m_streamPending.clear();
    m_streamStartRow = m_store.blockCount();
    clearActiveSelection();
    m_store.beginStreamAtEnd();
    emit streamingChanged();
}

void EditorController::streamAppend(const QString& text) {
    if (!m_streaming || text.isEmpty()) {
        return;
    }
    m_streamPending += text;
    if (!m_streamTimer.isActive()) {
        m_streamTimer.start();
    }
}

void EditorController::flushStreamBuffer() {
    if (!m_streaming || m_streamPending.isEmpty()) {
        return;
    }
    const QString chunk = m_streamPending;
    m_streamPending.clear();
    const be::BlockChangeSet changeSet = m_store.streamAppend(chunk);
    m_model.applyChangeSet(changeSet);
    // applyChangeSet does incremental row inserts/removes without resetModel(),
    // so the active block's row index can shift without activeBlockIdChanged.
    if (changeSet.insertedCount > 0 || changeSet.removedCount > 0) {
        emit activeBlockRowChanged();
    }
    emit streamContentAppended();
}

void EditorController::endStream() {
    if (!m_streaming) {
        return;
    }

    m_streamTimer.stop();
    flushStreamBuffer();
    m_store.endStream();
    m_streaming = false;

    // Record the stream as a single scoped undo step over just the appended tail.
    QVector<be::BlockRecord> appended;
    for (qsizetype row = m_streamStartRow; row < m_store.blockCount(); ++row) {
        if (const be::BlockRecord* block = m_store.blockAt(row)) {
            appended.push_back(*block);
        }
    }
    if (!appended.isEmpty()) {
        m_commands.pushApplied(std::make_unique<be::StreamCommand>(
            m_streamStartRow, QVector<be::BlockRecord>{}, std::move(appended)));
    }

    rebuildHeightIndex();
    emit streamingChanged();
    emit streamContentAppended();
    scheduleFlush();
}

qulonglong EditorController::appendTypedBlock(const QString& kind, const QVariantMap& metadata) {
    const be::BlockType type = be::agentBlockTypeForFence(kind);
    if (type == be::BlockType::Unknown) {
        return 0;
    }
    be::BlockChangeSet changeSet;
    const be::BlockId id = m_store.appendTypedBlock(type, metadata, &changeSet);
    m_model.applyChangeSet(changeSet);
    rebuildHeightIndex();
    emit streamContentAppended();
    scheduleFlush();
    emit documentChanged();
    return id;
}

void EditorController::updateTypedBlock(qulonglong blockId, const QVariantMap& patch) {
    const be::BlockChangeSet changeSet = m_store.updateBlockMetadata(blockId, patch);
    if (changeSet.changedFirst < 0) {
        return;
    }
    m_model.applyChangeSet(changeSet);
    scheduleFlush();
    emit documentChanged();
}

qulonglong EditorController::blockIdForCallId(const QString& callId) const {
    return m_store.blockIdForMetadata(QStringLiteral("callId"), callId);
}

void EditorController::ingestEvent(const QVariantMap& event) {
    const QVector<be::BlockChangeSet> sets = m_ingest.ingest(event);
    for (const be::BlockChangeSet& cs : sets) {
        m_model.applyChangeSet(cs);
    }
    rebuildHeightIndex();
    emit streamContentAppended();
    scheduleFlush();
    emit documentChanged();
}

void EditorController::ingestEvents(const QVariantList& events) {
    for (const QVariant& event : events) {
        ingestEvent(event.toMap());
    }
}

void EditorController::requestImagePreview(const QString& url, const QString& alt) {
    if (url.isEmpty()) {
        return;
    }
    emit imagePreviewRequested(url, alt);
}

void EditorController::answerClarify(qulonglong blockId, const QString& requestId,
                                     const QVariantMap& answers) {
    // Local echo: mark the clarify tool answered so it collapses to a compact
    // resolved row, persisted via the canonical fenced markdown. The patch
    // contract (structured `answers` + flat `answer` summary) is single-sourced
    // in be::clarifyAnswerPatch so the TUI answers identically.
    updateTypedBlock(blockId, be::clarifyAnswerPatch(answers));
    emit clarifyAnswered(blockId, requestId, answers);
}

void EditorController::answerToolApproval(qulonglong blockId, const QString& callId,
                                          const QString& decision, bool permanent) {
    // Local echo: record the decision so the approval bar clears (shared patch
    // contract with the TUI). A denial flips the tool to an error state; an
    // approval leaves it running for the host to drive to completion.
    updateTypedBlock(blockId, be::toolApprovalPatch(decision));
    emit toolApprovalAnswered(blockId, callId, decision, permanent);
}

void EditorController::appendUserMessage(const QString& text) {
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    m_store.appendMessageBlocks(be::MessageRole::User, trimmed);
    resetModel();
    rebuildHeightIndex();
    scheduleFlush();
    emit documentChanged();
}

void EditorController::appendSystemMessage(const QString& text, const QString& variant) {
    Q_UNUSED(variant);
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    m_store.appendMessageBlocks(be::MessageRole::System, trimmed);
    resetModel();
    rebuildHeightIndex();
    scheduleFlush();
    emit documentChanged();
}

void EditorController::editUserMessage(const QString& messageId, const QString& text) {
    const QString trimmed = text.trimmed();
    if (m_store.rowForMessage(messageId) < 0 || trimmed.isEmpty()) {
        return;
    }
    // Truncate the document at the edited message (it and the assistant reply it
    // produced are dropped), then re-add the new text as a fresh user message.
    m_store.rewindToMessage(messageId);
    const QString newId = m_store.appendMessageBlocks(be::MessageRole::User, trimmed);
    afterStructuralRewind();
    emit userMessageEdited(newId, trimmed);
}

void EditorController::requestRegenerate(const QString& messageId) {
    // Drop the assistant message (and anything after it) so the host can stream a
    // fresh reply in its place; the prior user turn is kept.
    if (m_store.regenerateFromMessage(messageId)) {
        afterStructuralRewind();
    }
    emit regenerateRequested(messageId);
}

void EditorController::restoreToMessage(const QString& messageId) {
    // "Restore checkpoint": rewind to this user message and re-run with its own
    // text. Reuses the edit path (same text), so userMessageEdited fires and the
    // host re-runs the turn.
    const QString text = messageText(messageId).trimmed();
    if (text.isEmpty()) {
        return;
    }
    editUserMessage(messageId, text);
}

QString EditorController::editFromMessage(const QString& messageId) {
    if (m_store.rowForMessage(messageId) < 0) {
        return {};
    }
    const QString text = m_store.rewindToMessage(messageId);
    afterStructuralRewind();
    return text.trimmed();
}

void EditorController::undoToMessage(const QString& messageId) {
    if (m_store.rowForMessage(messageId) < 0) {
        return;
    }
    m_store.rewindToMessage(messageId);
    afterStructuralRewind();
}

QString EditorController::lastUserMessageId() const {
    QString id;
    for (qsizetype row = 0; row < m_store.blockCount(); ++row) {
        const be::BlockRecord* block = m_store.blockAt(row);
        if (block != nullptr && block->role == be::MessageRole::User &&
            !block->messageId.isEmpty()) {
            id = block->messageId;
        }
    }
    return id;
}

QString EditorController::messageText(const QString& messageId) const {
    if (messageId.isEmpty()) {
        return {};
    }
    QStringList parts;
    for (qsizetype row = 0; row < m_store.blockCount(); ++row) {
        const be::BlockRecord* block = m_store.blockAt(row);
        if (block && block->messageId == messageId) {
            parts << block->markdown();
        }
    }
    return parts.join(QStringLiteral("\n\n"));
}

void EditorController::copyMessageToClipboard(const QString& messageId) const {
    const QString text = messageText(messageId);
    if (text.isEmpty()) {
        return;
    }
    if (QClipboard* clipboard = QGuiApplication::clipboard()) {
        clipboard->setText(text);
    }
}

void EditorController::notifyInlineEditOpen() {
    emit inlineEditOpened();
}

QVariantList EditorController::ansiSpans(const QString& text) const {
    return be::ansiToSpans(text);
}

QVariantList EditorController::parseDiff(const QString& diff) const {
    return be::parseUnifiedDiff(diff);
}

void EditorController::activateBlock(qulonglong blockId) {
    const be::BlockRecord* block = m_store.blockAt(m_store.rowForBlock(blockId));
    if (!block) {
        return;
    }

    if (m_activeBlockId == blockId) {
        emit editorFocusRequested(m_activeBlockId, ExactOffset, m_activeCursorOffset, 0.0);
        return;
    }

    m_activeBlockId = blockId;
    m_activeCursorOffset = 0;
    m_activeNativeStart = m_activeNativeEnd = 0;
    emit activeBlockIdChanged();
    emit activeCursorOffsetChanged();
    emit editorFocusRequested(m_activeBlockId, ExactOffset, m_activeCursorOffset, 0.0);
}

void EditorController::activateBlockAt(int row, int cursorOffset) {
    const be::BlockRecord* block = m_store.blockAt(row);
    if (!block) {
        return;
    }

    const int boundedOffset = cursorOffset < 0 ? block->markdown().size()
                                               : qBound(0, cursorOffset, block->markdown().size());

    const bool blockChanged = m_activeBlockId != block->id;
    const bool cursorChanged = m_activeCursorOffset != boundedOffset;
    m_activeBlockId = block->id;
    m_activeCursorOffset = boundedOffset;
    if (blockChanged) {
        m_activeNativeStart = m_activeNativeEnd = 0;
    }

    if (blockChanged) {
        emit activeBlockIdChanged();
    }
    if (cursorChanged) {
        emit activeCursorOffsetChanged();
    }
    emit editorFocusRequested(m_activeBlockId, ExactOffset, m_activeCursorOffset, 0.0);
}

void EditorController::requestFocusForActiveBlock(int placement, int cursorOffset, qreal visualX) {
    const be::BlockRecord* block = m_store.blockAt(m_store.rowForBlock(m_activeBlockId));
    if (!block) {
        return;
    }

    const int boundedOffset = cursorOffset < 0 ? block->markdown().size()
                                               : qBound(0, cursorOffset, block->markdown().size());
    if (placement == ExactOffset || placement == Start || placement == End) {
        m_activeCursorOffset =
            placement == Start ? 0 : (placement == End ? block->markdown().size() : boundedOffset);
        emit activeCursorOffsetChanged();
    }
    emit editorFocusRequested(m_activeBlockId, placement, m_activeCursorOffset, visualX);
}

void EditorController::moveActiveBlock(int delta, int placement, qreal visualX) {
    const qsizetype currentRow = m_store.rowForBlock(m_activeBlockId);
    if (currentRow < 0) {
        return;
    }

    const be::BlockRecord* target = m_store.blockAt(currentRow + delta);
    if (!target) {
        return;
    }

    const int cursorOffset =
        placement == End || placement == LastVisualLineAtX ? target->markdown().size() : 0;
    const bool blockChanged = m_activeBlockId != target->id;
    m_activeBlockId = target->id;
    m_activeCursorOffset = cursorOffset;
    if (blockChanged) {
        m_activeNativeStart = m_activeNativeEnd = 0;
        emit activeBlockIdChanged();
    }
    emit activeCursorOffsetChanged();
    emit editorFocusRequested(m_activeBlockId, placement, m_activeCursorOffset, visualX);
}

void EditorController::updateActiveCursorOffset(int cursorOffset) {
    const be::BlockRecord* block = m_store.blockAt(m_store.rowForBlock(m_activeBlockId));
    const int maxOffset = block ? block->markdown().size() : 0;
    const int boundedOffset = qBound(0, cursorOffset, maxOffset);
    if (m_activeCursorOffset == boundedOffset) {
        return;
    }
    m_activeCursorOffset = boundedOffset;
    emit activeCursorOffsetChanged();
}

void EditorController::replaceBlockMarkdown(qulonglong blockId, const QString& markdown) {
    const QString before = m_store.blockMarkdown(blockId);
    if (before == markdown) {
        return;
    }

    m_commands.push(std::make_unique<be::ReplaceBlockCommand>(blockId, before, markdown), m_store);
    m_activeBlockId = blockId;
    m_activeCursorOffset = qBound(0, m_activeCursorOffset, markdown.size());
    m_activeNativeStart = m_activeNativeEnd = 0;
    m_model.notifyBlockChanged(blockId);
    emit activeCursorOffsetChanged();
    scheduleFlush();
}

QString EditorController::projectionText(qulonglong blockId, bool revealMarkdown,
                                         int rawCursor) const {
    const be::BlockRecord* block = m_store.blockAt(m_store.rowForBlock(blockId));
    if (!block) {
        return {};
    }
    return m_projector.project(*block, revealMarkdown, rawCursor).visualText;
}

void EditorController::replaceVisualRange(qulonglong blockId, int visualStart, int visualEnd,
                                          const QString& insertedText) {
    const be::BlockRecord* block = m_store.blockAt(m_store.rowForBlock(blockId));
    if (!block) {
        return;
    }

    const be::BlockProjection projection = m_projector.project(*block);
    const qsizetype rawStart = projection.visualToRaw(visualStart);
    const qsizetype rawEnd = projection.visualToRaw(visualEnd);
    QString markdown = block->markdown();
    markdown.replace(rawStart, rawEnd - rawStart, insertedText);
    replaceBlockMarkdown(blockId, markdown);
}

void EditorController::insertBlockAfter(qulonglong blockId, const QString& markdown) {
    QVector<be::BlockRecord> before = m_store.snapshot();
    if (!m_store.insertBlockAfter(blockId, markdown)) {
        return;
    }
    m_commands.push(std::make_unique<be::StructuralCommand>(std::move(before), m_store.snapshot()),
                    m_store);
    clearActiveSelection();
    resetModel();
    scheduleFlush();
}

void EditorController::splitBlock(qulonglong blockId, int rawOffset) {
    performSplit(blockId, rawOffset, false);
}

void EditorController::splitParagraph(qulonglong blockId, int rawOffset) {
    performSplit(blockId, rawOffset, true);
}

void EditorController::performSplit(qulonglong blockId, int rawOffset, bool trimBoundary) {
    QVector<be::BlockRecord> before = m_store.snapshot();
    be::BlockId resultBlock = blockId;
    qsizetype resultCursor = 0;
    if (!m_store.splitBlock(blockId, rawOffset, &resultBlock, &resultCursor, trimBoundary)) {
        return;
    }

    m_commands.push(std::make_unique<be::StructuralCommand>(std::move(before), m_store.snapshot()),
                    m_store);
    clearActiveSelection();

    m_activeBlockId = resultBlock;
    m_activeCursorOffset = static_cast<int>(resultCursor);
    emit activeBlockIdChanged();
    emit activeCursorOffsetChanged();
    resetModel();
    emit editorFocusRequested(m_activeBlockId, ExactOffset, m_activeCursorOffset, 0.0);
    scheduleFlush();
}

void EditorController::mergeWithPrevious(qulonglong blockId) {
    const qsizetype row = m_store.rowForBlock(blockId);
    if (row <= 0) {
        return;
    }

    const be::BlockRecord* previous = m_store.blockAt(row - 1);
    if (!previous) {
        return;
    }
    const be::BlockId previousId = previous->id;

    QVector<be::BlockRecord> before = m_store.snapshot();
    qsizetype resultCursor = 0;
    if (!m_store.mergeBlocks(previousId, blockId, &resultCursor)) {
        return;
    }

    m_commands.push(std::make_unique<be::StructuralCommand>(std::move(before), m_store.snapshot()),
                    m_store);
    clearActiveSelection();

    m_activeBlockId = previousId;
    m_activeCursorOffset = static_cast<int>(resultCursor);
    emit activeBlockIdChanged();
    emit activeCursorOffsetChanged();
    resetModel();
    emit editorFocusRequested(m_activeBlockId, ExactOffset, m_activeCursorOffset, 0.0);
    scheduleFlush();
}

void EditorController::pasteMarkdownAtRange(qulonglong blockId, int rawStart, int rawEnd,
                                            const QString& text) {
    if (text.isEmpty()) {
        return;
    }
    if (m_store.rowForBlock(blockId) < 0) {
        return;
    }

    QVector<be::BlockRecord> before = m_store.snapshot();
    be::BlockId caretBlock = blockId;
    qsizetype caretOffset = 0;
    if (!m_store.pasteMarkdown(blockId, rawStart, rawEnd, text, &caretBlock, &caretOffset)) {
        return;
    }

    m_commands.push(std::make_unique<be::StructuralCommand>(std::move(before), m_store.snapshot()),
                    m_store);
    clearActiveSelection();

    m_activeBlockId = caretBlock;
    m_activeCursorOffset = static_cast<int>(caretOffset);
    emit activeBlockIdChanged();
    emit activeCursorOffsetChanged();
    resetModel();
    emit editorFocusRequested(m_activeBlockId, ExactOffset, m_activeCursorOffset, 0.0);
    scheduleFlush();
}

void EditorController::pasteFromClipboard(qulonglong blockId, int rawStart, int rawEnd) {
    const QClipboard* clipboard = QGuiApplication::clipboard();
    if (!clipboard) {
        return;
    }
    const QMimeData* mime = clipboard->mimeData();
    if (!mime) {
        return;
    }

    // Markdown-first: an explicit text/markdown flavor wins; otherwise the plain
    // text is already the markdown source; HTML is only a last resort (stripped).
    QString text;
    if (mime->hasFormat(QStringLiteral("text/markdown"))) {
        text = QString::fromUtf8(mime->data(QStringLiteral("text/markdown")));
    } else if (mime->hasText()) {
        text = mime->text();
    } else if (mime->hasHtml()) {
        text = normalizeClipboardText(QString(), QString(), mime->html());
    }

    pasteMarkdownAtRange(blockId, rawStart, rawEnd, text);
}

void EditorController::applyListIndent(qulonglong blockId, int deltaUnits) {
    QVector<be::BlockRecord> before = m_store.snapshot();
    qsizetype cursorDelta = 0;
    if (!m_store.adjustListIndent(blockId, deltaUnits, &cursorDelta)) {
        return;
    }

    m_commands.push(std::make_unique<be::StructuralCommand>(std::move(before), m_store.snapshot()),
                    m_store);
    clearActiveSelection();

    m_activeBlockId = blockId;
    m_activeCursorOffset = std::max(0, m_activeCursorOffset + static_cast<int>(cursorDelta));
    emit activeBlockIdChanged();
    emit activeCursorOffsetChanged();
    resetModel();
    emit editorFocusRequested(m_activeBlockId, ExactOffset, m_activeCursorOffset, 0.0);
    scheduleFlush();
}

void EditorController::indentListItem(qulonglong blockId) {
    applyListIndent(blockId, 1);
}

void EditorController::outdentListItem(qulonglong blockId) {
    applyListIndent(blockId, -1);
}

void EditorController::backspaceAtStart(qulonglong blockId) {
    const qsizetype row = m_store.rowForBlock(blockId);
    const be::BlockRecord* block = m_store.blockAt(row);
    if (!block) {
        return;
    }

    const bool isListItem = block->type == be::BlockType::BulletListItem ||
                            block->type == be::BlockType::OrderedListItem ||
                            block->type == be::BlockType::TaskListItem;

    if (isListItem) {
        if (block->indent > 0) {
            // Nested item: pull it out one level instead of merging.
            applyListIndent(blockId, -1);
            return;
        }

        // Top-level item: drop the marker and become a paragraph in place.
        QVector<be::BlockRecord> before = m_store.snapshot();
        qsizetype resultCursor = 0;
        if (!m_store.unlistItem(blockId, &resultCursor)) {
            return;
        }

        m_commands.push(
            std::make_unique<be::StructuralCommand>(std::move(before), m_store.snapshot()),
            m_store);
        clearActiveSelection();

        m_activeBlockId = blockId;
        m_activeCursorOffset = static_cast<int>(resultCursor);
        emit activeBlockIdChanged();
        emit activeCursorOffsetChanged();
        resetModel();
        emit editorFocusRequested(m_activeBlockId, ExactOffset, m_activeCursorOffset, 0.0);
        scheduleFlush();
        return;
    }

    mergeWithPrevious(blockId);
}

void EditorController::deleteBlock(qulonglong blockId) {
    const qsizetype row = m_store.rowForBlock(blockId);
    if (row < 0) {
        return;
    }

    QVector<be::BlockRecord> before = m_store.snapshot();
    if (!m_store.deleteBlocks(row, 1)) {
        return;
    }
    m_commands.push(std::make_unique<be::StructuralCommand>(std::move(before), m_store.snapshot()),
                    m_store);

    if (m_activeBlockId == blockId) {
        m_activeBlockId =
            m_store.blockCount() > 0 ? m_store.blockAt(qMin(row, m_store.blockCount() - 1))->id : 0;
        emit activeBlockIdChanged();
    }
    clearActiveSelection();
    resetModel();
    scheduleFlush();
}

void EditorController::moveBlock(int fromRow, int toRow) {
    QVector<be::BlockRecord> before = m_store.snapshot();
    if (!m_store.moveBlocks(fromRow, 1, toRow)) {
        return;
    }
    m_commands.push(std::make_unique<be::StructuralCommand>(std::move(before), m_store.snapshot()),
                    m_store);
    clearActiveSelection();
    resetModel();
    scheduleFlush();
}

void EditorController::undo() {
    m_commands.undo(m_store);
    syncActiveBlockAfterUndo();
    resetModel();
    scheduleFlush();
}

void EditorController::redo() {
    m_commands.redo(m_store);
    syncActiveBlockAfterUndo();
    resetModel();
    scheduleFlush();
}

void EditorController::beginSelection(int row, int visualOffset) {
    const be::BlockRecord* block = m_store.blockAt(row);
    if (!block) {
        return;
    }
    const be::BlockProjection projection = m_projector.project(*block);
    const qsizetype visual =
        qBound<qsizetype>(0, qsizetype(visualOffset), projection.visualText.size());
    const qsizetype raw = projection.visualToRaw(visual);
    m_activeNativeStart = m_activeNativeEnd = 0;
    m_selection.setAnchor({block->id, row, raw, visual});
    emit selectionRevisionChanged();
}

void EditorController::updateSelection(int row, int visualOffset) {
    const be::BlockRecord* block = m_store.blockAt(row);
    if (!block) {
        return;
    }
    const be::BlockProjection projection = m_projector.project(*block);
    const qsizetype visual =
        qBound<qsizetype>(0, qsizetype(visualOffset), projection.visualText.size());
    const qsizetype raw = projection.visualToRaw(visual);
    m_selection.setHead({block->id, row, raw, visual});
    emit selectionRevisionChanged();
}

void EditorController::beginSelectionAtRaw(int row, int rawOffset) {
    const be::BlockRecord* block = m_store.blockAt(row);
    if (!block) {
        return;
    }
    const be::BlockProjection projection = m_projector.project(*block);
    const qsizetype raw = qBound<qsizetype>(0, qsizetype(rawOffset), block->markdown().size());
    const qsizetype visual = projection.rawToVisual(raw);
    // A fresh document selection invalidates any native-editor range we tracked.
    m_activeNativeStart = m_activeNativeEnd = 0;
    m_selection.setAnchor({block->id, row, raw, visual});
    emit selectionRevisionChanged();
}

void EditorController::updateSelectionAtRaw(int row, int rawOffset) {
    const be::BlockRecord* block = m_store.blockAt(row);
    if (!block) {
        return;
    }
    const be::BlockProjection projection = m_projector.project(*block);
    const qsizetype raw = qBound<qsizetype>(0, qsizetype(rawOffset), block->markdown().size());
    const qsizetype visual = projection.rawToVisual(raw);
    m_selection.setHead({block->id, row, raw, visual});
    emit selectionRevisionChanged();
}

void EditorController::clearSelection() {
    m_activeNativeStart = m_activeNativeEnd = 0;
    m_selection.clear();
    emit selectionRevisionChanged();
}

void EditorController::selectAll() {
    if (m_store.blockCount() == 0) {
        return;
    }

    const be::BlockRecord* first = m_store.blockAt(0);
    const be::BlockRecord* last = m_store.blockAt(m_store.blockCount() - 1);
    const be::BlockProjection lastProjection = m_projector.project(*last);
    m_activeNativeStart = m_activeNativeEnd = 0;
    m_selection.setAnchor({first->id, 0, 0, 0});
    m_selection.setHead({last->id, m_store.blockCount() - 1, last->markdown().size(),
                         lastProjection.visualText.size()});
    emit selectionRevisionChanged();
}

void EditorController::selectWord(int row, int visualOffset) {
    const be::BlockRecord* block = m_store.blockAt(row);
    if (!block) {
        return;
    }
    const be::BlockProjection projection = m_projector.project(*block);
    const auto range =
        be::SelectionControllerCore::wordRangeAt(projection.visualText, visualOffset);
    m_selection.setAnchor({block->id, row, projection.visualToRaw(range.first), range.first});
    m_selection.setHead({block->id, row, projection.visualToRaw(range.second), range.second});
    emit selectionRevisionChanged();
}

void EditorController::selectLine(int row, int visualOffset) {
    const be::BlockRecord* block = m_store.blockAt(row);
    if (!block) {
        return;
    }
    const be::BlockProjection projection = m_projector.project(*block);
    const auto range =
        be::SelectionControllerCore::lineRangeAt(projection.visualText, visualOffset);
    m_selection.setAnchor({block->id, row, projection.visualToRaw(range.first), range.first});
    m_selection.setHead({block->id, row, projection.visualToRaw(range.second), range.second});
    emit selectionRevisionChanged();
}

void EditorController::selectWordAtRaw(int row, int rawOffset) {
    const be::BlockRecord* block = m_store.blockAt(row);
    if (!block) {
        return;
    }
    const QString raw = block->markdown();
    const be::BlockProjection projection = m_projector.project(*block);
    const auto range = be::SelectionControllerCore::wordRangeAt(raw, rawOffset);
    m_activeNativeStart = m_activeNativeEnd = 0;
    m_selection.setAnchor({block->id, row, range.first, projection.rawToVisual(range.first)});
    m_selection.setHead({block->id, row, range.second, projection.rawToVisual(range.second)});
    emit selectionRevisionChanged();
}

void EditorController::selectLineAtRaw(int row, int rawOffset) {
    const be::BlockRecord* block = m_store.blockAt(row);
    if (!block) {
        return;
    }
    const QString raw = block->markdown();
    const be::BlockProjection projection = m_projector.project(*block);
    const auto range = be::SelectionControllerCore::lineRangeAt(raw, rawOffset);
    m_activeNativeStart = m_activeNativeEnd = 0;
    m_selection.setAnchor({block->id, row, range.first, projection.rawToVisual(range.first)});
    m_selection.setHead({block->id, row, range.second, projection.rawToVisual(range.second)});
    emit selectionRevisionChanged();
}

void EditorController::extendSelectionFromActive(int row, int offset, bool offsetIsRaw) {
    const be::BlockRecord* headBlock = m_store.blockAt(row);
    if (!headBlock) {
        return;
    }
    const be::BlockProjection headProjection = m_projector.project(*headBlock);
    qsizetype headRaw = 0;
    qsizetype headVisual = 0;
    if (offsetIsRaw) {
        headRaw = qBound<qsizetype>(0, qsizetype(offset), headBlock->markdown().size());
        headVisual = headProjection.rawToVisual(headRaw);
    } else {
        headVisual = qBound<qsizetype>(0, qsizetype(offset), headProjection.visualText.size());
        headRaw = headProjection.visualToRaw(headVisual);
    }

    // No existing selection: seed the anchor at the current active caret so a
    // Shift+click extends from where editing left off (else anchor at the click).
    if (!m_selection.selection().isActive()) {
        const qsizetype anchorRow = m_store.rowForBlock(m_activeBlockId);
        const be::BlockRecord* anchorBlock = m_store.blockAt(anchorRow);
        if (anchorBlock) {
            const be::BlockProjection anchorProjection = m_projector.project(*anchorBlock);
            const qsizetype anchorRaw = qBound<qsizetype>(0, qsizetype(m_activeCursorOffset),
                                                          anchorBlock->markdown().size());
            m_selection.setAnchor(
                {anchorBlock->id, anchorRow, anchorRaw, anchorProjection.rawToVisual(anchorRaw)});
        } else {
            m_selection.setAnchor({headBlock->id, row, headRaw, headVisual});
        }
    }

    m_activeNativeStart = m_activeNativeEnd = 0;
    m_selection.setHead({headBlock->id, row, headRaw, headVisual});
    emit selectionRevisionChanged();
}

void EditorController::setActiveNativeSelection(int rawStart, int rawEnd) {
    m_activeNativeStart = rawStart;
    m_activeNativeEnd = rawEnd;
}

QVariantMap EditorController::selectionSpanForBlock(qulonglong blockId, int row,
                                                    int visualLength) const {
    const be::BlockRecord* block = m_store.blockAt(row);
    const qsizetype rawLength = block ? block->markdown().size() : 0;
    const be::SelectionSpan span = m_selection.spanForBlock(blockId, row, visualLength, rawLength);
    QVariantMap map;
    map.insert(QStringLiteral("kind"), static_cast<int>(span.kind));
    map.insert(QStringLiteral("start"), span.visualStart);
    map.insert(QStringLiteral("end"), span.visualEnd);
    map.insert(QStringLiteral("rawStart"), span.rawStart);
    map.insert(QStringLiteral("rawEnd"), span.rawEnd);
    return map;
}

const EditorController::TableCellMap* EditorController::tableCellMap(qulonglong blockId) const {
    const be::BlockRecord* block = m_store.blockAt(m_store.rowForBlock(blockId));
    if (!block || block->type != be::BlockType::Table) {
        return nullptr;
    }

    // Reuse the cached grid while the block content is unchanged.
    if (m_tableCellCache.blockId == blockId && m_tableCellCache.revision == block->revision &&
        m_tableCellCache.columns > 0) {
        return &m_tableCellCache;
    }

    const be::TableData table = be::parseTable(block->markdown());
    if (table.columns == 0) {
        return nullptr;
    }

    const auto projectCell = [this](const be::TableCell& cell) {
        TableCellProjection tcp;
        tcp.rawStart = cell.rawStart;
        tcp.rawLen = cell.rawEnd - cell.rawStart;
        be::BlockRecord paragraph;
        paragraph.type = be::BlockType::Paragraph;
        paragraph.markdownUtf8 = cell.raw.toUtf8();
        tcp.projection = m_projector.project(paragraph);
        return tcp;
    };

    TableCellMap map;
    map.blockId = blockId;
    map.revision = block->revision;
    map.columns = table.columns;

    QVector<TableCellProjection> headerRow;
    headerRow.reserve(table.header.size());
    for (const be::TableCell& cell : table.header) {
        headerRow.push_back(projectCell(cell));
    }
    map.grid.push_back(std::move(headerRow));

    for (const QVector<be::TableCell>& row : table.rows) {
        QVector<TableCellProjection> projected;
        projected.reserve(row.size());
        for (const be::TableCell& cell : row) {
            projected.push_back(projectCell(cell));
        }
        map.grid.push_back(std::move(projected));
    }

    m_tableCellCache = std::move(map);
    return &m_tableCellCache;
}

int EditorController::tableRawOffsetForCell(qulonglong blockId, int rowIndex, int col,
                                            int inCellVisualOffset) const {
    const TableCellMap* map = tableCellMap(blockId);
    if (!map || rowIndex < 0 || rowIndex >= map->grid.size()) {
        return 0;
    }
    const QVector<TableCellProjection>& row = map->grid[rowIndex];
    if (col < 0 || col >= row.size()) {
        return 0;
    }
    const TableCellProjection& cell = row[col];
    const qsizetype visual =
        qBound<qsizetype>(0, qsizetype(inCellVisualOffset), cell.projection.visualText.size());
    return static_cast<int>(cell.rawStart + cell.projection.visualToRaw(visual));
}

QVariantMap EditorController::tableCellSelectionSpan(qulonglong blockId, int rowIndex,
                                                     int col) const {
    QVariantMap result;
    result.insert(QStringLiteral("has"), false);
    result.insert(QStringLiteral("start"), 0);
    result.insert(QStringLiteral("end"), 0);

    const TableCellMap* map = tableCellMap(blockId);
    if (!map || rowIndex < 0 || rowIndex >= map->grid.size()) {
        return result;
    }
    const QVector<TableCellProjection>& row = map->grid[rowIndex];
    if (col < 0 || col >= row.size()) {
        return result;
    }
    const TableCellProjection& cell = row[col];

    const qsizetype blockRow = m_store.rowForBlock(blockId);
    const be::BlockRecord* block = m_store.blockAt(blockRow);
    if (!block) {
        return result;
    }
    const qsizetype rawLength = block->markdown().size();
    const be::SelectionSpan span = m_selection.spanForBlock(blockId, blockRow, 0, rawLength);
    if (span.kind == be::SelectionSpan::Kind::None) {
        return result;
    }

    // Intersect the selection's raw range (in block coords) with this cell.
    const qsizetype cellStart = cell.rawStart;
    const qsizetype cellEnd = cell.rawStart + cell.rawLen;
    const qsizetype interStart = qMax<qsizetype>(span.rawStart, cellStart);
    const qsizetype interEnd = qMin<qsizetype>(span.rawEnd, cellEnd);
    if (interEnd <= interStart) {
        // No (or zero-length) overlap: nothing to paint in this cell.
        return result;
    }

    const qsizetype visualStart = cell.projection.rawToVisual(interStart - cellStart);
    const qsizetype visualEnd = cell.projection.rawToVisual(interEnd - cellStart);
    result.insert(QStringLiteral("has"), true);
    result.insert(QStringLiteral("start"), static_cast<int>(visualStart));
    result.insert(QStringLiteral("end"), static_cast<int>(visualEnd));
    return result;
}

QString EditorController::copySelectionMarkdown() const {
    const QString documentSelection = m_selection.copyMarkdown(m_store.blocks());
    if (!documentSelection.isEmpty()) {
        return documentSelection;
    }

    // Fallback: a selection made entirely inside the active editor with the
    // keyboard is native-only (not in the document model). Copy that raw slice.
    if (m_activeNativeStart != m_activeNativeEnd) {
        const be::BlockRecord* block = m_store.blockAt(m_store.rowForBlock(m_activeBlockId));
        if (block) {
            const QString markdown = block->markdown();
            const int lo =
                qBound(0, qMin(m_activeNativeStart, m_activeNativeEnd), int(markdown.size()));
            const int hi =
                qBound(0, qMax(m_activeNativeStart, m_activeNativeEnd), int(markdown.size()));
            return markdown.mid(lo, hi - lo);
        }
    }
    return {};
}

void EditorController::copySelectionToClipboard() const {
    const QString markdown = copySelectionMarkdown();
    if (markdown.isEmpty()) {
        return;
    }
    if (QClipboard* clipboard = QGuiApplication::clipboard()) {
        clipboard->setText(markdown);
    }
}

void EditorController::reportBlockHeight(qulonglong blockId, qreal height) {
    const qsizetype row = m_store.rowForBlock(blockId);
    be::BlockRecord* block = m_store.mutableBlockAt(row);
    if (!block) {
        return;
    }
    block->heightHint = static_cast<float>(height);
    m_heightIndex.setHeight(row, height);
}

int EditorController::rowAtContentY(qreal y) const {
    return static_cast<int>(m_heightIndex.rowAtContentY(y));
}

int EditorController::rowForAnchor(const QString& fragment) const {
    return static_cast<int>(m_store.rowForHeadingAnchor(fragment));
}

qreal EditorController::prefixHeight(int row) const {
    return m_heightIndex.prefixHeight(row);
}

bool EditorController::openPersistence(const QString& path) {
    return m_persistence.open(path);
}

bool EditorController::flushChangedBlocks() {
    if (!m_persistence.isOpen()) {
        return false;
    }
    const bool ok = m_persistence.saveBlocks(m_store.blocks());
    if (ok) {
        for (const be::BlockRecord& block : m_store.blocks()) {
            m_store.markPersisted(block.id);
        }
        resetModel();
    }
    return ok;
}

QString EditorController::exportMarkdown() const {
    return m_store.toMarkdown();
}

void EditorController::importMarkdown(const QString& markdown) {
    loadMarkdown(markdown);
    scheduleFlush();
}

QString EditorController::normalizeClipboardText(const QString& plainText,
                                                 const QString& markdownText,
                                                 const QString& htmlText) const {
    if (!markdownText.isEmpty()) {
        return markdownText;
    }
    if (!htmlText.isEmpty()) {
        QString text = htmlText;
        text.replace(QRegularExpression(QStringLiteral("<br\\s*/?>"),
                                        QRegularExpression::CaseInsensitiveOption),
                     QStringLiteral("\n"));
        text.replace(QRegularExpression(QStringLiteral("</p\\s*>"),
                                        QRegularExpression::CaseInsensitiveOption),
                     QStringLiteral("\n\n"));
        text.remove(QRegularExpression(QStringLiteral("<[^>]+>")));
        return text.toHtmlEscaped()
            .replace(QStringLiteral("&lt;"), QStringLiteral("<"))
            .replace(QStringLiteral("&gt;"), QStringLiteral(">"))
            .replace(QStringLiteral("&amp;"), QStringLiteral("&"));
    }
    return plainText;
}

QString EditorController::mathImageUrl(const QString& latex, bool displayMode) const {
    return be::mathImageUrl(latex, displayMode, m_palette.bodyPixelSize, m_palette.text);
}

QSizeF EditorController::mathLogicalSize(const QString& latex, bool displayMode) const {
    return be::app::measureMathLogicalSize(latex, displayMode, m_palette.bodyPixelSize);
}

void EditorController::clearActiveSelection() {
    // Structural edits shift row indices, so any active selection (DocPos is
    // row-based) is stale; drop it.
    m_activeNativeStart = m_activeNativeEnd = 0;
    m_selection.clear();
    emit selectionRevisionChanged();
}

void EditorController::syncActiveBlockAfterUndo() {
    if (m_store.rowForBlock(m_activeBlockId) < 0) {
        m_activeBlockId = m_store.blockCount() > 0 ? m_store.blockAt(0)->id : 0;
        m_activeCursorOffset = 0;
        emit activeBlockIdChanged();
        emit activeCursorOffsetChanged();
    }
    m_selection.clear();
    emit selectionRevisionChanged();
}

void EditorController::afterStructuralRewind() {
    clearActiveSelection();
    m_activeBlockId = 0;
    resetModel();
    rebuildHeightIndex();
    emit activeBlockIdChanged();
    scheduleFlush();
    emit documentChanged();
}

void EditorController::resetModel() {
    rebuildHeightIndex();
    m_model.resetFromStore();
    // A reset can move the active block to a new row without changing its id;
    // re-sync the view-level current index.
    emit activeBlockRowChanged();
}

void EditorController::rebuildHeightIndex() {
    m_heightIndex.reset(m_store.blockCount());
    for (qsizetype row = 0; row < m_store.blockCount(); ++row) {
        if (const be::BlockRecord* block = m_store.blockAt(row)) {
            m_heightIndex.setHeight(row, block->heightHint);
        }
    }
}

void EditorController::scheduleFlush() {
    if (m_persistence.isOpen()) {
        m_flushTimer.start();
    }
    // Every mutating op funnels through here; surface a single change signal the
    // host can debounce into a persist of the exported markdown.
    emit documentChanged();
}

} // namespace be::app
