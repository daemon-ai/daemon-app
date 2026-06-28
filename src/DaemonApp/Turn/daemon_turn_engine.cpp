#include "daemon_turn_engine.h"

#include "daemon/client_cache_schema.h"
#include "daemon/daemon_cache_store.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon/subscription_manager.h"

#include <algorithm>
#include <QDateTime>
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
    case AgentEventKind::ToolFinished:
        return QVariantMap{{QStringLiteral("type"), QStringLiteral("toolFinished")},
                           {QStringLiteral("callId"), event.callId},
                           {QStringLiteral("status"),
                            event.toolOk ? QStringLiteral("ok") : QStringLiteral("error")}};
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
    m_cursor = 0;
    m_epoch = 0;
    m_epochKnown = false;
    m_subscribeStreamId = 0;
    m_finished = false;
    m_parked = false;
    m_pendingRequestId = 0;
    m_pendingKind.clear();
    m_pendingOptions.clear();
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
        // Render an awaiting-approval tool row the inline ToolApprovalBar answers.
        emit eventsEmitted(
            QVariantList{QVariantMap{{QStringLiteral("type"), QStringLiteral("toolStarted")},
                                     {QStringLiteral("callId"), id},
                                     {QStringLiteral("name"), QStringLiteral("approval")},
                                     {QStringLiteral("argsSummary"), entry.hostPrompt},
                                     {QStringLiteral("approvalCommand"), entry.hostPrompt},
                                     {QStringLiteral("needsApproval"), true},
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

void DaemonTurnEngine::respondApproval(const QString& requestId, bool allow) {
    if (!m_parked) {
        return;
    }
    const quint32 id = requestId.isEmpty() ? m_pendingRequestId : requestId.toUInt();
    sendRespond(NodeApiCodec::encodeRespondApprovalRequest(m_sessionId, id, allow));
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
        if (entry.payloadKind != DecodedLogEntry::PayloadKind::Event) {
            continue; // skip the inbound user command echo, meta, etc.
        }
        if (entry.event.kind == AgentEventKind::Error) {
            setErrorText(entry.event.text);
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
        // Re-baseline replay: render the durable conversation (coalesced blocks), re-surface an
        // unanswered parked Request, then finish - the interrupted turn ended with the reset.
        QList<daemonapp::daemon::DecodedJournalRecord> records;
        quint64 journalNext = 0;
        NodeApiCodec::decodeJournal(responseCbor, &records, &journalNext);
        emit eventsEmitted(
            QVariantList{QVariantMap{{QStringLiteral("type"), QStringLiteral("flush")}}});
        // L3: the journal is the authoritative coalesced transcript — rebuild the durable cache
        // from it so a subsequent refocus/cold-start renders from disk. Wipe the stale generation
        // first.
        if (m_cache != nullptr && !m_sessionId.isEmpty()) {
            m_cache->clearTranscript(m_sessionId);
        }
        bool parked = false;
        for (const daemonapp::daemon::DecodedJournalRecord& rec : records) {
            if (!rec.isBlock) {
                continue;
            }
            using Kind = daemonapp::daemon::DecodedTranscriptBlock::Kind;
            const auto& b = rec.block;
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
                row.summary = b.summary;
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
            case Kind::ToolResult:
                emit eventsEmitted(QVariantList{
                    QVariantMap{{QStringLiteral("type"), QStringLiteral("toolFinished")},
                                {QStringLiteral("callId"), b.callId},
                                {QStringLiteral("status"),
                                 b.ok ? QStringLiteral("ok") : QStringLiteral("error")}}});
                break;
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
        // Persist the journal + watermark cursors so a later cold-start replays the cache and only
        // fetches the delta past here.
        if (m_cache != nullptr && !m_sessionId.isEmpty() && journalNext > 0) {
            m_cache->setCursor(dcache::sessionJournalScope(m_sessionId),
                               QString::number(journalNext), QDateTime::currentMSecsSinceEpoch());
        }
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
