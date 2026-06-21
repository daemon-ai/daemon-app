#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

// Swappable, scripted stand-in for the daemon runtime - the C++ port of
// TurnSimulator.qml. Given a user prompt it plays a believable assistant turn
// (reasoning -> tool running/done -> streamed text -> flush) by emitting daemon-
// shaped event maps on a re-armed timer. A real gateway later replaces this class
// by emitting the same event shapes; consumers only read turnState/elapsedMs/
// errorText and connect to eventsEmitted.
//
// GUI-free (Qt6::Qml for QML_ELEMENT + Qt6::Core for QTimer/QVariant): the GUI
// feeds `eventsEmitted` into EditorController::ingestEvents; the TUI renders the
// text deltas directly.
//
// turnState: "idle" | "thinking" | "running" | "streaming" | "stalled" | "error"
class TurnController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(QString turnState READ turnState NOTIFY turnStateChanged)
    Q_PROPERTY(int elapsedMs READ elapsedMs NOTIFY elapsedMsChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)

public:
    explicit TurnController(QObject* parent = nullptr);

    [[nodiscard]] bool active() const { return m_active; }
    [[nodiscard]] QString turnState() const { return m_turnState; }
    [[nodiscard]] int elapsedMs() const { return m_elapsedMs; }
    [[nodiscard]] QString errorText() const { return m_errorText; }

    Q_INVOKABLE void start(const QString& prompt);
    Q_INVOKABLE void cancel();
    // Resume a turn paused at an approval gate (a gated step pauses scheduling
    // until the inline approval answer drives this). No-op unless paused.
    Q_INVOKABLE void resume();
    [[nodiscard]] bool paused() const { return m_paused; }

signals:
    void activeChanged();
    void turnStateChanged();
    void elapsedMsChanged();
    void errorTextChanged();

    void turnStarted();
    void turnFinished();
    // One scripted step's daemon event(s). The GUI forwards these to
    // EditorController::ingestEvents; the TUI inspects them for text deltas.
    void eventsEmitted(const QVariantList& events);

private:
    struct Step {
        int delayMs = 0;
        QVariantMap event;
        bool error = false;
        QString errorText;
        // Pause the turn after emitting this step (an awaiting-approval tool);
        // resume() continues scheduling once the inline answer is given.
        bool gate = false;
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
