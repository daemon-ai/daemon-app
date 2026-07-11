// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "daemon/node_api_codec.h"
#include "i_turn_engine.h"

#include <QByteArray>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>

namespace daemonapp::daemon {
class NodeApiClient;
class DaemonCacheStore;
class SubscriptionManager;
class ITranscriptMirrorSink; // A7T: transcript dual-write into the mirror window
struct CachedTranscriptBlockRow;
struct DecodedLogEntry;
} // namespace daemonapp::daemon

// The real turn engine (CHA-1 / CHA-2): on start() it sends Submit{StartTurn} for the bound
// session, then drives a non-destructive Subscribe loop, decoding the node's AgentEvent stream and
// re-emitting it as the same daemon-shaped event maps the simulator produced (so EditorController /
// TranscriptIngest render it unchanged). The turn ends on AgentEvent::TurnFinished.
//
// Uses the shared NodeApiClient request pipeline with per-session correlation ids, so it coexists
// with the other repositories on the single in-flight queue.
class DaemonTurnEngine : public ITurnEngine {
    Q_OBJECT

public:
    // `cache` (optional) persists the per-session resync cursors (epoch/watermark) and serves
    // the child title/end-reason roster reads; null disables those (mock/test paths).
    // `subscriptions` (optional) is the L3 feed consumer this engine self-registers with on
    // `setSessionId`, so a `SessionAdvanced` for its bound session nudges it (the per-tab focus
    // wiring) regardless of which front end owns the tab.
    // `mirrorSink` (optional) is the SINGLE transcript write path (AD; §5.1 — the sink feeds the
    // ingestor's `w_transcript_blocks` window, never the store directly; the write-behind
    // persists it for the offline cold-boot render). Null disables transcript persistence
    // entirely (mock / bare test stacks — the mock TurnController simulator never writes one).
    explicit DaemonTurnEngine(daemonapp::daemon::NodeApiClient* client,
                              daemonapp::daemon::DaemonCacheStore* cache = nullptr,
                              daemonapp::daemon::SubscriptionManager* subscriptions = nullptr,
                              daemonapp::daemon::ITranscriptMirrorSink* mirrorSink = nullptr,
                              QObject* parent = nullptr);
    ~DaemonTurnEngine() override;

    [[nodiscard]] bool active() const override { return m_active; }
    [[nodiscard]] QString turnState() const override { return m_turnState; }
    [[nodiscard]] int elapsedMs() const override { return m_elapsedMs; }
    [[nodiscard]] QString errorText() const override { return m_errorText; }

    // Bind the session and, on a real change, prime the resync cursors (epoch/watermark) from the
    // durable cache so the subscribe resumes past what's already cached (render-from-cache) instead
    // of re-streaming from 0.
    void setSessionId(const QString& sessionId) override;
    void setProfile(const QString& profile) override { m_profile = profile; }

    void start(const QString& prompt) override;
    void cancel() override;
    void resume() override; // bare resume: continues the Subscribe loop if parked

    // HITL gate resolution (CHA-4 / CHA-5): send the matching Respond, then resume the loop.
    // [wave2:app-approvals-safety] D3: trailing `reason` (wire v29) rides Approved.reason on a
    // deny.
    void respondApproval(const QString& requestId, bool allow, bool allowPermanent = false,
                         const QString& reason = QString()) override;
    void respondInput(const QString& requestId, const QString& text) override;
    void respondChoice(const QString& requestId, int index) override;
    // CHA-6: interrupt / steer a running turn via Submit{Interrupt/Steer}.
    void interrupt(const QString& reason) override;
    void steer(const QString& text) override;
    // L3 lazy-focus catch-up: ensure the push subscription is open so an idle focused session
    // renders out-of-band activity the node just announced via SessionAdvanced.
    void nudge() override;
    // Fleet supervision change (FleetChanged): re-fetch the structured subagent events for this
    // session while a turn is active, so the live subagent strip reflects spawn/finish transitions.
    void fleetChanged() override;

private:
    void onResponse(const QString& correlationId, const QByteArray& responseCbor);
    void onFailure(const QString& correlationId, const QString& message);
    // Open the push Subscribe stream (L2): the server pushes LogPage Items from m_cursor; replaces
    // the old 120ms one-shot poll loop.
    void openSubscribeStream();
    // Apply one streamed LogPage: epoch/reset detection, then dedup-and-apply entries past
    // m_cursor.
    void applyLogPage(const QByteArray& responseCbor);
    // Mux stream signals (from the shared NodeApiClient), filtered to m_subscribeStreamId.
    void onStreamItem(quint64 id, const QByteArray& responseCbor);
    void onStreamEnded(quint64 id, bool error, const QString& message);
    void onStreamReset(quint64 id, quint64 epoch, quint64 headSeq);
    // L2 re-baseline: a generation change (epoch bump / head_seq<cursor / Reset) means the live log
    // we were following is gone (a restart/reactivation); replay the durable journal to recover the
    // conversation, re-surface any unanswered parked Request, then finish (the interrupted turn).
    void rebaselineFromJournal();
    void finishTurn(const QString& errorText = QString());
    // [waveB:app-v30] C6: stage-specific copy for a foreign-agent failure on a terminal turn
    // (TurnSummary.failure). `agent` (when non-empty) names the failing agent.
    [[nodiscard]] static QString foreignFailureText(const QString& stage, const QString& agent);
    // [waveB:app-v30] stretch: the node-reported end_reason for a child session (cached fleet
    // tree); drives the subagent strip's error status. Empty when unknown.
    [[nodiscard]] QString childEndReason(const QString& childId) const;
    // Park the turn on a decoded HostRequest: surface the gate to the transcript and wait for a
    // respond* (the open stream keeps delivering once the daemon resumes).
    void parkOnRequest(const daemonapp::daemon::DecodedLogEntry& entry);
    // Send an encoded Respond/ApprovalDecide for the parked gate; the open stream delivers the
    // continuation.
    void sendRespond(const QByteArray& cbor);
    // Persist the applied (epoch, watermark) for this session so a refocus/cold-start resumes the
    // subscribe + journal reads past it instead of from 0. No-op without a cache.
    void persistWatermark();
    // Reasoning persistence (L3): deltas accumulate into one durable Reasoning block per run
    // (keyed by the first delta's seq, mirroring the live ingest's coalescing). checkpoint()
    // upserts the accumulated text (page boundary durability); settle() upserts and closes the
    // run (the next delta starts a new block).
    void checkpointReasoningBlock();
    void settleReasoningBlock();
    // A7T: persist one coalesced transcript block to the durable cache AND (dual-write) the mirror
    // window (if a sink is bound). The single point every transcript write funnels through so the
    // two stores never diverge; a byte-parity harness (tst_mirror_transcript_parity) guards them.
    void persistTranscriptBlock(const daemonapp::daemon::CachedTranscriptBlockRow& block);

