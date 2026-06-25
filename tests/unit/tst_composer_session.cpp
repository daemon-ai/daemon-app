#include "composer_session_controller.h"

#include "models/mock_model_catalog.h"
#include "uimodels/variant_list_model.h"

#include "cache_test_support.h"

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
    void init() { resetMockCache(); }

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

    // With an injected IModelCatalog the model list is the installed models and
    // the current selection is the catalog's active model: selecting in the
    // composer activates in the catalog (single source of truth), and a catalog
    // activation flows back into the composer's current selection.
    void modelSourceUnifiesWithCatalog()
    {
        models::MockModelCatalog catalog;
        ComposerSessionController c;
        c.setModelSource(&catalog);

        // The composer list now mirrors the catalog's installed models.
        auto* installed = qobject_cast<uimodels::VariantListModel*>(catalog.installed());
        QVERIFY(installed != nullptr);
        QCOMPARE(c.models().size(), installed->count());

        // The current selection tracks the catalog's active id.
        const QString activeId = catalog.currentModelId();
        QVERIFY(!activeId.isEmpty());
        const int idx = c.currentModelIndex();
        QVERIFY(idx >= 0);
        QCOMPARE(c.modelCatalog().at(idx).toMap().value(QStringLiteral("id")).toString(), activeId);

        // Install a second model so there is something else to switch to.
        const QString secondId = QStringLiteral("mistral-7b-instruct");
        catalog.download(secondId);
        QTRY_VERIFY(installed->indexOfId(secondId) >= 0);

        // Selecting it in the composer activates it in the shared catalog.
        int target = -1;
        const QVariantList entries = c.modelCatalog();
        for (int i = 0; i < entries.size(); ++i) {
            if (entries.at(i).toMap().value(QStringLiteral("id")).toString() == secondId) {
                target = i;
            }
        }
        QVERIFY(target >= 0);
        c.selectModel(target);
        QCOMPARE(catalog.currentModelId(), secondId);
        QCOMPARE(c.currentModelIndex(), target);
    }

    // --- Reverse incremental history search (Ctrl+R) -----------------------------
    // Seeds a three-entry history (oldest -> newest) for the active session.
    static void seedHistory(ComposerSessionController& c)
    {
        c.setSessionId(QStringLiteral("s-1"));
        const char* entries[] = { "alpha one", "beta two", "alpha three" };
        for (const char* e : entries) {
            c.setDraft(QString::fromLatin1(e));
            c.submit(); // idle send: pushes history + clears the draft
        }
    }

    // Typing narrows to the newest match; Ctrl+R steps to the next older match and
    // flags failure (keeping the last preview) when none remains. The match is
    // previewed into the draft via draftReset so both front ends show it.
    void reverseSearchNarrowsAndSteps()
    {
        ComposerSessionController c;
        seedHistory(c);

        QSignalSpy reset(&c, &ComposerSessionController::draftReset);
        c.reverseSearchStart();
        QVERIFY(c.reverseSearching());
        QVERIFY(c.reverseSearchQuery().isEmpty());

        c.reverseSearchType(QStringLiteral("alpha"));
        QCOMPARE(c.reverseSearchQuery(), QStringLiteral("alpha"));
        QVERIFY(c.reverseSearchFound());
        QCOMPARE(c.draft(), QStringLiteral("alpha three")); // newest match
        QVERIFY(reset.count() >= 1);                        // preview emitted

        c.reverseSearchNext();
        QVERIFY(c.reverseSearchFound());
        QCOMPARE(c.draft(), QStringLiteral("alpha one")); // next older match

        c.reverseSearchNext();
        QVERIFY(!c.reverseSearchFound());                 // no older match
        QCOMPARE(c.draft(), QStringLiteral("alpha one")); // preview unchanged
    }

    // Backspacing widens the query; emptying it reverts the preview to the draft
    // that was in the field when the search began.
    void reverseSearchBackspaceWidensAndRestores()
    {
        ComposerSessionController c;
        seedHistory(c);
        c.setDraft(QStringLiteral("work in progress"));

        c.reverseSearchStart();
        c.reverseSearchType(QStringLiteral("beta"));
        QCOMPARE(c.draft(), QStringLiteral("beta two"));

        c.reverseSearchBackspace(); // "bet" still matches "beta two"
        QCOMPARE(c.reverseSearchQuery(), QStringLiteral("bet"));
        QCOMPARE(c.draft(), QStringLiteral("beta two"));

        c.reverseSearchType(QStringLiteral("z")); // "betz": no match
        QVERIFY(!c.reverseSearchFound());

        // Backspace down to empty: preview reverts to the pre-search draft.
        c.reverseSearchBackspace(); // "bet"
        c.reverseSearchBackspace(); // "be"
        c.reverseSearchBackspace(); // "b"
        c.reverseSearchBackspace(); // ""
        QVERIFY(c.reverseSearchQuery().isEmpty());
        QCOMPARE(c.draft(), QStringLiteral("work in progress"));
    }

    // Accept keeps the previewed match as the editable draft and never submits.
    void reverseSearchAcceptKeepsDraftWithoutSubmit()
    {
        ComposerSessionController c;
        seedHistory(c);
        QSignalSpy submitSpy(&c, &ComposerSessionController::submitted);

        c.reverseSearchStart();
        c.reverseSearchType(QStringLiteral("beta"));
        QCOMPARE(c.draft(), QStringLiteral("beta two"));

        c.reverseSearchAccept();
        QVERIFY(!c.reverseSearching());
        QCOMPARE(c.draft(), QStringLiteral("beta two"));
        QCOMPARE(submitSpy.count(), 0);
    }

    // Cancel restores the pre-search draft and exits the mode.
    void reverseSearchCancelRestoresDraft()
    {
        ComposerSessionController c;
        seedHistory(c);
        c.setDraft(QStringLiteral("keep me"));

        c.reverseSearchStart();
        c.reverseSearchType(QStringLiteral("alpha"));
        QVERIFY(c.draft() != QStringLiteral("keep me"));

        c.reverseSearchCancel();
        QVERIFY(!c.reverseSearching());
        QCOMPARE(c.draft(), QStringLiteral("keep me"));
    }

    // A session swap (or any external draft edit) drops out of an active search.
    void reverseSearchResetsOnSessionSwap()
    {
        ComposerSessionController c;
        seedHistory(c);

        c.reverseSearchStart();
        c.reverseSearchType(QStringLiteral("alpha"));
        QVERIFY(c.reverseSearching());

        c.setSessionId(QStringLiteral("s-2"));
        QVERIFY(!c.reverseSearching());
        QVERIFY(c.reverseSearchQuery().isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestComposerSession)
#include "tst_composer_session.moc"
