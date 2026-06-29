// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "core/block_record.h"
#include "core/document_store.h"

#include <QVariantList>
#include <QVariantMap>
#include <QVector>

namespace be {

// Translates the daemon orchestrator's structured AgentEvent stream into typed
// block mutations on a DocumentStore. This is the transport-agnostic seam: today
// it is fed by a mock/sample event list (and the unit tests); later the real
// daemon Unix-socket/FFI feed plugs in here unchanged, because it only speaks in
// already-decoded event maps.
//
// Event maps mirror the daemon AgentEvent variants (keyed by "type"):
//   { type: "text",          text }                       -> streamed paragraph
//   { type: "reasoningDelta", text }                      -> open/extend Reasoning
//   { type: "reasoningDone",  durationMs? }               -> settle Reasoning
//   { type: "toolStarted",   callId, name, argsSummary?,
//                            tone?, detailKind? }          -> append ToolCall (running)
//   { type: "toolFinished",  callId, status?, durationMs?,
//                            detailKind?, <detail fields> } -> patch ToolCall by callId
//   { type: "content",       kind, body }                 -> append Content
//
// Each ingest() returns the BlockChangeSets to apply to a model, in order, so a
// controller can drive incremental view updates; tests inspect the store directly.
class TranscriptIngest {
public:
    explicit TranscriptIngest(DocumentStore* store);

    QVector<BlockChangeSet> ingest(const QVariantMap& event);
    QVector<BlockChangeSet> ingestAll(const QVariantList& events);

    // Settle any open text stream / reasoning block (call at end of a turn).
    QVector<BlockChangeSet> finish();

private:
    // Open an assistant message on the first content event of a turn so every
    // streamed/typed block it produces is tagged with that message id. Reset by
    // finish() (the turn-end marker), so the next turn opens a fresh message.
    void ensureTurn();

    DocumentStore* m_store = nullptr;
    bool m_textStreaming = false;
    bool m_turnOpen = false;
    BlockId m_reasoningBlock = 0;
    QString m_reasoningBody;
};

} // namespace be
