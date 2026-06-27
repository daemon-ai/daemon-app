#include "daemon_turn_engine.h"

#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"

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
    case AgentEventKind::TurnFinished:
        return QVariantMap{{QStringLiteral("type"), QStringLiteral("flush")}};
    default:
        // ToolStarted/ToolFinished/Usage/Context/etc. are CHA-3 work; not rendered in this slice.
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

void DaemonTurnEngine::start(const QString& prompt) {
    cancel();
    if (m_client == nullptr || m_sessionId.isEmpty()) {
        setErrorText(tr("No session is bound for this turn."));
        setTurnState(QStringLiteral("error"));
        return;
    }
    m_cursor = 0;
    m_finished = false;
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
    m_elapsedTimer.stop();
    m_deadlineTimer.stop();
    if (m_active) {
        setActive(false);
        if (m_turnState != QStringLiteral("error")) {
            setTurnState(QStringLiteral("idle"));
        }
    }
    m_finished = true;
}

void DaemonTurnEngine::resume() {
    // Host-gate resume (approval/clarify) is CHA-4/5; not part of the basic-chat slice.
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

    if (correlationId == subscribeCorrelation()) {
        QList<DecodedLogEntry> entries;
        quint64 nextSeq = m_cursor;
        if (!NodeApiCodec::decodeLogPageEntries(responseCbor, &entries, &nextSeq)) {
            finishTurn(tr("Failed to read the agent's response stream."));
            return;
        }
        for (const DecodedLogEntry& entry : entries) {
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
    if (correlationId == submitCorrelation() || correlationId == subscribeCorrelation()) {
        finishTurn(message.isEmpty() ? tr("The connection to the agent was lost.") : message);
    }
}

void DaemonTurnEngine::finishTurn(const QString& errorText) {
    if (m_finished && !m_active) {
        return;
    }
    m_pollTimer.stop();
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
