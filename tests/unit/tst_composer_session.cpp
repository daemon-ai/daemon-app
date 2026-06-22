#include "composer_session_controller.h"

#include <QSignalSpy>
#include <QtTest>

// Exercises the composer-session logic extracted from QML in the "finish the
// deferred seams" pass: the primary-action rule (send/queue/stop + enablement,
// the C++ home of ComposerControls.qml's `mode`/`actionEnabled`) and the model
// selector (the C++ home of ModelPill.qml's list/currentIndex). Both front ends
// drive these.
class TestComposerSession : public QObject {
    Q_OBJECT

private slots:
    // primaryAction mirrors ComposerControls.qml: idle -> "send", busy with a
    // payload -> "queue", busy with no payload -> "stop".
    void primaryActionTracksBusyAndPayload()
    {
        ComposerSessionController c;

        // Idle, empty: send but not actionable (needs a payload).
        QCOMPARE(c.primaryAction(), QStringLiteral("send"));
        QVERIFY(!c.primaryActionEnabled());

        // Idle with a draft: send, actionable.
        c.setDraft(QStringLiteral("hello"));
        QCOMPARE(c.primaryAction(), QStringLiteral("send"));
        QVERIFY(c.primaryActionEnabled());

        // Busy with a draft: queue (and always actionable while enabled).
        c.setBusy(true);
        QCOMPARE(c.primaryAction(), QStringLiteral("queue"));
        QVERIFY(c.primaryActionEnabled());

        // Busy, empty draft: stop.
        c.setDraft(QString());
        QCOMPARE(c.primaryAction(), QStringLiteral("stop"));
        QVERIFY(c.primaryActionEnabled());
    }

    // primaryActionEnabled folds in `enabled` and re-notifies via derivedChanged.
    void primaryActionEnabledFoldsEnabled()
    {
        ComposerSessionController c;
        c.setBusy(true); // stop mode -> normally actionable
        QVERIFY(c.primaryActionEnabled());

        QSignalSpy spy(&c, &ComposerSessionController::derivedChanged);
        c.setEnabled(false);
        QVERIFY(!c.primaryActionEnabled());
        QVERIFY(spy.count() >= 1);
    }

    // Model selector exposes the canned list with a default selection, and
    // selectModel clamps + notifies via currentModelChanged.
    void modelSelectionClampsAndNotifies()
    {
        ComposerSessionController c;
        QVERIFY(c.models().size() >= 2);
        QCOMPARE(c.currentModelIndex(), 0);
        QCOMPARE(c.currentModel(), c.models().first());

        QSignalSpy spy(&c, &ComposerSessionController::currentModelChanged);
        c.selectModel(2);
        QCOMPARE(c.currentModelIndex(), 2);
        QCOMPARE(c.currentModel(), c.models().at(2));
        QCOMPARE(spy.count(), 1);

        // Out-of-range selections are ignored (no change, no extra signal).
        c.selectModel(-1);
        c.selectModel(c.models().size());
        QCOMPARE(c.currentModelIndex(), 2);
        QCOMPARE(spy.count(), 1);
    }
};

QTEST_GUILESS_MAIN(TestComposerSession)
#include "tst_composer_session.moc"
