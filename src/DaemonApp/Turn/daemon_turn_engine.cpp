// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon_turn_engine.h"

#include "daemon/client_cache_schema.h"
#include "daemon/daemon_cache_store.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon/subscription_manager.h"

#include <algorithm>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantList>
#include <QVariantMap>

namespace dcache = daemonapp::daemon::cache;

using daemonapp::daemon::AgentEventKind;
using daemonapp::daemon::ApiResponseKind;
using daemonapp::daemon::DecodedAgentEvent;
using daemonapp::daemon::DecodedLogEntry;
using daemonapp::daemon::NodeApiClient;
using daemonapp::daemon::NodeApiCodec;

namespace {

// D1/R2 defensive cap on the persisted tool-result content: the full ToolResult `summary` (diff /
// stdout / listing text) rides into block metadata, the transcript cache, and the cold-start fence
// projection, so a runaway multi-MiB output would bloat all three. One soft cap, applied
// identically on the live and journal-replay paths before the summary enters metadata/cache.
// 1 Mi QChars (>= 1 MiB UTF-8) keeps any plausible diff/listing intact. The suffix is an
// untranslated data marker, matching the node's own embedded markers ("[diff truncated]", ...).
constexpr qsizetype kToolSummaryCapChars = qsizetype(1024) * 1024;

QString cappedSummary(const QString& summary) {
    if (summary.size() <= kToolSummaryCapChars) {
        return summary;
    }
    return summary.left(kToolSummaryCapChars) + QStringLiteral("\n[output truncated]");
}

// Map a decoded AgentEvent to the daemon-shaped event map the transcript ingest already understands
// (the same shapes TurnController emits). Returns an empty map for events that carry no transcript
// content (the ingest skips those anyway).
QVariantMap toIngestEvent(const DecodedAgentEvent& event) {
    switch (event.kind) {
    case AgentEventKind::TextDelta:
        return QVariantMap{{QStringLiteral("type"), QStringLiteral("text")},
                           {QStringLiteral("text"), event.text}};
    case AgentEventKind::ReasoningDelta:
        return QVariantMap{{QStringLiteral("type"), QStringLiteral("reasoningDelta")},
                           {QStringLiteral("text"), event.text}};
    case AgentEventKind::ToolStarted:
        // CHA-3: render the real tool row (the call id lets ToolFinished patch it).
        return QVariantMap{{QStringLiteral("type"), QStringLiteral("toolStarted")},
                           {QStringLiteral("callId"), event.callId},
                           {QStringLiteral("name"), event.toolName},
                           {QStringLiteral("argsSummary"), event.toolArgs}};
    case AgentEventKind::ToolFinished: {
        // D1: carry the raw rich-result payload (full content + typed detail) through to the
        // ingest; buildToolView projects it into the flat renderer keys. detailBody is JSON
        // UTF-8 for native tools, carried as text through metadata/fences.
        QVariantMap map{{QStringLiteral("type"), QStringLiteral("toolFinished")},
                        {QStringLiteral("callId"), event.callId},
                        {QStringLiteral("status"),
                         event.toolOk ? QStringLiteral("ok") : QStringLiteral("error")}};
        if (!event.toolSummary.isEmpty()) {
            map.insert(QStringLiteral("summary"), cappedSummary(event.toolSummary));
        }
        if (!event.detailKind.isEmpty()) {
            map.insert(QStringLiteral("detailKind"), event.detailKind);
            map.insert(QStringLiteral("detailBody"), QString::fromUtf8(event.detailBody));
        }
        return map;
    }
    case AgentEventKind::Usage:
        // CHA-3 HUD: consumed off-stream by StatusBarModel (the ingest skips it).
        return QVariantMap{
            {QStringLiteral("type"), QStringLiteral("usage")},
            {QStringLiteral("tokensIn"), event.inputTokens},
            {QStringLiteral("tokensOut"), event.outputTokens},
            {QStringLiteral("costUsd"), static_cast<double>(event.costMicros) / 1'000'000.0}};
    case AgentEventKind::Context:
        return QVariantMap{{QStringLiteral("type"), QStringLiteral("context")},
                           {QStringLiteral("used"), event.contextUsed},
                           {QStringLiteral("max"),
                            event.hasContextMax ? QVariant(event.contextMax) : QVariant(0)}};
    case AgentEventKind::TurnFinished:
        return QVariantMap{{QStringLiteral("type"), QStringLiteral("flush")}};
    default:
        return {};
    }
}

// Project the `todo` tool's structured detail (JSON [{id, content, status}], the node's
// ToolDetail{kind:"todo"} body) into the status stack's {text, done} rows. `done` folds the
// terminal states (completed/cancelled); a malformed body yields an empty list (clears the stack).
QVariantList todoItemsFromDetail(const QByteArray& body) {
    QVariantList items;
    const QJsonDocument doc = QJsonDocument::fromJson(body);
    const QJsonArray list = doc.array();
    items.reserve(list.size());
    for (const auto& v : list) {
        const QJsonObject o = v.toObject();
        const QString status = o.value(QStringLiteral("status")).toString();
        items.push_back(
            QVariantMap{{QStringLiteral("text"), o.value(QStringLiteral("content")).toString()},
                        {QStringLiteral("done"), status == QStringLiteral("completed") ||
                                                     status == QStringLiteral("cancelled")}});
    }
    return items;
}

} // namespace

