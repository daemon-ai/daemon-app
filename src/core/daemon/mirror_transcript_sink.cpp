// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/mirror_transcript_sink.h"

#include "mirror/generated/entities_gen.h"
#include "mirror/ingestor.h"

namespace daemonapp::daemon {

namespace {

// The one wire→canonical mapping point for the app-local transcript stream (the twin of the
// wire-decode map_transcript_block, but sourced from the engine's coalesced cache row). G2 (M5)
// closed the entity gap: args_summary (ToolCall) + detail_kind/detail_body (ToolResult structured
// detail) now round-trip, so the mirror msg-fence projection reproduces tool fences byte-for-byte.
// detail_body is the JSON payload decoded as UTF-8 (the exact transform the legacy projection
// applied at render time), so a straight fence insert matches the legacy bytes.
mirror::TranscriptBlock toMirrorBlock(const CachedTranscriptBlockRow& row) {
    mirror::TranscriptBlock out;
    out.session = row.sessionId;
    out.seq = row.seq;
    out.kind = row.kind;
    out.role = row.role;
    out.text = row.text;
    out.call_id = row.callId;
    out.name = row.toolName;
    out.args_summary = row.argsSummary;
    out.summary = row.summary;
    out.ok = row.ok;
    out.detail_kind = row.detailKind;
    out.detail_body = QString::fromUtf8(row.detailBody);
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
