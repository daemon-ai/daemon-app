#pragma once

#include "i_turn_engine.h"

#include <QByteArray>
#include <QString>
#include <QTimer>

namespace daemonapp::daemon {
class NodeApiClient;
}

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
    explicit DaemonTurnEngine(daemonapp::daemon::NodeApiClient* client, QObject* parent = nullptr);

    [[nodiscard]] bool active() const override { return m_active; }
    [[nodiscard]] QString turnState() const override { return m_turnState; }
    [[nodiscard]] int elapsedMs() const override { return m_elapsedMs; }
    [[nodiscard]] QString errorText() const override { return m_errorText; }

    void setSessionId(const QString& sessionId) override { m_sessionId = sessionId; }
    void setProfile(const QString& profile) override { m_profile = profile; }

    void start(const QString& prompt) override;
    void cancel() override;
    void resume() override; // host-gate resume (CHA-4/5) - no-op in this slice

private:
    void onResponse(const QString& correlationId, const QByteArray& responseCbor);
    void onFailure(const QString& correlationId, const QString& message);
    void pollSubscribe();
    void finishTurn(const QString& errorText = QString());

    void setActive(bool active);
    void setTurnState(const QString& state);
    void setElapsedMs(int ms);
    void setErrorText(const QString& text);

    [[nodiscard]] QString submitCorrelation() const;
    [[nodiscard]] QString subscribeCorrelation() const;

    daemonapp::daemon::NodeApiClient* m_client = nullptr;
    QString m_sessionId;
    QString m_profile;    // optional profile binding for Submit (PRO-5)
    quint64 m_cursor = 0; // next_seq watermark for Subscribe
    quint32 m_requestId = 1;
    bool m_active = false;
    bool m_finished = false;
    QString m_turnState = QStringLiteral("idle");
    int m_elapsedMs = 0;
    QString m_errorText;

    QTimer m_pollTimer;     // re-arms the Subscribe poll
    QTimer m_elapsedTimer;  // 1s tick for elapsedMs
    QTimer m_deadlineTimer; // hard cap so a stuck turn cannot poll forever
    qint64 m_startedAt = 0;

    static constexpr int kPollIntervalMs = 120;
    static constexpr int kDeadlineMs = 180000; // 3 min safety cap
    static constexpr quint32 kSubscribeMax = 256;
};
