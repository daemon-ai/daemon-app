// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "core/agent_ingest.h"

#include <array>
#include <utility>

namespace be {

namespace {

QString stringField(const QVariantMap& m, const QString& key) {
    return m.value(key).toString();
}

// Copy the detail payload fields a ToolFinished event may carry through to the
// block metadata, so buildToolView's sub-renderer has what it needs. The raw
// node carriers (detailKind + detailBody + summary, D1) ride alongside the
// already-flat keys the simulator emits; buildToolView projects the raw form
// into the flat renderer keys late, in the shared view model.
void copyToolDetail(const QVariantMap& event, QVariantMap& meta) {
    static constexpr auto keys =
        std::to_array<const char*>({"detailKind", "detailBody", "summary", "stdout", "stderr",
                                    "body", "diff", "hits", "imageUrl", "count", "exitCode"});
    for (const char* key : keys) {
        const QString k = QString::fromLatin1(key);
        if (event.contains(k)) {
            meta.insert(k, event.value(k));
        }
    }
}

} // namespace

TranscriptIngest::TranscriptIngest(DocumentStore* store) : m_store(store) {}

QVector<BlockChangeSet> TranscriptIngest::ingest(const QVariantMap& event) {
    QVector<BlockChangeSet> out;
    if (!m_store) {
        return out;
    }

    const QString type = stringField(event, QStringLiteral("type"));

    // Turn-end marker: close any open text stream and settle a running reasoning
    // block. Reachable from QML via ingestEvents([{type:"flush"}]); mirrors the
    // daemon emitting an end-of-turn event.
    if (type == QStringLiteral("flush") || type == QStringLiteral("finish")) {
        return finish();
    }

    // Status-only events (token usage, context-window fill, rate-limit windows,
    // subagent/delegation progress, and todo-list updates) carry no transcript
    // content: the status bar, subagent model, and composer todo stack consume
    // them off the same stream. Ignore them here so they neither open an
    // assistant turn nor create blocks.
    if (type == QStringLiteral("usage") || type == QStringLiteral("context") ||
        type == QStringLiteral("rateLimit") || type == QStringLiteral("todoUpdate") ||
        type.startsWith(QStringLiteral("subagent"))) {
        return out;
    }

    // The node's echo of the user's own message (an inbound StartTurn/Steer Command on the
    // session log): settle any open assistant run first, then append a roled user message via
    // the same primitive the transcript-log applier uses, so the block carries a real message
    // id/boundary marker (edit/retry affordances work). The next assistant content re-opens a
    // fresh assistant message. Must not run through ensureTurn (that opens an Assistant role).
    if (type == QStringLiteral("userMessage")) {
        const QString text = stringField(event, QStringLiteral("text")).trimmed();
        if (text.isEmpty()) {
            return out;
        }
        out += finish();
        const qsizetype before = m_store->blockCount();
        m_store->appendMessageBlocks(MessageRole::User, text);
        BlockChangeSet appended;
        appended.structuralRow = before;
        appended.insertedCount = m_store->blockCount() - before;
        out.push_back(appended);
        return out;
    }

    // [wave2:app-delegation] F1/F2: a delegation completion notice (the node injecting a detached
    // child's result as a fresh turn) renders as a System message — an honest "↳" delegation line,
    // not a raw user bubble. Settle any open assistant run first (like userMessage); reuses the
    // same roled-message primitive so it persists + reloads via the existing System-role path.
    if (type == QStringLiteral("delegationNotice")) {
        const QString text = stringField(event, QStringLiteral("text")).trimmed();
        if (text.isEmpty()) {
            return out;
        }
        out += finish();
        const qsizetype before = m_store->blockCount();
        m_store->appendMessageBlocks(MessageRole::System, text);
        BlockChangeSet appended;
        appended.structuralRow = before;
        appended.insertedCount = m_store->blockCount() - before;
        out.push_back(appended);
        return out;
    }

    // Any content-producing event opens the assistant message if one is not
    // already open, so the blocks below group under a single assistant turn. The
    // turn-opening event carries the wire anchor captured for feedback.
    ensureTurn(event);

    if (type == QStringLiteral("text")) {
        const QString text = stringField(event, QStringLiteral("text"));
        if (text.isEmpty()) {
            return out;
        }
        if (!m_textStreaming) {
            m_store->beginStreamAtEnd();
            m_textStreaming = true;
        }
        out.push_back(m_store->streamAppend(text));
        return out;
    }

    // Any non-text event ends the open text stream first, so the streamed
    // paragraph settles above the structured block.
    if (m_textStreaming) {
        m_store->endStream();
        m_textStreaming = false;
    }

    if (type == QStringLiteral("reasoningDelta")) {
        const QString text = stringField(event, QStringLiteral("text"));
        m_reasoningBody += text;
        if (m_reasoningBlock == 0) {
            QVariantMap meta;
            meta.insert(QStringLiteral("status"), QStringLiteral("running"));
            meta.insert(QStringLiteral("body"), m_reasoningBody);
            BlockChangeSet cs;
            m_reasoningBlock = m_store->appendTypedBlock(BlockType::Reasoning, meta, &cs);
            out.push_back(cs);
        } else {
            QVariantMap patch;
            patch.insert(QStringLiteral("body"), m_reasoningBody);
            out.push_back(m_store->updateBlockMetadata(m_reasoningBlock, patch));
        }
        return out;
    }

    if (type == QStringLiteral("reasoningDone")) {
        if (m_reasoningBlock != 0) {
            QVariantMap patch;
            patch.insert(QStringLiteral("status"), QStringLiteral("complete"));
            if (event.contains(QStringLiteral("durationMs"))) {
                patch.insert(QStringLiteral("durationMs"),
                             event.value(QStringLiteral("durationMs")));
            }
            out.push_back(m_store->updateBlockMetadata(m_reasoningBlock, patch));
        }
        m_reasoningBlock = 0;
        m_reasoningBody.clear();
        return out;
    }

    if (type == QStringLiteral("toolStarted")) {
        QVariantMap meta;
        meta.insert(QStringLiteral("callId"), event.value(QStringLiteral("callId")));
        meta.insert(QStringLiteral("name"), stringField(event, QStringLiteral("name")));
        meta.insert(QStringLiteral("argsSummary"),
                    stringField(event, QStringLiteral("argsSummary")));
        meta.insert(QStringLiteral("tone"), stringField(event, QStringLiteral("tone")));
        meta.insert(QStringLiteral("status"), QStringLiteral("running"));
        if (event.contains(QStringLiteral("detailKind"))) {
            meta.insert(QStringLiteral("detailKind"), event.value(QStringLiteral("detailKind")));
        }
        // Interactive tool fields (approval gate / clarify form): passed through
        // verbatim when present so a live turn can stream an awaiting-approval or
        // clarify tool that the inline controls answer. Absent for ordinary tools,
        // so non-interactive turns are unchanged.
        for (const QString& key :
             {QStringLiteral("needsApproval"), QStringLiteral("allowPermanent"),
              QStringLiteral("approvalCommand"), QStringLiteral("questions"),
              QStringLiteral("requestId")}) {
            if (event.contains(key)) {
                meta.insert(key, event.value(key));
            }
        }
        BlockChangeSet cs;
        m_store->appendTypedBlock(BlockType::ToolCall, meta, &cs);
        out.push_back(cs);
        return out;
    }

    if (type == QStringLiteral("toolFinished")) {
        const QVariant callId = event.value(QStringLiteral("callId"));
        const BlockId id = m_store->blockIdForMetadata(QStringLiteral("callId"), callId);
        if (id != 0) {
            QVariantMap patch;
            patch.insert(QStringLiteral("status"),
                         event.value(QStringLiteral("status"), QStringLiteral("ok")));
            if (event.contains(QStringLiteral("durationMs"))) {
                patch.insert(QStringLiteral("durationMs"),
                             event.value(QStringLiteral("durationMs")));
            }
            copyToolDetail(event, patch);
            out.push_back(m_store->updateBlockMetadata(id, patch));
        }
        return out;
    }

    if (type == QStringLiteral("content")) {
        QVariantMap meta;
        meta.insert(QStringLiteral("kind"), stringField(event, QStringLiteral("kind")));
        meta.insert(QStringLiteral("body"), stringField(event, QStringLiteral("body")));
        BlockChangeSet cs;
        m_store->appendTypedBlock(BlockType::Content, meta, &cs);
        out.push_back(cs);
        return out;
    }

    return out;
}

QVector<BlockChangeSet> TranscriptIngest::ingestAll(const QVariantList& events) {
    QVector<BlockChangeSet> out;
    for (const QVariant& e : events) {
        out += ingest(e.toMap());
    }
    return out;
}

void TranscriptIngest::ensureTurn(const QVariantMap& event) {
    if (!m_store || m_turnOpen) {
        return;
    }
    const QString messageId = m_store->beginMessage(MessageRole::Assistant);
    m_turnOpen = true;

    // Capture the wire anchor from the turn-opening event so per-message feedback
    // can reference the exact node event (turn seq / journal cursor / trace id).
    // Absent keys are omitted: the mock/simulator maps carry none, so the anchor
    // is simply empty for those turns.
    QVariantMap anchor;
    static constexpr std::array<std::pair<const char*, const char*>, 3> kAnchorFields{{
        {"seq", "turnSeq"},
        {"journalCursor", "journalCursor"},
        {"traceId", "traceId"},
    }};
    for (const auto& [eventKey, anchorKey] : kAnchorFields) {
        const QString key = QString::fromLatin1(eventKey);
        if (event.contains(key)) {
            anchor.insert(QString::fromLatin1(anchorKey), event.value(key));
        }
    }
    if (!messageId.isEmpty()) {
        m_anchors.insert(messageId, anchor);
    }
}

QVariantMap TranscriptIngest::anchorForMessage(const QString& messageId) const {
    return m_anchors.value(messageId);
}

QVector<BlockChangeSet> TranscriptIngest::finish() {
    QVector<BlockChangeSet> out;
    if (!m_store) {
        return out;
    }
    m_turnOpen = false;
    if (m_textStreaming) {
        m_store->endStream();
        m_textStreaming = false;
    }
    if (m_reasoningBlock != 0) {
        QVariantMap patch;
        patch.insert(QStringLiteral("status"), QStringLiteral("complete"));
        out.push_back(m_store->updateBlockMetadata(m_reasoningBlock, patch));
        m_reasoningBlock = 0;
        m_reasoningBody.clear();
    }
    return out;
}

} // namespace be
