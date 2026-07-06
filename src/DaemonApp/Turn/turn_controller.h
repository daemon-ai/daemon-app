// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "i_turn_engine.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>
#include <QVariantList>
#include <QVariantMap>

// Demo/simulator runtime for pre-backend UI coverage - the C++ port of
// TurnSimulator.qml. Given a user prompt it plays a canned assistant turn
// (reasoning -> tool running/done -> streamed text -> flush) by emitting daemon-
// shaped event maps on a re-armed timer. The real DaemonTurnEngine emits the same
// event shapes; consumers (bound to the ITurnEngine seam) are agnostic.
//
// GUI-free (Qt6::Qml for QML_ELEMENT + Qt6::Core for QTimer/QVariant): the GUI
// feeds `eventsEmitted` into EditorController::ingestEvents; the TUI renders the
// text deltas directly.
//
// turnState: "idle" | "thinking" | "running" | "streaming" | "stalled" | "error"
class TurnController : public ITurnEngine {
    Q_OBJECT
    QML_ELEMENT

public:
    explicit TurnController(QObject* parent = nullptr);

    [[nodiscard]] bool active() const override { return m_active; }
    [[nodiscard]] QString turnState() const override { return m_turnState; }
    [[nodiscard]] int elapsedMs() const override { return m_elapsedMs; }
    [[nodiscard]] QString errorText() const override { return m_errorText; }
    [[nodiscard]] bool paused() const override { return m_paused; }
    // The simulator seeds canned status-stack todos per turn (pre-backend UI coverage).
    [[nodiscard]] bool simulatesStatusFeeds() const override { return true; }

    void start(const QString& prompt) override;
    void cancel() override;
    // Resume a turn paused at an approval gate (a gated step pauses scheduling
    // until the inline approval answer drives this). No-op unless paused.
    void resume() override;
    // Answer the approval gate: the simulator emits its OWN scripted toolFinished
    // for the gated tool (ok/stdout on allow, error on deny) and a matching text,
    // then resumes. Front ends never fabricate the result — they render only this
    // (mock) or the node's real ToolFinished stream event (daemon). No-op unless
    // paused at the gate. `allowPermanent` (wire v28) is accepted for seam parity;
    // the simulator has no persistent policy, so it plays out the same either way.
    void respondApproval(const QString& requestId, bool allow,
                         bool allowPermanent = false) override;

private:
    struct Step {
        int delayMs = 0;
        QVariantMap event;
        bool error = false;
        QString errorText;
        // Pause the turn after emitting this step (an awaiting-approval tool);
        // resume() continues scheduling once the inline answer is given.
        bool gate = false;
        // When set ("password"/"secret"), emitting this step also raises a host
        // input request (and implies a gate): the front end shows a masked prompt
        // and resume()s when answered.
        QString hostRequestKind;
        QString hostRequestPrompt;
    };

    void setActive(bool active);
    void setTurnState(const QString& state);
    void setElapsedMs(int ms);
    void setErrorText(const QString& text);

    [[nodiscard]] static QString stateForEvent(const QString& type);
    void scheduleNext();
    void runStep();
    void complete();
    void armStall();
    [[nodiscard]] static QList<Step> buildScript(const QString& prompt);

    // Stall threshold: if no scripted step lands within this window mid-turn, the
    // chrome shows "still thinking". One script gap is longer than this on
    // purpose so the indicator is exercised.
    static constexpr int kStallMs = 2000;

    bool m_active = false;
    bool m_paused = false; // waiting at an approval gate
    QString m_turnState = QStringLiteral("idle");
    int m_elapsedMs = 0;
    QString m_errorText;

    QList<Step> m_steps;
    int m_index = 0;
    qint64 m_startedAt = 0;
    QString m_resumeState = QStringLiteral("thinking");

    QTimer m_stepTimer;
    QTimer m_stallTimer;
    QTimer m_elapsedTimer;
};