DaemonTurnEngine::DaemonTurnEngine(NodeApiClient* client,
                                   daemonapp::daemon::DaemonCacheStore* cache,
                                   daemonapp::daemon::SubscriptionManager* subscriptions,
                                   QObject* parent)
    : ITurnEngine(parent), m_client(client), m_cache(cache), m_subscriptions(subscriptions) {
    m_elapsedTimer.setInterval(1000);
    m_deadlineTimer.setSingleShot(true);
    m_deadlineTimer.setInterval(kDeadlineMs);
    m_resumeTimer.setSingleShot(true);

    connect(&m_resumeTimer, &QTimer::timeout, this, &DaemonTurnEngine::openSubscribeStream);
    connect(&m_elapsedTimer, &QTimer::timeout, this, [this] {
        setElapsedMs(static_cast<int>(QDateTime::currentMSecsSinceEpoch() - m_startedAt));
    });
    connect(&m_deadlineTimer, &QTimer::timeout, this,
            [this] { finishTurn(tr("The turn timed out waiting for the agent.")); });

    if (m_client != nullptr) {
        connect(m_client, &NodeApiClient::responseReady, this, &DaemonTurnEngine::onResponse);
        connect(m_client, &NodeApiClient::failed, this, &DaemonTurnEngine::onFailure);
        connect(m_client, &NodeApiClient::streamItem, this, &DaemonTurnEngine::onStreamItem);
        connect(m_client, &NodeApiClient::streamEnded, this, &DaemonTurnEngine::onStreamEnded);
        connect(m_client, &NodeApiClient::streamReset, this, &DaemonTurnEngine::onStreamReset);
    }
}

DaemonTurnEngine::~DaemonTurnEngine() {
    // Clear our focus registration so a SessionAdvanced no longer routes to this destroyed engine.
    if (m_subscriptions != nullptr && !m_sessionId.isEmpty()) {
        m_subscriptions->unregisterFocus(m_sessionId, this);
    }
}

void DaemonTurnEngine::setSessionId(const QString& sessionId) {
    if (sessionId == m_sessionId) {
        return;
    }
    // L3 focus wiring: move this engine's registration from the old session to the new, so the
    // SubscriptionManager nudges it when its *bound* session advances (per-tab, both front ends).
    if (m_subscriptions != nullptr) {
        if (!m_sessionId.isEmpty()) {
            m_subscriptions->unregisterFocus(m_sessionId, this);
        }
        if (!sessionId.isEmpty()) {
            m_subscriptions->registerFocus(sessionId, this);
        }
    }
    m_sessionId = sessionId;
    // Cold-start / refocus: resume the live subscribe past the persisted watermark so
    // already-cached entries are not re-streamed (the transcript renders from
    // CachedSessionStore::content()).
    m_cursor = 0;
    m_epoch = 0;
    m_epochKnown = false;
    // Any in-flight journal page loop belongs to the previous session; drop its accumulation.
    m_journalAcc.clear();
    m_journalPages = 0;
    if (m_cache != nullptr && !sessionId.isEmpty()) {
        const QString w = m_cache->cursor(dcache::sessionWatermarkScope(sessionId));
        const QString e = m_cache->cursor(dcache::sessionEpochScope(sessionId));
        if (!w.isEmpty()) {
            m_cursor = w.toULongLong();
        }
        if (!e.isEmpty()) {
            m_epoch = e.toULongLong();
            m_epochKnown = true;
        }
    }
}

QString DaemonTurnEngine::submitCorrelation() const {
    return QStringLiteral("turn/submit/") + m_sessionId;
}

QString DaemonTurnEngine::respondCorrelation() const {
    return QStringLiteral("turn/respond/") + m_sessionId;
}

QString DaemonTurnEngine::journalCorrelation() const {
    return QStringLiteral("turn/journal/") + m_sessionId;
}

