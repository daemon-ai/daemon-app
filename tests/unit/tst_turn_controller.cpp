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

        // reasoning + 2 tools + 3 text + flush, interleaved with the live
        // usage/context/rate-limit deltas; assert structure, not an exact total.
        QVERIFY(count >= 11);
        QVERIFY(!turn.active());
        QCOMPARE(turn.turnState(), QStringLiteral("idle"));
        QVERIFY(turn.errorText().isEmpty());

        // The last emitted event is the turn-end flush.
        QCOMPARE(events.last().value(QStringLiteral("type")).toString(),
                 QStringLiteral("flush"));
        // The scripted text deltas and live status events both make it through.
        int textCount = 0;
        bool sawUsage = false;
        bool sawContext = false;
        for (const QVariantMap& e : events) {
            const QString type = e.value(QStringLiteral("type")).toString();
            if (type == QStringLiteral("text")) {
                ++textCount;
            } else if (type == QStringLiteral("usage")) {
                sawUsage = true;
            } else if (type == QStringLiteral("context")) {
                sawContext = true;
            }
        }
        QCOMPARE(textCount, 3);
        QVERIFY(sawUsage);
        QVERIFY(sawContext);
    }

    // A prompt containing "fail" drives the error branch: the terminal tool fails
    // and the turn settles in the "error" state with a surfaced error message.
    void failPromptEndsInError()
    {
        TurnController turn;
        QList<QVariantMap> events;
        const int count = runToCompletion(turn, QStringLiteral("make the build fail"), events);

        // reasoning + toolStarted + toolFinished(error) + text + flush, plus the
        // live status deltas; assert structure, not an exact total.
        QVERIFY(count >= 7);
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

    // A prompt mentioning "sudo" pauses the turn at a host-input gate: the
    // controller emits hostRequested + awaitingInput and parks (paused, no finish)
    // until resume() drives it to completion.
    void sudoPromptGatesForHostInput()
    {
        TurnController turn;
        QSignalSpy hostSpy(&turn, &TurnController::hostRequested);
        QSignalSpy awaitSpy(&turn, &TurnController::awaitingInput);
        QSignalSpy finished(&turn, &TurnController::turnFinished);

        turn.start(QStringLiteral("please sudo make install"));
        // The gate lands within the first few hundred ms; wait for the request.
        QVERIFY(hostSpy.wait(5000));
        QCOMPARE(hostSpy.count(), 1);
        QCOMPARE(hostSpy.at(0).at(0).toString(), QStringLiteral("password"));
        QCOMPARE(awaitSpy.count(), 1);
        QCOMPARE(awaitSpy.at(0).at(0).toString(), QStringLiteral("password"));

        // Parked at the gate: still active, not finished, reports paused.
        QVERIFY(turn.active());
        QVERIFY(turn.paused());
        QCOMPARE(finished.count(), 0);

        // Answering the prompt resumes the turn to completion.
        turn.resume();
        QVERIFY(finished.wait(5000));
        QVERIFY(!turn.active());
        QCOMPARE(turn.turnState(), QStringLiteral("idle"));
    }

    // A prompt mentioning a "secret" / "api key" gates with the secret kind.
    void secretPromptGatesWithSecretKind()
    {
        TurnController turn;
        QSignalSpy hostSpy(&turn, &TurnController::hostRequested);
        turn.start(QStringLiteral("rotate the api key"));
        QVERIFY(hostSpy.wait(5000));
        QCOMPARE(hostSpy.at(0).at(0).toString(), QStringLiteral("secret"));
        turn.cancel(); // abandoning the gate resets cleanly
        QVERIFY(!turn.active());
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
