#include "conversation_controller.h"
#include "conversation_orchestrator.h"
#include "todo_list_model.h"
#include "turn_controller.h"

#include "persistence/in_memory_conversation_store.h"

#include <QSignalSpy>
#include <QtTest>

// Exercises the shared submit pipeline (ConversationOrchestrator): submit starts
// the turn and populates the status-stack todos; the todos clear a beat after the
// turn settles; cancel stops the turn; slash "/new" is handled here while other
// commands surface via commandRequested. Both front ends drive this identically.
class TestConversationOrchestrator : public QObject {
    Q_OBJECT

private slots:
    void submitStartsTurnAndPopulatesTodos()
    {
        ConversationOrchestrator orch;
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

    void submitAppendsUserTextToConversation()
    {
        persistence::InMemoryConversationStore store;
        ConversationController controller;
        controller.setStore(&store);
        const int id = controller.createConversation(QString());
        QVERIFY(id >= 0);

        ConversationOrchestrator orch;
        orch.setConversation(&controller);
        orch.submit(QStringLiteral("hello there"), QStringLiteral("@file:README.md"));

        const QString content = controller.content();
        QVERIFY(content.contains(QStringLiteral("hello there")));
        QVERIFY(content.contains(QStringLiteral("@file:README.md")));

        orch.cancel();
    }

    void invokeCommandNewCreatesConversation()
    {
        persistence::InMemoryConversationStore store;
        ConversationController controller;
        controller.setStore(&store);
        QVERIFY(!controller.hasConversation());

        ConversationOrchestrator orch;
        orch.setConversation(&controller);
        QSignalSpy requestedSpy(&orch, &ConversationOrchestrator::commandRequested);

        orch.invokeCommand(QStringLiteral("new"));

        QVERIFY(controller.hasConversation());
        QCOMPARE(requestedSpy.count(), 0); // "new" is handled, not surfaced
    }

    void invokeCommandOtherEmitsRequested()
    {
        ConversationOrchestrator orch;
        QSignalSpy requestedSpy(&orch, &ConversationOrchestrator::commandRequested);

        orch.invokeCommand(QStringLiteral("theme"));

        QCOMPARE(requestedSpy.count(), 1);
        QCOMPARE(requestedSpy.last().at(0).toString(), QStringLiteral("theme"));
    }

    void todosClearAfterTurnSettles()
    {
        ConversationOrchestrator orch;
        QSignalSpy finished(orch.turn(), &TurnController::turnFinished);

        orch.submit(QStringLiteral("hello"), QString());
        QCOMPARE(orch.todos()->count(), 3);

        // The scripted turn runs to completion on its internal timers, then the
        // orchestrator clears the todos ~1.5s later.
        QVERIFY(finished.wait(15000));
        QTRY_COMPARE_WITH_TIMEOUT(orch.todos()->count(), 0, 4000);
    }
};

QTEST_GUILESS_MAIN(TestConversationOrchestrator)
#include "tst_conversation_orchestrator.moc"