void DaemonTurnEngine::start(const QString& prompt) {
    cancel();
    if (m_client == nullptr || m_sessionId.isEmpty()) {
        setErrorText(tr("No session is bound for this turn."));
        setTurnState(QStringLiteral("error"));
        return;
    }
    // Resume from the applied watermark (primed by setSessionId / advanced by prior turns), never
    // from 0: re-subscribing from 0 would replay the whole session log into the open document
    // (duplicate user echoes + assistant text) and a replayed old TurnFinished would end this
    // fresh turn early. A stale watermark/epoch is covered by the reset detection in
    // applyLogPage (epoch bump / head_seq < cursor -> journal re-baseline).
    m_subscribeStreamId = 0;
    m_finished = false;
    m_parked = false;
    m_pendingRequestId = 0;
    m_pendingKind.clear();
    m_pendingOptions.clear();
    m_todoCallIds.clear();
    m_reasoningSeq = 0;
    m_reasoningText.clear();
    m_resumeAttempts = 0;
    m_resumeBackoffMs = kResumeMinMs;
    m_startedAt = QDateTime::currentMSecsSinceEpoch();
    setElapsedMs(0);
    setErrorText(QString());
    setActive(true);
    setTurnState(QStringLiteral("thinking"));
    m_elapsedTimer.start();
    m_deadlineTimer.start();
    emit turnStarted();
    m_client->sendRequest(
        NodeApiCodec::encodeSubmitStartTurnRequest(m_sessionId, prompt, m_profile, m_requestId++),
        submitCorrelation());
}

void DaemonTurnEngine::cancel() {
    if (m_subscribeStreamId != 0 && m_client != nullptr) {
        m_client->cancelStream(m_subscribeStreamId);
    }
    m_subscribeStreamId = 0;
    m_resumeTimer.stop();
    m_elapsedTimer.stop();
    m_deadlineTimer.stop();
    m_parked = false;
    if (m_active) {
        setActive(false);
        if (m_turnState != QStringLiteral("error")) {
            setTurnState(QStringLiteral("idle"));
        }
    }
    m_finished = true;
}

void DaemonTurnEngine::resume() {
    // Bare resume: if parked without an explicit answer, just lift the gate; the open stream keeps
    // delivering once the daemon resumes (used by the base-class default / host-input flows).
    if (m_parked) {
        m_parked = false;
        m_deadlineTimer.start();
    }
}

void DaemonTurnEngine::parkOnRequest(const DecodedLogEntry& entry) {
    m_parked = true;
    m_pendingRequestId = entry.hostRequestId;
    m_pendingKind = entry.hostKind;
    m_pendingOptions = entry.hostOptions;
    m_resumeTimer.stop();
    // A parked turn is waiting on the user, not stuck: stop the hard deadline so it cannot be
    // killed mid-approval (it re-arms on respond). The push stream stays open and delivers the
    // continuation once the daemon resumes.
    m_deadlineTimer.stop();
    const QString id = QString::number(entry.hostRequestId);
    if (entry.hostKind == QStringLiteral("Approval")) {
        // Render an awaiting-approval tool row the inline ToolApprovalBar answers. The
        // "Allow permanently" affordance is shown only when the node advertised it on this gate
        // (wire v28 `allow_permanent_offered`) — the app never fabricates that offer.
        emit eventsEmitted(QVariantList{
            QVariantMap{{QStringLiteral("type"), QStringLiteral("toolStarted")},
                        {QStringLiteral("callId"), id},
                        {QStringLiteral("name"), QStringLiteral("approval")},
                        {QStringLiteral("argsSummary"), entry.hostPrompt},
                        {QStringLiteral("approvalCommand"), entry.hostPrompt},
                        {QStringLiteral("needsApproval"), true},
                        {QStringLiteral("allowPermanent"), entry.hostAllowPermanentOffered},
                        {QStringLiteral("requestId"), id}}});
        emit awaitingInput(QStringLiteral("approval"));
    } else if (entry.hostKind == QStringLiteral("Choice")) {
        // Render a clarify form: one single-select question carrying the options.
        QVariantList options;
        for (const QString& opt : entry.hostOptions) {
            options.append(
                QVariantMap{{QStringLiteral("id"), opt}, {QStringLiteral("label"), opt}});
        }
        const QVariantMap question{{QStringLiteral("id"), id},
                                   {QStringLiteral("prompt"), entry.hostPrompt},
                                   {QStringLiteral("kind"), QStringLiteral("single")},
                                   {QStringLiteral("options"), options}};
        emit eventsEmitted(
            QVariantList{QVariantMap{{QStringLiteral("type"), QStringLiteral("toolStarted")},
                                     {QStringLiteral("callId"), id},
                                     {QStringLiteral("name"), QStringLiteral("clarify")},
                                     {QStringLiteral("argsSummary"), entry.hostPrompt},
                                     {QStringLiteral("requestId"), id},
                                     {QStringLiteral("questions"), QVariantList{question}}}});
        emit awaitingInput(QStringLiteral("clarify"));
    } else {
        // Input: a masked / free-text host prompt the composer answers.
        emit awaitingInput(QStringLiteral("input"));
        emit hostRequested(QStringLiteral("input"), entry.hostPrompt);
    }
}

void DaemonTurnEngine::sendRespond(const QByteArray& cbor) {
    if (m_client == nullptr || cbor.isEmpty()) {
        return;
    }
    m_parked = false;
    m_deadlineTimer.start();
    m_client->sendRequest(cbor, respondCorrelation());
}

