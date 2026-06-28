#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <QVariantList>

// The turn-execution seam both front ends bind to. A turn produces a stream of daemon-shaped event
// maps (consumed by EditorController::ingestEvents in the GUI / TranscriptIngest::ingestAll in the
// TUI) and exposes coarse turn state for the composer chrome.
//
// Two implementations:
//   - TurnController  - the canned simulator (mock mode / pre-backend UI coverage)
//   - DaemonTurnEngine - the real engine: Submit{StartTurn} then a Subscribe loop applying the
//                        node's AgentEvent stream (CHA-1 / CHA-2)
//
// Consumers only read active/turnState/elapsedMs/errorText and connect to eventsEmitted /
// turnStarted / turnFinished / hostRequested / awaitingInput, so the two are interchangeable.
//
// turnState: "idle" | "thinking" | "running" | "streaming" | "stalled" | "error"
class ITurnEngine : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("ITurnEngine is an abstract turn-engine seam")
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(QString turnState READ turnState NOTIFY turnStateChanged)
    Q_PROPERTY(int elapsedMs READ elapsedMs NOTIFY elapsedMsChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)

public:
    using QObject::QObject;
    ~ITurnEngine() override = default;

    [[nodiscard]] virtual bool active() const = 0;
    [[nodiscard]] virtual QString turnState() const = 0;
    [[nodiscard]] virtual int elapsedMs() const = 0;
    [[nodiscard]] virtual QString errorText() const = 0;
    [[nodiscard]] virtual bool paused() const { return false; }

    // Bind the engine to a session id before starting a turn. The simulator ignores it; the daemon
    // engine needs it for Submit/Subscribe. Front ends call this when the active session changes.
    virtual void setSessionId(const QString& /*sessionId*/) {}
    // Bind the turn to a profile (PRO-5): a non-empty profile makes Submit carry it, so the new
    // session runs under that agent's model/credentials. Empty = the node's active profile.
    virtual void setProfile(const QString& /*profile*/) {}

    Q_INVOKABLE virtual void start(const QString& prompt) = 0;
    Q_INVOKABLE virtual void cancel() = 0;
    Q_INVOKABLE virtual void resume() = 0;

    // HITL gate resolution (CHA-4 / CHA-5). `requestId` is the gate's call id (the stream
    // HostRequest.request_id rendered as a string). The mock maps these onto its single gate;
    // the daemon engine sends the matching Respond / ApprovalDecide and resumes the Subscribe loop.
    Q_INVOKABLE virtual void respondApproval(const QString& requestId, bool allow) {
        Q_UNUSED(requestId)
        Q_UNUSED(allow)
        resume();
    }
    Q_INVOKABLE virtual void respondInput(const QString& requestId, const QString& text) {
        Q_UNUSED(requestId)
        Q_UNUSED(text)
        resume();
    }
    Q_INVOKABLE virtual void respondChoice(const QString& requestId, int index) {
        Q_UNUSED(requestId)
        Q_UNUSED(index)
        resume();
    }

    // CHA-6: interrupt a running turn (composer Stop), or steer it with extra text mid-flight.
    Q_INVOKABLE virtual void interrupt(const QString& reason) {
        Q_UNUSED(reason)
        cancel();
    }
    Q_INVOKABLE virtual void steer(const QString& text) { Q_UNUSED(text) }

    // L3 awareness (daemon-sync-protocol §5): the SubscriptionManager calls this by name (via its
    // focus registry, so the daemon-client module stays decoupled from the Turn module) when the
    // node reports this engine's focused session advanced out of band — an idle focused engine then
    // catches up on its push subscription without polling. Default no-op (the simulator ignores
    // it).
    Q_INVOKABLE virtual void nudge() {}

signals:
    void activeChanged();
    void turnStateChanged();
    void elapsedMsChanged();
    void errorTextChanged();

    void turnStarted();
    void turnFinished();
    // One turn step's daemon event(s). GUI -> EditorController::ingestEvents; TUI ->
    // TranscriptIngest.
    void eventsEmitted(const QVariantList& events);
    // The turn paused for a masked host input. `kind` is "password" | "secret".
    void hostRequested(const QString& kind, const QString& prompt);
    // The turn reached a user gate (approval/clarify/host-input). `kind` is
    // "approval" | "password" | "secret". Front ends surface it out-of-band.
    void awaitingInput(const QString& kind);
};