    // Structured subagent strip (live delegation rows): fetch the session's UnitEvents (a one-shot
    // Call — the wire has no cursored subagent stream) and map the ManageEventView::Subagent arms
    // (SubagentPhase Spawned/Finished + the child session id + role) into {type:"subagent"} events
    // the SubagentModel upserts. Seq-deduped so overlapping refetches don't re-emit. Titles are
    // enriched from the roster cache (structured node state), never parsed from display text.
    void fetchSubagentEvents();
    void applyUnitEvents(const QByteArray& responseCbor);
    [[nodiscard]] QString subagentTitle(const QString& childId) const;

    void setActive(bool active);
    void setTurnState(const QString& state);
    void setElapsedMs(int ms);
    void setErrorText(const QString& text);

    [[nodiscard]] QString submitCorrelation() const;
    [[nodiscard]] QString respondCorrelation() const;
    [[nodiscard]] QString journalCorrelation() const;
    [[nodiscard]] QString unitEventsCorrelation() const;

    daemonapp::daemon::NodeApiClient* m_client = nullptr;
    daemonapp::daemon::DaemonCacheStore* m_cache = nullptr;
    daemonapp::daemon::SubscriptionManager* m_subscriptions = nullptr;
    daemonapp::daemon::ITranscriptMirrorSink* m_mirrorSink = nullptr;
    QString m_sessionId;
    QString m_profile;    // optional profile binding for Submit (PRO-5)
    quint64 m_cursor = 0; // applied-seq watermark: reopen cursor + dedup floor (apply iff seq > it)
    quint64 m_epoch = 0;  // tracked session-activation generation (L2)
    bool m_epochKnown = false;
    quint64 m_subscribeStreamId = 0; // the open push subscription's stream id (0 = none)
    quint32 m_requestId = 1;
    bool m_active = false;
    bool m_finished = false;
    // HITL: the turn is parked on a HostRequest waiting for a respond* call.
    bool m_parked = false;
    quint32 m_pendingRequestId = 0;
    QString m_pendingKind; // "Approval" | "Input" | "Choice"
    QStringList m_pendingOptions;
    // Call ids of in-flight `todo` tool calls (status-stack feed): their ToolStarted was
    // suppressed, so the matching ToolFinished is suppressed too (and carries the todoUpdate).
    QSet<QString> m_todoCallIds;
    // The open reasoning run: first delta's seq (0 = none) + accumulated disclosure text.
    quint64 m_reasoningSeq = 0;
    QString m_reasoningText;
    // Live subagent strip: the highest ManageEventView::Subagent seq already emitted, so a refetch
    // (turn start / FleetChanged) only emits newer spawn/finish transitions. Reset per
    // turn/session.
    quint64 m_subagentSeq = 0;
    // The re-baseline journal read accumulates across pages (wire v24: each SessionHistory page
    // is bounded at kWirePageMax records); the replay runs exactly once over the full set. Reset
    // whenever a new rebaseline starts (or the bound session changes).
    QList<daemonapp::daemon::DecodedJournalRecord> m_journalAcc;
    int m_journalPages = 0;
    QString m_turnState = QStringLiteral("idle");
    int m_elapsedMs = 0;
    QString m_errorText;

    QTimer m_elapsedTimer;  // 1s tick for elapsedMs
    QTimer m_deadlineTimer; // hard cap so a stuck turn cannot run forever
    QTimer m_resumeTimer;   // backoff stream-reopen after a mid-turn transport drop (turn resume)
    qint64 m_startedAt = 0;
    int m_resumeBackoffMs = kResumeMinMs; // grows per consecutive stream drop
    int m_resumeAttempts = 0;             // consecutive drops since the last good page

    static constexpr int kDeadlineMs = 180000; // 3 min safety cap
    // Per-page read size for Subscribe/SessionHistory: the wire page bound (a larger request is
    // clamped by the node anyway; the client codec cannot decode past it).
    static constexpr quint32 kSubscribeMax = daemonapp::daemon::NodeApiCodec::kWirePageMax;
    // Runaway guard for the journal re-baseline page loop (64k records at 64/page).
    static constexpr int kMaxJournalPages = 1024;
    // Turn resume: a mid-turn stream drop (the socket dropped, daemon alive) is transient - reopen
    // the stream from m_cursor with backoff instead of killing the turn. A generation change is a
    // different case (re-baseline, not reopen).
    static constexpr int kResumeMinMs = 250;
    static constexpr int kResumeMaxMs = 4000;
    static constexpr int kResumeMaxAttempts = 12; // give up -> finish with error after this many
};