void DaemonTurnEngine::respondApproval(const QString& requestId, bool allow, bool allowPermanent) {
    if (!m_parked) {
        return;
    }
    const quint32 id = requestId.isEmpty() ? m_pendingRequestId : requestId.toUInt();
    // The encoder emits allow_permanent only for an allow (a deny is never persisted).
    sendRespond(NodeApiCodec::encodeRespondApprovalRequest(m_sessionId, id, allow, allowPermanent));
}

void DaemonTurnEngine::respondInput(const QString& requestId, const QString& text) {
    if (!m_parked) {
        return;
    }
    const quint32 id = requestId.isEmpty() ? m_pendingRequestId : requestId.toUInt();
    // A Choice gate answered with the option's text resolves to its index (the daemon expects
    // Chosen); otherwise it is free-text Input.
    if (m_pendingKind == QStringLiteral("Choice")) {
        const qsizetype idx = m_pendingOptions.indexOf(text);
        if (idx >= 0) {
            sendRespond(NodeApiCodec::encodeRespondChoiceRequest(m_sessionId, id,
                                                                 static_cast<quint32>(idx)));
            return;
        }
    }
    sendRespond(NodeApiCodec::encodeRespondInputRequest(m_sessionId, id, text));
}

void DaemonTurnEngine::respondChoice(const QString& requestId, int index) {
    if (!m_parked || index < 0) {
        return;
    }
    const quint32 id = requestId.isEmpty() ? m_pendingRequestId : requestId.toUInt();
    sendRespond(
        NodeApiCodec::encodeRespondChoiceRequest(m_sessionId, id, static_cast<quint32>(index)));
}

void DaemonTurnEngine::interrupt(const QString& reason) {
    if (m_client == nullptr || m_sessionId.isEmpty() || m_finished) {
        return;
    }
    // Interrupt the turn and keep draining: the daemon emits TurnFinished(Interrupted) which
    // settles the transcript via the normal Subscribe path.
    m_parked = false;
    m_client->sendRequest(NodeApiCodec::encodeSubmitInterruptRequest(m_sessionId, reason),
                          respondCorrelation());
}

void DaemonTurnEngine::steer(const QString& text) {
    if (m_client == nullptr || m_sessionId.isEmpty() || m_finished || text.isEmpty()) {
        return;
    }
    m_client->sendRequest(NodeApiCodec::encodeSubmitSteerRequest(m_sessionId, text, m_requestId++),
                          respondCorrelation());
    // The open subscribe stream delivers the steered continuation; nothing to re-poll.
}

void DaemonTurnEngine::nudge() {
    // L3 lazy-focus catch-up: the node announced this focused session advanced out of band. If we
    // already hold an open subscription (an active turn, or a prior catch-up) the push stream is
    // already delivering those entries, so the announcement is redundant. Otherwise open a
    // read-only catch-up subscription from the applied watermark (no Submit — this does not start a
    // turn) so the new entries render. Best-effort awareness, not turn lifecycle.
    if (m_client == nullptr || m_sessionId.isEmpty() || m_subscribeStreamId != 0) {
        return;
    }
    m_finished = false;
    openSubscribeStream();
}

void DaemonTurnEngine::openSubscribeStream() {
    if (m_finished || m_client == nullptr) {
        return;
    }
    m_subscribeStreamId = m_client->openStream(
        NodeApiCodec::encodeSubscribeRequest(m_sessionId, m_cursor, kSubscribeMax));
}

void DaemonTurnEngine::onStreamItem(quint64 id, const QByteArray& responseCbor) {
    if (id == m_subscribeStreamId && !m_finished) {
        applyLogPage(responseCbor);
    }
}

void DaemonTurnEngine::onStreamReset(quint64 id, quint64 epoch, quint64 /*headSeq*/) {
    if (id != m_subscribeStreamId || m_finished) {
        return;
    }
    // The live log generation we were following is gone (lossy lag / reactivation); re-baseline.
    m_epoch = epoch;
    rebaselineFromJournal();
}

void DaemonTurnEngine::onStreamEnded(quint64 id, bool error, const QString& message) {
    if (id != m_subscribeStreamId || m_finished) {
        return;
    }
    m_subscribeStreamId = 0;
    if (!error) {
        // A clean End (the daemon closed the stream, e.g. the session is gone) ends the turn.
        finishTurn(message);
        return;
    }
    // A transport drop with the daemon alive is recoverable: reopen from m_cursor (same epoch) with
    // backoff, up to the resume budget. A generation change arrives as a Reset, not an error End.
    if (m_resumeAttempts >= kResumeMaxAttempts) {
        finishTurn(message.isEmpty() ? tr("The connection to the agent was lost.") : message);
        return;
    }
    ++m_resumeAttempts;
    setTurnState(QStringLiteral("stalled"));
    m_resumeTimer.start(m_resumeBackoffMs);
    m_resumeBackoffMs = std::min(m_resumeBackoffMs * 2, kResumeMaxMs);
}

