#include "turn_controller.h"

#include <QSignalSpy>
#include <QVariantList>
#include <QVariantMap>
#include <QtTest>

// Exercises the shared TurnController FSM (the C++ port of TurnSimulator.qml):
// a started turn runs to completion via its internal timers, emits the scripted
// daemon events, and settles in a terminal state - with the prompt-driven "fail"
// branch ending in error. These guard the behavior the GUI and TUI both rely on.
class TestTurnController : public QObject {
    Q_OBJECT

private:
    // Run a turn to completion (or time out), collecting the flattened event
    // stream. Returns the total number of events emitted across all steps.
    static int runToCompletion(TurnController& turn, const QString& prompt,
                               QList<QVariantMap>& events)
    {
        QObject::connect(&turn, &TurnController::eventsEmitted, &turn,
                         [&events](const QVariantList& batch) {
                             for (const QVariant& v : batch) {
                                 events.append(v.toMap());
                             }
                         });
        QSignalSpy finished(&turn, &TurnController::turnFinished);
        turn.start(prompt);
        // The full script (incl. the deliberate > stall gap) runs in ~7s; allow
        // generous headroom so the test is not timing-flaky under load.
        const bool ok = finished.wait(15000);
        Q_ASSERT(ok);
        return static_cast<int>(events.size());
    }

private slots:
    // A normal turn streams reasoning -> tools -> text -> flush and settles idle.
    void normalTurnCompletesIdle()
    {
        TurnController turn;
        QCOMPARE(turn.turnState(), QStringLiteral("idle"));
        QVERIFY(!turn.active());

        QList<QVariantMap> events;
        const int count = runToCompletion(turn, QStringLiteral("hello there"), events);

        // 2 reasoningDelta + reasoningDone + (toolStarted/toolFinished) x2 + 3 text + flush.
        QCOMPARE(count, 11);
        QVERIFY(!turn.active());
        QCOMPARE(turn.turnState(), QStringLiteral("idle"));
        QVERIFY(turn.errorText().isEmpty());

        // The last emitted event is the turn-end flush.
        QCOMPARE(events.last().value(QStringLiteral("type")).toString(),
                 QStringLiteral("flush"));
        // At least one streamed text delta made it through.
        int textCount = 0;
        for (const QVariantMap& e : events) {
            if (e.value(QStringLiteral("type")).toString() == QStringLiteral("text")) {
                ++textCount;
            }
        }
        QCOMPARE(textCount, 3);
    }

    // A prompt containing "fail" drives the error branch: the terminal tool fails
    // and the turn settles in the "error" state with a surfaced error message.
    void failPromptEndsInError()
    {
        TurnController turn;
        QList<QVariantMap> events;
        const int count = runToCompletion(turn, QStringLiteral("make the build fail"), events);

        // 2 reasoningDelta + reasoningDone + toolStarted + toolFinished(error) + text + flush.
        QCOMPARE(count, 7);
        QVERIFY(!turn.active());
        QCOMPARE(turn.turnState(), QStringLiteral("error"));
        QVERIFY(!turn.errorText().isEmpty());

        // The failing tool reports error status.
        bool sawError = false;
        for (const QVariantMap& e : events) {
            if (e.value(QStringLiteral("type")).toString() == QStringLiteral("toolFinished")
                && e.value(QStringLiteral("status")).toString() == QStringLiteral("error")) {
                sawError = true;
            }
        }
        QVERIFY(sawError);
    }

    // cancel() stops an in-flight turn and resets to idle without emitting finish.
    void cancelResetsToIdle()
    {
        TurnController turn;
        turn.start(QStringLiteral("anything"));
        QVERIFY(turn.active());
        turn.cancel();
        QVERIFY(!turn.active());
        QCOMPARE(turn.turnState(), QStringLiteral("idle"));
    }
};

QTEST_GUILESS_MAIN(TestTurnController)
#include "tst_turn_controller.moc"
