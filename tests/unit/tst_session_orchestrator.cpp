#include "persistence/in_memory_session_store.h"
#include "session_controller.h"
#include "session_orchestrator.h"
#include "todo_list_model.h"
#include "turn_controller.h"

#include <QSignalSpy>
#include <QtTest>

// Exercises the shared submit pipeline (SessionOrchestrator): submit starts
// the turn and populates the status-stack todos; the todos clear a beat after the
// turn settles; cancel stops the turn; slash "/new" is handled here while other
// commands surface via commandRequested. Both front ends drive this identically.
class TestSessionOrchestrator : public QObject {
    Q_OBJECT

private slots:
    void submitStartsTurnAndPopulatesTodos() {
        SessionOrchestrator orch;
        QVERIFY(!orch.busy());
        QCOMPARE(orch.todos()->count(), 0);

        orch.submit(QStringLiteral("hi"), QString());

        QVERIFY(orch.busy());
        QVERIFY(orch.turn()->active());
        QCOMPARE(orch.todos()->count(), 3);

        orch.cancel();
        QVERIFY(!orch.busy());
        QVERIFY(!orch.turn()->active());
    }

    void submitAppendsUserTextToSession() {
        persistence::InMemorySessionStore store;
        SessionController controller;
        controller.setStore(&store);
        const QString id = controller.createSession(QString());
        QVERIFY(!id.isEmpty());

        SessionOrchestrator orch;
        orch.setSession(&controller);
        orch.submit(QStringLiteral("hello there"), QStringLiteral("@file:README.md"));

        const QString content = controller.content();
        QVERIFY(content.contains(QStringLiteral("hello there")));
        QVERIFY(content.contains(QStringLiteral("@file:README.md")));

        orch.cancel();
    }

    void invokeCommandNewCreatesSession() {
        persistence::InMemorySessionStore store;
        SessionController controller;
        controller.setStore(&store);
        QVERIFY(!controller.hasSession());

        SessionOrchestrator orch;
        orch.setSession(&controller);
        QSignalSpy requestedSpy(&orch, &SessionOrchestrator::commandRequested);

        orch.invokeCommand(QStringLiteral("new"));

        QVERIFY(controller.hasSession());
        QCOMPARE(requestedSpy.count(), 0); // "new" is handled, not surfaced
    }

    void invokeCommandOtherEmitsRequested() {
        SessionOrchestrator orch;
        QSignalSpy requestedSpy(&orch, &SessionOrchestrator::commandRequested);

        orch.invokeCommand(QStringLiteral("theme"));

        QCOMPARE(requestedSpy.count(), 1);
        QCOMPARE(requestedSpy.last().at(0).toString(), QStringLiteral("theme"));
    }

    void todosClearAfterTurnSettles() {
        SessionOrchestrator orch;
        QSignalSpy finished(orch.turn(), &TurnController::turnFinished);

        orch.submit(QStringLiteral("hello"), QString());
        QCOMPARE(orch.todos()->count(), 3);

        // The scripted turn runs to completion on its internal timers, then the
        // orchestrator clears the todos ~1.5s later.
        QVERIFY(finished.wait(15000));
        QTRY_COMPARE_WITH_TIMEOUT(orch.todos()->count(), 0, 4000);
    }
};

QTEST_GUILESS_MAIN(TestSessionOrchestrator)
#include "tst_session_orchestrator.moc"