void DaemonTurnEngine::applyLogPage(const QByteArray& responseCbor) {
    QList<DecodedLogEntry> entries;
    quint64 nextSeq = m_cursor;
    quint64 headSeq = 0;
    quint64 pageEpoch = 0;
    if (!NodeApiCodec::decodeLogPageEntries(responseCbor, &entries, &nextSeq, &headSeq,
                                            &pageEpoch)) {
        finishTurn(tr("Failed to read the agent's response stream."));
        return;
    }
    // Reset detection (L2): a strictly greater epoch is a re-activation; a head_seq below our
    // watermark is a same-epoch truncation. Either way the live log is no longer the one we were
    // following - re-baseline from the durable journal.
    if (!m_epochKnown) {
        m_epoch = pageEpoch;
        m_epochKnown = true;
    } else if (pageEpoch > m_epoch || headSeq < m_cursor) {
        m_epoch = pageEpoch;
        rebaselineFromJournal();
        return;
    }
    // A good page means the connection is live again: clear any resume backoff + lift "stalled".
    m_resumeAttempts = 0;
    m_resumeBackoffMs = kResumeMinMs;
    if (m_turnState == QStringLiteral("stalled")) {
        setTurnState(QStringLiteral("thinking"));
    }
    for (const DecodedLogEntry& entry : entries) {
        if (entry.seq <= m_cursor) {
            continue; // applied-seq watermark: drop already-rendered entries (additive deltas)
        }
        m_cursor = entry.seq;
        if (entry.payloadKind == DecodedLogEntry::PayloadKind::Request) {
            parkOnRequest(entry);
            return;
        }
        if (entry.payloadKind == DecodedLogEntry::PayloadKind::Command) {
            // The node's echo of an inbound user command (StartTurn/Steer): render the user's
            // message node-authoritatively — the same log entry every other observer sees — and
            // persist it as a durable Message block so a reload/cold start replays it from cache.
            if (entry.direction == QStringLiteral("Inbound") && !entry.commandText.isEmpty()) {
                emit eventsEmitted(QVariantList{
                    QVariantMap{{QStringLiteral("type"), QStringLiteral("userMessage")},
                                {QStringLiteral("text"), entry.commandText}}});
                if (m_cache != nullptr && !m_sessionId.isEmpty()) {
                    daemonapp::daemon::CachedTranscriptBlockRow b;
                    b.sessionId = m_sessionId;
                    b.seq = entry.seq;
                    b.kind = QStringLiteral("Message");
                    b.role = QStringLiteral("User");
                    b.text = entry.commandText;
                    b.updatedAtMs = QDateTime::currentMSecsSinceEpoch();
                    m_cache->upsertTranscriptBlock(b);
                }
            }
            continue;
        }
        if (entry.payloadKind != DecodedLogEntry::PayloadKind::Event) {
            continue; // meta / host-response frames carry no transcript content
        }
        if (entry.event.kind == AgentEventKind::Error) {
            setErrorText(entry.event.text);
        }
        // Reasoning persistence: deltas accumulate into one durable Reasoning block per run
        // (mirroring the live ingest's coalescing into a single disclosure card); any CONTENT
        // event settles the open run, so a reload renders the same reasoning blocks in order.
        // Status-only events (Usage/Context) interleave mid-disclosure and must not split it —
        // the same rule the live ingest applies.
        if (entry.event.kind == AgentEventKind::ReasoningDelta) {
            if (m_reasoningSeq == 0) {
                m_reasoningSeq = entry.seq;
            }
            m_reasoningText += entry.event.text;
        } else if (entry.event.kind == AgentEventKind::TextDelta ||
                   entry.event.kind == AgentEventKind::ToolStarted ||
                   entry.event.kind == AgentEventKind::ToolFinished ||
                   entry.event.kind == AgentEventKind::TurnFinished ||
                   entry.event.kind == AgentEventKind::Error) {
            settleReasoningBlock();
        }
        // The `todo` tool is the status stack's structured feed, not transcript content: suppress
        // its inline tool blocks (never rendered, never cached) and surface the finished result's
        // detail as a `todoUpdate` the orchestrator applies to the TodoListModel.
        if (entry.event.kind == AgentEventKind::ToolStarted &&
            entry.event.toolName == QStringLiteral("todo")) {
            m_todoCallIds.insert(entry.event.callId);
            continue;
        }
        if (entry.event.kind == AgentEventKind::ToolFinished) {
            const bool startedAsTodo = m_todoCallIds.remove(entry.event.callId);
            if (entry.event.detailKind == QStringLiteral("todo")) {
                emit eventsEmitted(QVariantList{QVariantMap{
                    {QStringLiteral("type"), QStringLiteral("todoUpdate")},
                    {QStringLiteral("items"), todoItemsFromDetail(entry.event.detailBody)}}});
                continue;
            }
            if (startedAsTodo) {
                continue; // a failed todo call (no detail): suppressed, list left as-is
            }
        }
        const QVariantMap mapped = toIngestEvent(entry.event);
        if (!mapped.isEmpty()) {
            if (mapped.value(QStringLiteral("type")).toString() == QStringLiteral("text")) {
                setTurnState(QStringLiteral("streaming"));
            }
            emit eventsEmitted(QVariantList{mapped});
        }
        // L3 render-from-cache: persist the durable-content events as coalesced transcript blocks
        // keyed by seq (TextDeltas are transient; the final assistant message lands on
        // TurnFinished).
        if (m_cache != nullptr && !m_sessionId.isEmpty()) {
            const qint64 now = QDateTime::currentMSecsSinceEpoch();
            if (entry.event.kind == AgentEventKind::ToolStarted) {
                daemonapp::daemon::CachedTranscriptBlockRow b;
                b.sessionId = m_sessionId;
                b.seq = entry.seq;
                b.kind = QStringLiteral("ToolCall");
                b.callId = entry.event.callId;
                b.toolName = entry.event.toolName;
                b.argsSummary = entry.event.toolArgs;
                b.updatedAtMs = now;
                m_cache->upsertTranscriptBlock(b);
            } else if (entry.event.kind == AgentEventKind::ToolFinished) {
                daemonapp::daemon::CachedTranscriptBlockRow b;
                b.sessionId = m_sessionId;
                b.seq = entry.seq;
                b.kind = QStringLiteral("ToolResult");
                b.callId = entry.event.callId;
                b.ok = entry.event.toolOk;
                // D1: persist the rich result (full content + typed detail) so a reload / history
                // scroll-back re-renders the same card the live turn showed.
                b.summary = cappedSummary(entry.event.toolSummary);
                b.detailKind = entry.event.detailKind;
                b.detailBody = entry.event.detailBody;
                b.updatedAtMs = now;
                m_cache->upsertTranscriptBlock(b);
            } else if (entry.event.kind == AgentEventKind::TurnFinished &&
                       !entry.event.finalText.isEmpty()) {
                daemonapp::daemon::CachedTranscriptBlockRow b;
                b.sessionId = m_sessionId;
                b.seq = entry.seq;
                b.kind = QStringLiteral("Message");
                b.role = QStringLiteral("Assistant");
                b.text = entry.event.finalText;
                b.updatedAtMs = now;
                m_cache->upsertTranscriptBlock(b);
            }
        }
        if (entry.event.kind == AgentEventKind::TurnFinished) {
            finishTurn(entry.event.turnCompleted
                           ? QString()
                           : (m_errorText.isEmpty()
                                  ? tr("The turn ended: %1").arg(entry.event.endReason)
                                  : m_errorText));
            return;
        }
    }
    // A page boundary mid-reasoning: checkpoint the accumulated text so a crash/refocus renders
    // what has streamed so far (the run stays open for the next page's deltas).
    checkpointReasoningBlock();
    persistWatermark();
}

