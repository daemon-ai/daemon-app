#include "daemon_turn_engine.h"

#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"

#include <algorithm>
#include <QDateTime>
#include <QVariantList>
#include <QVariantMap>

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

DaemonTurnEngine::DaemonTurnEngine(NodeApiClient* client, QObject* parent)
    : ITurnEngine(parent), m_client(client) {
    m_pollTimer.setSingleShot(true);
    m_pollTimer.setInterval(kPollIntervalMs);
    m_elapsedTimer.setInterval(1000);
    m_deadlineTimer.setSingleShot(true);
    m_deadlineTimer.setInterval(kDeadlineMs);
    m_resumeTimer.setSingleShot(true);

    connect(&m_resumeTimer, &QTimer::timeout, this, &DaemonTurnEngine::pollSubscribe);
    connect(&m_pollTimer, &QTimer::timeout, this, &DaemonTurnEngine::pollSubscribe);
    connect(&m_elapsedTimer, &QTimer::timeout, this, [this] {
        setElapsedMs(static_cast<int>(QDateTime::currentMSecsSinceEpoch() - m_startedAt));
    });
    connect(&m_deadlineTimer, &QTimer::timeout, this,
            [this] { finishTurn(tr("The turn timed out waiting for the agent.")); });

    if (m_client != nullptr) {
        connect(m_client, &NodeApiClient::responseReady, this, &DaemonTurnEngine::onResponse);
        connect(m_client, &NodeApiClient::failed, this, &DaemonTurnEngine::onFailure);
    }
}

QString DaemonTurnEngine::submitCorrelation() const {
    return QStringLiteral("turn/submit/") + m_sessionId;
}

QString DaemonTurnEngine::subscribeCorrelation() const {
    return QStringLiteral("turn/subscribe/") + m_sessionId;
}

QString DaemonTurnEngine::respondCorrelation() const {
    return QStringLiteral("turn/respond/") + m_sessionId;
}

void DaemonTurnEngine::start(const QString& prompt) {
    cancel();
    if (m_client == nullptr || m_sessionId.isEmpty()) {
        setErrorText(tr("No session is bound for this turn."));
        setTurnState(QStringLiteral("error"));
        return;
    }
    m_cursor = 0;
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
    m_pollTimer.stop();
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
    // Bare resume: if parked without an explicit answer, just continue draining (used by the
    // base-class default and by host-input flows that supply the answer separately).
    if (m_parked) {
        m_parked = false;
        m_deadlineTimer.start();
        pollSubscribe();
    }
}

void DaemonTurnEngine::parkOnRequest(const DecodedLogEntry& entry) {
    m_parked = true;
    m_pendingRequestId = entry.hostRequestId;
    m_pendingKind = entry.hostKind;
    m_pendingOptions = entry.hostOptions;
    m_pollTimer.stop();
    m_resumeTimer.stop();
    // A parked turn is waiting on the user, not stuck: stop the hard deadline so it cannot be
    // killed mid-approval (it re-arms on resume).
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
    if (!m_parked && !m_pollTimer.isActive()) {
        pollSubscribe();
    }
}

void DaemonTurnEngine::pollSubscribe() {
    if (m_finished || m_client == nullptr) {
        return;
    }
    m_client->sendRequest(
        NodeApiCodec::encodeSubscribeRequest(m_sessionId, m_cursor, kSubscribeMax),
        subscribeCorrelation());
}

void DaemonTurnEngine::onResponse(const QString& correlationId, const QByteArray& responseCbor) {
    if (correlationId == submitCorrelation()) {
        // Any non-error acknowledgement (Ok / Routed / other) means the turn was accepted; begin
        // draining the session log from the start. Only an explicit Error aborts the turn.
        if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Error) {
            daemonapp::daemon::DecodedApiError err;
            NodeApiCodec::decodeError(responseCbor, &err);
            finishTurn(err.message.isEmpty() ? tr("The agent could not start the turn.")
                                             : err.message);
        } else {
            pollSubscribe();
        }
        return;
    }

    if (correlationId == respondCorrelation()) {
        // A Respond / Interrupt / Steer was accepted: resume draining the log from the cursor.
        if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Error) {
            daemonapp::daemon::DecodedApiError err;
            NodeApiCodec::decodeError(responseCbor, &err);
            finishTurn(err.message.isEmpty() ? tr("The agent rejected the response.")
                                             : err.message);
        } else if (!m_finished) {
            pollSubscribe();
        }
        return;
    }

    if (correlationId == subscribeCorrelation()) {
        QList<DecodedLogEntry> entries;
        quint64 nextSeq = m_cursor;
        if (!NodeApiCodec::decodeLogPageEntries(responseCbor, &entries, &nextSeq)) {
            finishTurn(tr("Failed to read the agent's response stream."));
            return;
        }
        // A good page means the connection is live again: clear any resume backoff and lift the
        // transient "stalled" state (text below may bump it to "streaming").
        m_resumeAttempts = 0;
        m_resumeBackoffMs = kResumeMinMs;
        if (m_turnState == QStringLiteral("stalled")) {
            setTurnState(QStringLiteral("thinking"));
        }
        for (const DecodedLogEntry& entry : entries) {
            if (entry.payloadKind == DecodedLogEntry::PayloadKind::Request) {
                // A parked HostRequest (approval / clarify): surface the gate and stop polling
                // until the user answers. Advance the cursor past it so resuming reads what
                // follows.
                m_cursor = entry.seq;
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
                const QString type = mapped.value(QStringLiteral("type")).toString();
                if (type == QStringLiteral("text")) {
                    setTurnState(QStringLiteral("streaming"));
                }
                emit eventsEmitted(QVariantList{mapped});
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
        m_cursor = nextSeq;
        if (!m_finished) {
            m_pollTimer.start(); // re-arm the next poll
        }
        return;
    }
}

void DaemonTurnEngine::onFailure(const QString& correlationId, const QString& message) {
    // Submit-phase failure: the turn was never accepted, so there is nothing to resume - error out.
    if (correlationId == submitCorrelation() || correlationId == respondCorrelation()) {
        finishTurn(message.isEmpty() ? tr("The connection to the agent was lost.") : message);
        return;
    }
    // Subscribe-phase failure: the turn is in flight on the daemon and m_cursor marks how far we
    // read, so a dropped socket is recoverable. Suspend into "stalled" and retry from m_cursor with
    // backoff (the shared transport reconnects lazily under pollSubscribe) until the resume budget
    // is exhausted, only then giving up.
    if (correlationId == subscribeCorrelation()) {
        if (m_finished) {
            return;
        }
        if (m_resumeAttempts >= kResumeMaxAttempts) {
            finishTurn(message.isEmpty() ? tr("The connection to the agent was lost.") : message);
            return;
        }
        ++m_resumeAttempts;
        setTurnState(QStringLiteral("stalled"));
        m_resumeTimer.start(m_resumeBackoffMs);
        m_resumeBackoffMs = std::min(m_resumeBackoffMs * 2, kResumeMaxMs);
    }
}

void DaemonTurnEngine::finishTurn(const QString& errorText) {
    if (m_finished && !m_active) {
        return;
    }
    m_pollTimer.stop();
    m_resumeTimer.stop();
    m_elapsedTimer.stop();
    m_deadlineTimer.stop();
    m_finished = true;
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
