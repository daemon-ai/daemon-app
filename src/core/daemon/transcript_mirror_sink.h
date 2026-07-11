// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once
// A7T (spec 09 §13 wave M4 sub-step 6) → AD: the engine-facing mirror seam. The engine
// (DaemonApp/Turn) is deliberately kept free of mirror types — it sees ONLY this interface, in
// terms of the CachedTranscriptBlockRow value shape it already coalesces. The concrete adapter
// (MirrorTranscriptSink) maps rows onto the generated mirror::TranscriptBlock and feeds the
// ingestor (the single mirror writer, §5.1 — WRITES), and answers the engine's two roster
// enrichment READS (child title / child end-reason) from the mirror snapshot — the reads that
// used to hit the legacy daemon_session_roster / daemon_fleet_units caches (deleted in AD).
// Injected via the turn-engine factory; null (no-op / empty) in mock and bare test stacks.

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

    // Write ONE already-coalesced transcript block into the mirror transcript window (the single
    // transcript write path since AD).
    virtual void deliverBlock(const CachedTranscriptBlockRow& row) = 0;
    // Wipe a session's mirror transcript window (the journal-rebaseline clear before replay).
    virtual void clear(const QString& sessionId) = 0;

    // --- roster enrichment reads (AD) ------------------------------------------------------
    // The child session's roster title ("" when the roster has no row/title yet — the caller
    // falls back to the id). Structured enrichment only; never parsed from display text.
    [[nodiscard]] virtual QString sessionTitle(const QString& sessionId) const = 0;
    // The node-reported terminal reason for a child session's fleet unit ("" when unknown /
    // still running). Rendered verbatim, never derived.
    [[nodiscard]] virtual QString unitEndReason(const QString& sessionId) const = 0;
};

} // namespace daemonapp::daemon