void DaemonTurnEngine::rebaselineFromJournal() {
    // The live log generation changed (restart / reactivation), so the in-flight turn is gone.
    // Stop the dead stream and fetch the durable journal to recover the conversation.
    if (m_subscribeStreamId != 0 && m_client != nullptr) {
        m_client->cancelStream(m_subscribeStreamId);
        m_subscribeStreamId = 0;
    }
    m_resumeTimer.stop();
    setTurnState(QStringLiteral("stalled"));
    // A fresh re-baseline restarts the page loop from cursor 0: drop anything a superseded loop
    // accumulated so the eventual replay reflects exactly one generation.
    m_journalAcc.clear();
    m_journalPages = 0;
    if (m_client != nullptr) {
        m_client->sendRequest(
            NodeApiCodec::encodeSessionHistoryRequest(m_sessionId, 0, kSubscribeMax),
            journalCorrelation());
    }
}

void DaemonTurnEngine::onResponse(const QString& correlationId, const QByteArray& responseCbor) {
    if (correlationId == submitCorrelation()) {
        // Any non-error acknowledgement (Ok / Routed / other) means the turn was accepted; open the
        // push subscription. Only an explicit Error aborts the turn.
        if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Error) {
            daemonapp::daemon::DecodedApiError err;
            NodeApiCodec::decodeError(responseCbor, &err);
            finishTurn(err.message.isEmpty() ? tr("The agent could not start the turn.")
                                             : err.message);
        } else {
            openSubscribeStream();
        }
        return;
    }

    if (correlationId == respondCorrelation()) {
        // A Respond / Interrupt / Steer was accepted; the open stream delivers the continuation. An
        // explicit Error aborts the turn.
        if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Error) {
            daemonapp::daemon::DecodedApiError err;
            NodeApiCodec::decodeError(responseCbor, &err);
            finishTurn(err.message.isEmpty() ? tr("The agent rejected the response.")
                                             : err.message);
        }
        return;
    }

    if (correlationId == journalCorrelation()) {
        // One page of the re-baseline read (wire v24: SessionHistory pages are bounded at
        // kWirePageMax records). Accumulate, and keep paging while the durable head lies past
        // this page's cursor; the replay below runs exactly once over the full set.
        QList<daemonapp::daemon::DecodedJournalRecord> pageRecords;
        quint64 nextCursor = 0;
        quint64 headCursor = 0;
        const bool decoded =
            NodeApiCodec::decodeJournal(responseCbor, &pageRecords, &nextCursor, &headCursor);
        if (decoded) {
            m_journalAcc.append(pageRecords);
        }
        const bool morePages = decoded && !pageRecords.isEmpty() && nextCursor < headCursor;
        if (morePages && m_journalPages < kMaxJournalPages && m_client != nullptr) {
            ++m_journalPages;
            m_client->sendRequest(
                NodeApiCodec::encodeSessionHistoryRequest(m_sessionId, nextCursor, kSubscribeMax),
                journalCorrelation());
            return;
        }
        const QList<daemonapp::daemon::DecodedJournalRecord> records = std::move(m_journalAcc);
        m_journalAcc = {};
        m_journalPages = 0;
        // Re-baseline replay: render the durable conversation (coalesced blocks), re-surface an
        // unanswered parked Request, then finish - the interrupted turn ended with the reset.
        emit eventsEmitted(
            QVariantList{QVariantMap{{QStringLiteral("type"), QStringLiteral("flush")}}});
        // L3: the journal is the authoritative coalesced transcript — rebuild the durable cache
        // from it so a subsequent refocus/cold-start renders from disk. Wipe the stale generation
        // first (this drops locally-persisted Reasoning blocks too: the node's journal does not
        // retain disclosures, and stale-generation seqs cannot be trusted against the new log).
        if (m_cache != nullptr && !m_sessionId.isEmpty()) {
            m_cache->clearTranscript(m_sessionId);
        }
        m_reasoningSeq = 0;
        m_reasoningText.clear();
        bool parked = false;
        QSet<QString> replayTodoCalls;
        for (const daemonapp::daemon::DecodedJournalRecord& rec : records) {
            if (!rec.isBlock) {
                continue;
            }
            using Kind = daemonapp::daemon::DecodedTranscriptBlock::Kind;
            const auto& b = rec.block;
            // The `todo` tool's blocks are the status stack's feed, not transcript content: skip
            // them in the durable rebuild + replay (a replayed turn is over, the stack stays
            // empty; the live path surfaces them as todoUpdate).
            if (b.kind == Kind::ToolCall && b.toolName == QStringLiteral("todo")) {
                replayTodoCalls.insert(b.callId);
                continue;
            }
            if (b.kind == Kind::ToolResult && replayTodoCalls.contains(b.callId)) {
                continue;
            }
            if (m_cache != nullptr && !m_sessionId.isEmpty()) {
                daemonapp::daemon::CachedTranscriptBlockRow row;
                row.sessionId = m_sessionId;
                row.seq = rec.seq;
                row.role = b.role;
                row.text = b.text;
                row.callId = b.callId;
                row.toolName = b.toolName;
                row.argsSummary = b.argsSummary;
                row.ok = b.ok;
                row.summary = cappedSummary(b.summary);
                row.detailKind = b.detailKind;
                row.detailBody = b.detailBody;
                row.requestId = b.requestId;
                row.hostKind = b.hostKind;
                row.contentKind = b.contentKind;
                row.updatedAtMs = QDateTime::currentMSecsSinceEpoch();
                switch (b.kind) {
                case Kind::Message:
                    row.kind = QStringLiteral("Message");
                    break;
                case Kind::ToolCall:
                    row.kind = QStringLiteral("ToolCall");
                    break;
                case Kind::ToolResult:
                    row.kind = QStringLiteral("ToolResult");
                    break;
                case Kind::Request:
                    row.kind = QStringLiteral("Request");
                    break;
                case Kind::Content:
                    row.kind = QStringLiteral("Content");
                    break;
                default:
                    row.kind = QStringLiteral("Other");
                    break;
                }
                m_cache->upsertTranscriptBlock(row);
            }
            switch (b.kind) {
            case Kind::Message:
                if (!b.text.isEmpty()) {
                    emit eventsEmitted(
                        QVariantList{QVariantMap{{QStringLiteral("type"), QStringLiteral("text")},
                                                 {QStringLiteral("text"), b.text}}});
                    emit eventsEmitted(QVariantList{
                        QVariantMap{{QStringLiteral("type"), QStringLiteral("flush")}}});
                }
                break;
            case Kind::ToolCall:
                emit eventsEmitted(QVariantList{
                    QVariantMap{{QStringLiteral("type"), QStringLiteral("toolStarted")},
                                {QStringLiteral("callId"), b.callId},
                                {QStringLiteral("name"), b.toolName},
                                {QStringLiteral("argsSummary"), b.argsSummary}}});
                break;
            case Kind::ToolResult: {
                // D1: the journal retains the rich result - replay it exactly as the live path
                // emits it, so a re-baseline renders the same card.
                QVariantMap finished{{QStringLiteral("type"), QStringLiteral("toolFinished")},
                                     {QStringLiteral("callId"), b.callId},
                                     {QStringLiteral("status"),
                                      b.ok ? QStringLiteral("ok") : QStringLiteral("error")}};
                if (!b.summary.isEmpty()) {
                    finished.insert(QStringLiteral("summary"), cappedSummary(b.summary));
                }
                if (!b.detailKind.isEmpty()) {
                    finished.insert(QStringLiteral("detailKind"), b.detailKind);
                    finished.insert(QStringLiteral("detailBody"), QString::fromUtf8(b.detailBody));
                }
                emit eventsEmitted(QVariantList{finished});
                break;
            }
            case Kind::Request: {
                // Re-surface an unanswered host gate from the durable record (best-effort: the
                // block carries the id + kind, not the live prompt).
                DecodedLogEntry e;
                e.payloadKind = DecodedLogEntry::PayloadKind::Request;
                e.hostRequestId = b.requestId;
                e.hostKind = b.hostKind;
                parkOnRequest(e);
                parked = true;
                break;
            }
            default:
                break;
            }
            if (parked) {
                break;
            }
        }
        // Persist the applied watermark so a later cold-start renders from the transcript cache and
        // resumes the live stream past here. (The separate session:journal cursor was retired in
        // the Phase 4 closeout: rebaselineFromJournal always replays from seq 0, so a
        // delta-from-cursor cold-start is a future optimization, not a current code path.)
        persistWatermark();
        if (!parked) {
            finishTurn(m_errorText.isEmpty() ? tr("The session was reset; recovered from history.")
                                             : m_errorText);
        }
        return;
    }
}

