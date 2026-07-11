// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once
// A7T (spec 09 §13 wave M4 sub-step 6): the engine-facing seam that re-homes the turn engine's
// app-local transcript stream onto the mirror. The engine (DaemonApp/Turn) is deliberately kept
// free of mirror types — it sees ONLY this interface, in terms of the same CachedTranscriptBlockRow
// it already writes to the DaemonCacheStore. The concrete adapter (MirrorTranscriptSink, substrate-
// gated) maps the row onto the generated mirror::TranscriptBlock and feeds the ingestor (the single
// mirror writer, §5.1). Injected via the turn-engine factory; null (no-op) in mock / substrate-less
// stacks.

#include "daemon/daemon_cache_store.h"

#include <QString>

namespace daemonapp::daemon {

class ITranscriptMirrorSink {
public:
    ITranscriptMirrorSink() = default;
    virtual ~ITranscriptMirrorSink() = default;
    ITranscriptMirrorSink(const ITranscriptMirrorSink&) = delete;
    ITranscriptMirrorSink& operator=(const ITranscriptMirrorSink&) = delete;
    ITranscriptMirrorSink(ITranscriptMirrorSink&&) = delete;
    ITranscriptMirrorSink& operator=(ITranscriptMirrorSink&&) = delete;

    // Dual-write ONE already-coalesced transcript block (the same row handed to
    // DaemonCacheStore::upsertTranscriptBlock) into the mirror transcript window.
    virtual void deliverBlock(const CachedTranscriptBlockRow& row) = 0;
    // Wipe a session's mirror transcript window (the journal-rebaseline clear before replay).
    virtual void clear(const QString& sessionId) = 0;
};

} // namespace daemonapp::daemon
