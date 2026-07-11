// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/mock_scenario_host.h"

#include "mirror/mirror_service.h"
#include "outbox.h"
#include "verb_class.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

namespace daemonapp::daemon {

namespace {
constexpr int kTimelineTickMs = 250;
constexpr int kExpirySweepMs = 60'000;
constexpr qint64 kAcceptedGraceMs = 10LL * 60 * 1000; // §6.6 SPEC-DECISION default
constexpr int kDefaultTimeoutMs = 3'000;
} // namespace

MockScenarioHost::MockScenarioHost(mirror::MirrorService* svc, mirror::Outbox* outbox,
                                   MockScenario scenario, QObject* parent)
    : QObject(parent), m_svc(svc), m_outbox(outbox), m_scenario(std::move(scenario)),
      m_seeder(svc->store()), m_player(m_seeder, m_scenario.mirror), m_pager(*this) {
    // One batch through the same apply pipeline (§4.2/§9): the mock mirror is live from boot.
    m_player.playSeed();

    connect(m_outbox, &mirror::Outbox::sendRequested, this, &MockScenarioHost::onSendRequested);

    // The scripted timeline advances on wall time, deterministically ordered by the player.
    m_elapsed.start();
    m_timelineTimer.setInterval(kTimelineTickMs);
    connect(&m_timelineTimer, &QTimer::timeout, this, [this] {
        m_player.advanceTo(m_elapsed.elapsed());
        if (m_player.timelineExhausted()) {
            m_timelineTimer.stop();
        }
    });
    if (!m_player.timelineExhausted()) {
        m_timelineTimer.start();
    }

    // Robustness sweep (§6.6 accepted-state disposition): an acked op whose scripted outcome
    // carried no echo is disposed after the grace window instead of lingering.
    m_expirySweep.setInterval(kExpirySweepMs);
    connect(&m_expirySweep, &QTimer::timeout, this, [this] {
        m_outbox->expireAcceptedOps(QDateTime::currentMSecsSinceEpoch(), kAcceptedGraceMs);
    });
    m_expirySweep.start();
}

void MockScenarioHost::ScriptedPager::execute(const mirror::FetchJob& job) {
    executed.append(job);
    // Async completion: complete() pumps the next job; doing that inside execute() would re-enter
    // the scheduler mid-enqueue.
    const QString key = job.coalesceKey();
    QTimer::singleShot(0, &host,
                       [&hostRef = host, key] { hostRef.m_svc->scheduler().complete(key); });
}

void MockScenarioHost::onConnectionReady() {
    const int api = apiVersion();
    // The mock Hello advertised api/<N> (§9): the SAME per-connection wiring the daemon graph
    // runs on AuthOk — mode select (§5.6), provenance capability + auto-replay gate (§6.6/§6.8).
    m_svc->ingestor().onConnected(api, /*hasRevFields=*/true);
    m_outbox->setProvenanceCapable(mirror::Outbox::autoReplayEnabled(api));
    if (mirror::Outbox::autoReplayEnabled(api)) {
        m_outbox->drain();
    }
}

void MockScenarioHost::onConnectionLost() {
    m_svc->ingestor().onDisconnected();
}

void MockScenarioHost::advanceTimelineTo(qint64 elapsedMs) {
    m_player.advanceTo(elapsedMs);
    if (m_player.timelineExhausted()) {
        m_timelineTimer.stop();
    }
}

void MockScenarioHost::onSendRequested(const mirror::PendingOp& op) {
    // Dispatch lanes (turn prompts) are consumed by their own consumer at drain — never answered
    // here (§6.6 lane kinds).
    if (mirror::laneKind(mirror::verbClass(op.verb)) != mirror::LaneKind::Mutation) {
        return;
    }
    const QJsonObject args = QJsonDocument::fromJson(op.payload).object();
    const mirror::VerbOutcome outcome = m_player.scenario().verbScript.outcomeFor(op.verb, args);
    const QString opId = op.opId;

    switch (outcome.kind) {
    case mirror::VerbOutcomeKind::Reject:
        QTimer::singleShot(outcome.delayMs, this,
                           [this, opId, kind = outcome.errorKind, msg = outcome.errorMessage] {
                               m_outbox->onRejection(opId, kind, msg);
                           });
        return;
    case mirror::VerbOutcomeKind::Timeout: {
        const int ms = outcome.delayMs > 0 ? outcome.delayMs : kDefaultTimeoutMs;
        QTimer::singleShot(ms, this, [this, opId] {
            m_outbox->onTransientFailure(opId, QStringLiteral("mock timeout (scripted)"));
        });
        return;
    }
    case mirror::VerbOutcomeKind::Ok:
    case mirror::VerbOutcomeKind::Delay:
        QTimer::singleShot(outcome.delayMs, this, [this, opId] { m_outbox->onAck(opId); });
        if (outcome.echoDelayMs > 0 && op.verb == QStringLiteral("ConvSend")) {
            QTimer::singleShot(outcome.delayMs + outcome.echoDelayMs, this,
                               [this, op] { applyConvSendEcho(op); });
        }
        if (outcome.echoDelayMs > 0 && op.verb == QStringLiteral("SessionUpdateMeta")) {
            QTimer::singleShot(outcome.delayMs + outcome.echoDelayMs, this,
                               [this, op] { applySessionMetaEcho(op); });
        }
        return;
    }
}

void MockScenarioHost::applyConvSendEcho(const mirror::PendingOp& op) {
    // The mock twin of the node's read-path echo (§6.6/§10.3): the sent line lands as a
    // provenance-stamped mirror row (origin_op = op_id) through the seeder — the SAME landing
    // path the daemon ingestor drives, so the outbox op goes accepted → landed identically.
    const QJsonObject args = QJsonDocument::fromJson(op.payload).object();
    const QString transport = args.value(QStringLiteral("transport")).toString();
    const QString conv = args.value(QStringLiteral("conv")).toString();
    mirror::ChatMessage m;
    m.transport = transport;
    m.conv = conv;
    m.cursor = nextChatCursor(transport, conv);
    m.id = QStringLiteral("mock-echo-") + op.opId;
    m.author = QStringLiteral("Me");
    m.text = args.value(QStringLiteral("message")).toString();
    m.timestamp = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
    m.origin_op = op.opId;
    m_seeder.appendMessage(m, op.opId);
}

void MockScenarioHost::applySessionMetaEcho(const mirror::PendingOp& op) {
    // The mock twin of the node's authoritative read-path echo for a session-meta patch (§6.6 /
    // §10.3): apply the acked patch fields to the CURRENT mirror row through the seeder, stamped
    // origin_op = op_id — the same landing path the daemon's SessionMetaChanged → SessionGet
    // apply drives, so the outbox op goes accepted → landed identically and the roster
    // re-projects event-driven (never an optimistic local write).
    const QJsonObject args = QJsonDocument::fromJson(op.payload).object();
    const QString sessionId = args.value(QStringLiteral("session")).toString();
    if (sessionId.isEmpty()) {
        return;
    }
    const mirror::Session* existing =
        m_svc->store().snapshot().sessions.find(mirror::SessionKey{sessionId});
    if (existing == nullptr) {
        return; // an unknown session acks Ok but echoes nothing (node parity: nothing to patch)
    }
    mirror::Session patched = *existing;
    if (args.contains(QStringLiteral("pinned"))) {
        patched.pinned = args.value(QStringLiteral("pinned")).toBool();
    }
    if (args.contains(QStringLiteral("archived"))) {
        patched.archived = args.value(QStringLiteral("archived")).toBool();
    }
    if (args.contains(QStringLiteral("title"))) {
        patched.title = args.value(QStringLiteral("title")).toString();
    }
    m_seeder.upsertSession(patched, op.opId);
}

quint64 MockScenarioHost::nextChatCursor(const QString& transport, const QString& conv) const {
    const mirror::ChatMessageScope scope{transport, conv};
    const auto& chat = m_svc->store().snapshot().chat;
    if (chat.count(scope) == 0U) {
        return 1;
    }
    return chat[scope].meta.newest_cursor + 1;
}

} // namespace daemonapp::daemon