void DaemonTurnEngine::onFailure(const QString& correlationId, const QString& message) {
    // Submit / Respond / Journal one-shots are unrecoverable for this turn - error out. (A live
    // stream drop arrives via onStreamEnded(error), which reopens with backoff instead.)
    if (correlationId == submitCorrelation() || correlationId == respondCorrelation() ||
        correlationId == journalCorrelation()) {
        finishTurn(message.isEmpty() ? tr("The connection to the agent was lost.") : message);
    }
}

void DaemonTurnEngine::persistWatermark() {
    if (m_cache == nullptr || m_sessionId.isEmpty()) {
        return;
    }
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    m_cache->setCursor(dcache::sessionWatermarkScope(m_sessionId), QString::number(m_cursor), now);
    m_cache->setCursor(dcache::sessionEpochScope(m_sessionId), QString::number(m_epoch), now);
}

void DaemonTurnEngine::checkpointReasoningBlock() {
    if (m_reasoningSeq == 0 || m_cache == nullptr || m_sessionId.isEmpty()) {
        return;
    }
    daemonapp::daemon::CachedTranscriptBlockRow b;
    b.sessionId = m_sessionId;
    b.seq = m_reasoningSeq; // upsert key: re-checkpointing the same run is idempotent
    b.kind = QStringLiteral("Reasoning");
    b.text = m_reasoningText;
    b.updatedAtMs = QDateTime::currentMSecsSinceEpoch();
    m_cache->upsertTranscriptBlock(b);
}

