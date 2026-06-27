#pragma once

#include "i_turn_engine.h"

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QTimer>

namespace daemonapp::daemon {
class NodeApiClient;
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
    explicit DaemonTurnEngine(daemonapp::daemon::NodeApiClient* client, QObject* parent = nullptr);

    [[nodiscard]] bool active() const override { return m_active; }
    [[nodiscard]] QString turnState() const override { return m_turnState; }
    [[nodiscard]] int elapsedMs() const override { return m_elapsedMs; }
    [[nodiscard]] QString errorText() const override { return m_errorText; }

    void setSessionId(const QString& sessionId) override { m_sessionId = sessionId; }
    void setProfile(const QString& profile) override { m_profile = profile; }

    void start(const QString& prompt) override;
    void cancel() override;
    void resume() override; // bare resume: continues the Subscribe loop if parked

    // HITL gate resolution (CHA-4 / CHA-5): send the matching Respond, then resume the loop.
    void respondApproval(const QString& requestId, bool allow) override;
    void respondInput(const QString& requestId, const QString& text) override;
    void respondChoice(const QString& requestId, int index) override;
    // CHA-6: interrupt / steer a running turn via Submit{Interrupt/Steer}.
    void interrupt(const QString& reason) override;
    void steer(const QString& text) override;

private:
    void onResponse(const QString& correlationId, const QByteArray& responseCbor);
    void onFailure(const QString& correlationId, const QString& message);
    void pollSubscribe();
    void finishTurn(const QString& errorText = QString());
    // Park the turn on a decoded HostRequest: surface the gate to the transcript and stop polling
    // until a respond* arrives.
    void parkOnRequest(const daemonapp::daemon::DecodedLogEntry& entry);
    // Send an encoded Respond/ApprovalDecide for the parked gate, then resume the Subscribe loop.
    void sendRespond(const QByteArray& cbor);

    void setActive(bool active);
    void setTurnState(const QString& state);
    void setElapsedMs(int ms);
    void setErrorText(const QString& text);

    [[nodiscard]] QString submitCorrelation() const;
    [[nodiscard]] QString subscribeCorrelation() const;
    [[nodiscard]] QString respondCorrelation() const;

    daemonapp::daemon::NodeApiClient* m_client = nullptr;
    QString m_sessionId;
    QString m_profile;    // optional profile binding for Submit (PRO-5)
    quint64 m_cursor = 0; // next_seq watermark for Subscribe
    quint32 m_requestId = 1;
    bool m_active = false;
    bool m_finished = false;
    // HITL: the turn is parked on a HostRequest waiting for a respond* call.
    bool m_parked = false;
    quint32 m_pendingRequestId = 0;
    QString m_pendingKind; // "Approval" | "Input" | "Choice"
    QStringList m_pendingOptions;
    QString m_turnState = QStringLiteral("idle");
    int m_elapsedMs = 0;
    QString m_errorText;

    QTimer m_pollTimer;     // re-arms the Subscribe poll
    QTimer m_elapsedTimer;  // 1s tick for elapsedMs
    QTimer m_deadlineTimer; // hard cap so a stuck turn cannot poll forever
    QTimer m_resumeTimer;   // backoff re-Subscribe after a mid-turn drop (turn resume)
    qint64 m_startedAt = 0;
    int m_resumeBackoffMs = kResumeMinMs; // grows per consecutive subscribe failure
    int m_resumeAttempts = 0;             // consecutive subscribe failures since the last good page

    static constexpr int kPollIntervalMs = 120;
    static constexpr int kDeadlineMs = 180000; // 3 min safety cap
    static constexpr quint32 kSubscribeMax = 256;
    // Turn resume: a mid-turn Subscribe failure (the socket dropped) is transient - the shared
    // transport reconnects lazily, so retry from m_cursor with backoff instead of killing the turn.
    static constexpr int kResumeMinMs = 250;
    static constexpr int kResumeMaxMs = 4000;
    static constexpr int kResumeMaxAttempts = 12; // give up -> finish with error after this many
};
