// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/mirror_transcript_sink.h"

#include "mirror/generated/entities_gen.h"
#include "mirror/ingestor.h"

namespace daemonapp::daemon {

namespace {

// The one wire→canonical mapping point for the app-local transcript stream (the twin of the
// wire-decode map_transcript_block, but sourced from the engine's coalesced cache row). The
// entity's field set is the transcript-block CDDL union's common fields only — argsSummary /
// detailKind / detailBody have no home here (LEDGER-a7t "ENTITY-FIELD GAP").
mirror::TranscriptBlock toMirrorBlock(const CachedTranscriptBlockRow& row) {
    mirror::TranscriptBlock out;
    out.session = row.sessionId;
    out.seq = row.seq;
    out.kind = row.kind;
    out.role = row.role;
    out.text = row.text;
    out.call_id = row.callId;
    out.name = row.toolName;
    out.summary = row.summary;
    out.ok = row.ok;
    return out;
}

} // namespace

void MirrorTranscriptSink::deliverBlock(const CachedTranscriptBlockRow& row) {
    if (m_ingestor == nullptr || row.sessionId.isEmpty()) {
        return;
    }
    m_ingestor->deliverTranscriptBlock(toMirrorBlock(row));
}

void MirrorTranscriptSink::clear(const QString& sessionId) {
    if (m_ingestor == nullptr || sessionId.isEmpty()) {
        return;
    }
    m_ingestor->clearTranscriptBlocks(sessionId);
}

} // namespace daemonapp::daemon
