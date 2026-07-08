// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "cache_test_support.h"
#include "composer_session_controller.h"
#include "models/mock_model_catalog.h"
#include "uimodels/variant_list_model.h"

#include <QSignalSpy>
#include <QtTest>
#include <QVariantList>
#include <QVariantMap>

// A catalog fake that surfaces a per-session FOREIGN backend + advertised model selector (Phase E),
// so the composer controller's fork can be exercised without a live daemon. Subclasses the mock so
// every other IModelCatalog method keeps working; records the last SetSessionModel + ensureDetail.
// File-scoped (not in an anonymous namespace) so moc can generate its meta-object cleanly.
class ForeignCatalogFake : public models::MockModelCatalog {
    Q_OBJECT

public:
    QString backend;             // "" | "AgentNative" | "NodeProvider"
    QVariantMap selector;        // { hasSelector, current, choices:[{id,label}] }
    QString lastSetSession;      // recorded SetSessionModel target
    QString lastSetModel;        // recorded SetSessionModel model
    QStringList ensuredSessions; // recorded ensureSessionDetail ids

    [[nodiscard]] QString sessionBackend(const QString& /*sessionId*/) const override {
        return backend;
    }
    [[nodiscard]] QVariantMap sessionModelSelector(const QString& /*sessionId*/) const override {
        return selector.isEmpty() ? QVariantMap{{QStringLiteral("hasSelector"), false}} : selector;
    }
    void ensureSessionDetail(const QString& sessionId) override { ensuredSessions << sessionId; }
    void setSessionModel(const QString& sessionId, const QString& model) override {
        lastSetSession = sessionId;
        lastSetModel = model;
    }
    // Test seam: fire the base signal as the daemon adapter would on SessionMetaChanged.
    void fireSelectorChanged(const QString& sessionId) {
        emit sessionModelSelectorChanged(sessionId);
    }
};

namespace {

QVariantMap agentNativeSelector() {
    return QVariantMap{
        {QStringLiteral("hasSelector"), true},
        {QStringLiteral("current"), QStringLiteral("sonnet")},
        {QStringLiteral("choices"),
         QVariantList{
             QVariantMap{{QStringLiteral("id"), QStringLiteral("sonnet")},
                         {QStringLiteral("label"), QStringLiteral("Claude Sonnet")}},
             QVariantMap{{QStringLiteral("id"), QStringLiteral("opus")},
                         {QStringLiteral("label"), QStringLiteral("Claude Opus")}},
         }},
    };
}

} // namespace

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
    void primaryActionTracksBusyAndPayload() {
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
    void primaryActionEnabledFoldsEnabled() {
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
    void modelSelectionClampsAndNotifies() {
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
    void modelSourceUnifiesWithCatalog() {
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

    // --- Phase E: foreign-session live model selection ---------------------------
    // A native session (default fake backend) exposes no foreign backend/selector, so the composer
    // treats it exactly as before (the GUI keeps the pill hidden for a foreign engine only via the
    // backend fork).
    void foreignProjectionEmptyForNativeSession() {
        ForeignCatalogFake fake; // backend defaults to "" (native)
        ComposerSessionController c;
        c.setModelSource(&fake);
        c.setSessionId(QStringLiteral("s-native"));
        QVERIFY(c.foreignBackend().isEmpty());
        QVERIFY(!c.foreignModelSelector().value(QStringLiteral("hasSelector")).toBool());
    }

    // AgentNative: the controller projects the agent-advertised selector, and a pick routes through
    // SetSessionModel by model id (never the default-model activate path).
    void foreignAgentNativeProjectsSelectorAndSetsModel() {
        ForeignCatalogFake fake;
        fake.backend = QStringLiteral("AgentNative");
        fake.selector = agentNativeSelector();
        ComposerSessionController c;
        c.setModelSource(&fake);
        c.setSessionId(QStringLiteral("s-agent"));

        QCOMPARE(c.foreignBackend(), QStringLiteral("AgentNative"));
        const QVariantMap sel = c.foreignModelSelector();
        QVERIFY(sel.value(QStringLiteral("hasSelector")).toBool());
        QCOMPARE(sel.value(QStringLiteral("current")).toString(), QStringLiteral("sonnet"));
        QCOMPARE(sel.value(QStringLiteral("choices")).toList().size(), 2);
        // The bound session's detail is hydrated on bind.
        QVERIFY(fake.ensuredSessions.contains(QStringLiteral("s-agent")));

        // Picking sends SetSessionModel{session, model} — model only, never a provider.
        c.selectForeignModel(QStringLiteral("opus"));
        QCOMPARE(fake.lastSetSession, QStringLiteral("s-agent"));
        QCOMPARE(fake.lastSetModel, QStringLiteral("opus"));
    }

    // NodeProvider: no advertised selector (choices come from the node catalog), but the backend is
    // reported and a pick still routes through SetSessionModel by model id.
    void foreignNodeProviderReportsBackendAndSetsModel() {
        ForeignCatalogFake fake;
        fake.backend = QStringLiteral("NodeProvider");
        ComposerSessionController c;
        c.setModelSource(&fake);
        c.setSessionId(QStringLiteral("s-node"));

        QCOMPARE(c.foreignBackend(), QStringLiteral("NodeProvider"));
        QVERIFY(!c.foreignModelSelector().value(QStringLiteral("hasSelector")).toBool());

        c.selectForeignModel(QStringLiteral("gpt-5.3"));
        QCOMPARE(fake.lastSetSession, QStringLiteral("s-node"));
        QCOMPARE(fake.lastSetModel, QStringLiteral("gpt-5.3"));
    }

    // A catalog-side selector change for the bound session re-emits foreignModelSelectorChanged so
    // the GUI/TUI re-project (this is the SessionMetaChanged -> re-hydrate path's client end).
    void foreignSelectorChangeReprojects() {
        ForeignCatalogFake fake;
        fake.backend = QStringLiteral("AgentNative");
        fake.selector = agentNativeSelector();
        ComposerSessionController c;
        c.setModelSource(&fake);
        c.setSessionId(QStringLiteral("s-agent"));

        QSignalSpy spy(&c, &ComposerSessionController::foreignModelSelectorChanged);
        fake.fireSelectorChanged(QStringLiteral("s-agent")); // bound session -> re-project
        QCOMPARE(spy.count(), 1);
        fake.fireSelectorChanged(QStringLiteral("other")); // unrelated session -> ignored
        QCOMPARE(spy.count(), 1);
    }

    // --- Reverse incremental history search (Ctrl+R) -----------------------------
    // Seeds a three-entry history (oldest -> newest) for the active session.
    static void seedHistory(ComposerSessionController& c) {
        c.setSessionId(QStringLiteral("s-1"));
        const char* entries[] = {"alpha one", "beta two", "alpha three"};
        for (const char* e : entries) {
            c.setDraft(QString::fromLatin1(e));
            c.submit(); // idle send: pushes history + clears the draft
        }
    }

    // Typing narrows to the newest match; Ctrl+R steps to the next older match and
    // flags failure (keeping the last preview) when none remains. The match is
    // previewed into the draft via draftReset so both front ends show it.
    void reverseSearchNarrowsAndSteps() {
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
    void reverseSearchBackspaceWidensAndRestores() {
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
    void reverseSearchAcceptKeepsDraftWithoutSubmit() {
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
    void reverseSearchCancelRestoresDraft() {
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
    void reverseSearchResetsOnSessionSwap() {
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
