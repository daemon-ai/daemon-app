// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "completion_model.h"
#include "composer_session_controller.h"

#include <QSignalSpy>
#include <QtTest>

// Exercises the shared completion FSM extracted from Composer.qml into
// ComposerSessionController (+ CompletionModel): the slash/@ data + search, the
// regex trigger detection, list navigation, and the accept transforms (insert a
// value vs. strip the token and run a command). Both front ends drive this path.
class TestCompletion : public QObject {
    Q_OBJECT

private slots:
    // --- CompletionModel::search (ported CompletionProvider data) -----------
    void searchSlashEmptyReturnsPool() {
        const auto items = CompletionModel::search(QStringLiteral("slash"), QString());
        QVERIFY(items.size() >= 4);
        QCOMPARE(items.first().label, QStringLiteral("/new"));
    }

    void searchSlashFiltersBySubstring() {
        const auto items = CompletionModel::search(QStringLiteral("slash"), QStringLiteral("the"));
        QCOMPARE(items.size(), 1);
        QCOMPARE(items.first().label, QStringLiteral("/theme"));
        QCOMPARE(items.first().action, QStringLiteral("theme"));
    }

    void searchMentionPool() {
        const auto items =
            CompletionModel::search(QStringLiteral("mention"), QStringLiteral("read"));
        QCOMPARE(items.size(), 1);
        QCOMPARE(items.first().label, QStringLiteral("README.md"));
        QCOMPARE(items.first().action, QStringLiteral("insert"));
    }

    // --- Trigger detection --------------------------------------------------
    void slashTriggerActivatesAtLineStart() {
        ComposerSessionController c;
        QSignalSpy activeSpy(&c, &ComposerSessionController::completionActiveChanged);

        c.refreshTrigger(QStringLiteral("/the"), 4);
        QVERIFY(c.completionActive());
        QCOMPARE(c.completionKind(), QStringLiteral("slash"));
        QCOMPARE(c.completionItems()->count(), 1);
        QVERIFY(activeSpy.count() >= 1);
    }

    void mentionTriggerActivatesAfterSpace() {
        ComposerSessionController c;
        c.refreshTrigger(QStringLiteral("hello @read"), 11);
        QVERIFY(c.completionActive());
        QCOMPARE(c.completionKind(), QStringLiteral("mention"));
        QCOMPARE(c.completionItems()->count(), 1);
    }

    void noTriggerWhenNoToken() {
        ComposerSessionController c;
        c.refreshTrigger(QStringLiteral("just text"), 9);
        QVERIFY(!c.completionActive());
    }

    void unknownQueryClosesTrigger() {
        ComposerSessionController c;
        c.refreshTrigger(QStringLiteral("/the"), 4);
        QVERIFY(c.completionActive());
        // A query that matches nothing dismisses the popover.
        c.refreshTrigger(QStringLiteral("/zzzz"), 5);
        QVERIFY(!c.completionActive());
    }

    // --- Navigation ---------------------------------------------------------
    void moveActiveWrapsAround() {
        ComposerSessionController c;
        c.refreshTrigger(QStringLiteral("/"), 1); // whole slash pool
        const int n = c.completionItems()->count();
        QVERIFY(n >= 2);
        QCOMPARE(c.completionActiveIndex(), 0);
        c.moveActive(-1);
        QCOMPARE(c.completionActiveIndex(), n - 1); // wraps to the end
        c.moveActive(1);
        QCOMPARE(c.completionActiveIndex(), 0);
    }

    // --- Accept transforms --------------------------------------------------
    // Accepting an "insert" item replaces the trigger token with its value and
    // requests the caret at the end of the inserted text.
    void acceptInsertReplacesTokenAndMovesCaret() {
        ComposerSessionController c;
        QSignalSpy draftSpy(&c, &ComposerSessionController::draftReset);
        QSignalSpy cursorSpy(&c, &ComposerSessionController::cursorRequested);

        // The view keeps the draft in sync with the field before refreshing.
        c.setDraft(QStringLiteral("hi @read"));
        c.refreshTrigger(QStringLiteral("hi @read"), 8);
        QVERIFY(c.completionActive());
        c.acceptActive();

        QVERIFY(!c.completionActive());
        QCOMPARE(c.draft(), QStringLiteral("hi @file:README.md "));
        QVERIFY(draftSpy.count() >= 1);
        QCOMPARE(cursorSpy.count(), 1);
        QCOMPARE(cursorSpy.last().at(0).toInt(), c.draft().length());
    }

    // Accepting a command item strips the typed token and forwards the action via
    // commandInvoked (not "clear", which is handled inline).
    void acceptCommandStripsTokenAndInvokes() {
        ComposerSessionController c;
        QSignalSpy cmdSpy(&c, &ComposerSessionController::commandInvoked);

        c.setDraft(QStringLiteral("/theme"));
        c.refreshTrigger(QStringLiteral("/theme"), 6);
        QVERIFY(c.completionActive());
        c.acceptActive();

        QVERIFY(!c.completionActive());
        QCOMPARE(c.draft(), QString()); // token stripped
        QCOMPARE(cmdSpy.count(), 1);
        QCOMPARE(cmdSpy.last().at(0).toString(), QStringLiteral("theme"));
    }

    // The "/clear" command is handled inline by clear() rather than commandInvoked.
    void acceptClearCommandClearsDraft() {
        ComposerSessionController c;
        QSignalSpy cmdSpy(&c, &ComposerSessionController::commandInvoked);

        c.setDraft(QStringLiteral("/clear"));
        c.refreshTrigger(QStringLiteral("/clear"), 6);
        QVERIFY(c.completionActive());
        c.acceptActive();

        QVERIFY(!c.completionActive());
        QCOMPARE(c.draft(), QString());
        QCOMPARE(cmdSpy.count(), 0); // "clear" is not forwarded as a command
    }
};

QTEST_GUILESS_MAIN(TestCompletion)
#include "tst_completion.moc"