void DaemonTurnEngine::settleReasoningBlock() {
    checkpointReasoningBlock();
    m_reasoningSeq = 0;
    m_reasoningText.clear();
}

void DaemonTurnEngine::finishTurn(const QString& errorText) {
    if (m_finished && !m_active) {
        return;
    }
    if (m_subscribeStreamId != 0 && m_client != nullptr) {
        m_client->cancelStream(m_subscribeStreamId);
        m_subscribeStreamId = 0;
    }
    m_resumeTimer.stop();
    m_elapsedTimer.stop();
    m_deadlineTimer.stop();
    m_finished = true;
    settleReasoningBlock(); // a turn ending mid-disclosure still persists what streamed
    persistWatermark();
    if (!errorText.isEmpty()) {
        setErrorText(errorText);
    }
    // Close any open assistant stream the same way the simulator does.
    emit eventsEmitted(
        QVariantList{QVariantMap{{QStringLiteral("type"), QStringLiteral("flush")}}});
    setActive(false);
    setTurnState(m_errorText.isEmpty() ? QStringLiteral("idle") : QStringLiteral("error"));
    emit turnFinished();
}

void DaemonTurnEngine::setActive(bool active) {
    if (m_active == active) {
        return;
    }
    m_active = active;
    emit activeChanged();
}

void DaemonTurnEngine::setTurnState(const QString& state) {
    if (m_turnState == state) {
        return;
    }
    m_turnState = state;
    emit turnStateChanged();
}

void DaemonTurnEngine::setElapsedMs(int ms) {
    if (m_elapsedMs == ms) {
        return;
    }
    m_elapsedMs = ms;
    emit elapsedMsChanged();
}

void DaemonTurnEngine::setErrorText(const QString& text) {
    if (m_errorText == text) {
        return;
    }
    m_errorText = text;
    emit errorTextChanged();
}
