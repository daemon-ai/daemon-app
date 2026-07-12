// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "cache_test_support.h"
#include "daemon/mirror_session_store.h"
#include "i_turn_engine.h"
#include "mirror/mirror_service.h"
#include "profiles/mock_profile_store.h"
#include "session/mock_session_settings.h"
#include "session_controller.h"
#include "session_orchestrator.h"
#include "todo_list_model.h"
#include "turn_controller.h"
#include "turn_engine_factory.h"

#include <memory>
#include <QSignalSpy>
#include <QtTest>

namespace {

// A turn engine that only records the profile bound to it, so the orchestrator's #6b wiring
// (per-session profile -> resolved id -> setProfile) is observable without a live daemon.
class RecordingTurnEngine : public ITurnEngine {
    Q_OBJECT

public:
    using ITurnEngine::ITurnEngine;

    [[nodiscard]] bool active() const override { return m_active; }
    [[nodiscard]] QString turnState() const override { return QStringLiteral("idle"); }
    [[nodiscard]] int elapsedMs() const override { return 0; }
    [[nodiscard]] QString errorText() const override { return {}; }

    void setProfile(const QString& profile) override { boundProfile = profile; }
    void start(const QString& /*prompt*/) override { m_active = true; }
    void cancel() override { m_active = false; }
    void resume() override {}

    // Test seam: replay a daemon-shaped event batch through the engine's stream signal.
    void emitEvents(const QVariantList& events) { emit eventsEmitted(events); }

    QString boundProfile;

private:
    bool m_active = false;
};

class RecordingTurnEngineFactory : public ITurnEngineFactory {
    Q_OBJECT

public:
    using ITurnEngineFactory::ITurnEngineFactory;
    [[nodiscard]] ITurnEngine* create(QObject* parent) override {
        last = new RecordingTurnEngine(parent);
        return last;
    }

    RecordingTurnEngine* last = nullptr;
};

} // namespace

// AD: the production MirrorSessionStore over an in-memory mirror replaces the deleted InMemory
// fixture. The direct-create seam is answered synchronously here (the unit-test twin of the
// scenario host's scripted SessionCreate): createRequested -> a minted id seeded back through
// onNodeSessionCreated, so SessionController::createSession() adopts in-line like before.
struct SyncCreateMirrorStore {
    mirror::MirrorService svc;
    std::unique_ptr<daemonapp::daemon::MirrorSessionStore> store;
    int seq = 0;
    SyncCreateMirrorStore() {
        svc.openInMemory();
        store =
            std::make_unique<daemonapp::daemon::MirrorSessionStore>(&svc.store(), &svc.ingestor());
        QObject::connect(store.get(), &daemonapp::daemon::MirrorSessionStore::createRequested,
                         store.get(), [this](const QString& profileId) {
                             store->onNodeSessionCreated(QStringLiteral("s-t-%1").arg(++seq),
                                                         profileId);
                         });
    }
};

// Exercises the shared submit pipeline (SessionOrchestrator): submit starts
// the turn and populates the status-stack todos; the todos clear a beat after the
// turn settles; cancel stops the turn; slash "/new" is handled here while other
// commands surface via commandRequested. Both front ends drive this identically.
// The profile* cases guard #6b: the per-session profile selection reaches the turn engine
// (resolved to the node's canonical id), and empty = the node's active profile.
class TestSessionOrchestrator : public QObject {
    Q_OBJECT

private slots:
    void init() { resetMockCache(); }

    void submitStartsTurnAndPopulatesTodos() {
        // The default engine is the mock simulator (simulatesStatusFeeds), so submit seeds the
        // canned 3-row plan.
        SessionOrchestrator orch;
        QVERIFY(!orch.busy());
        QCOMPARE(orch.todos()->count(), 0);

        orch.submit(QStringLiteral("hi"), QString());

        QVERIFY(orch.busy());
        QVERIFY(orch.turn()->active());
        QVERIFY(orch.turn()->simulatesStatusFeeds());
        QCOMPARE(orch.todos()->count(), 3);

        orch.cancel();
        QVERIFY(!orch.busy());
        QVERIFY(!orch.turn()->active());
    }

    // A non-simulator engine (the daemon engine's seam: simulatesStatusFeeds == false) must NOT
    // get canned todos on submit — its todos arrive only as real `todoUpdate` stream events.
    void daemonEngineSubmitSeedsNoCannedTodos() {
        SessionOrchestrator orch;
        RecordingTurnEngineFactory factory;
        orch.setTurnEngines(&factory);
        QVERIFY(factory.last != nullptr);
        QVERIFY(!orch.turn()->simulatesStatusFeeds());

        orch.submit(QStringLiteral("hi"), QString());

        QCOMPARE(orch.todos()->count(), 0);
        orch.cancel();
    }

    // A `todoUpdate` event on the turn stream (the daemon engine's projection of the agent's
    // `todo` tool detail) replaces the status-stack todos with the FULL list — any N items,
    // contents and done flags intact; unrelated events are ignored.
    void todoUpdateEventPopulatesTodos() {
        SessionOrchestrator orch;
        RecordingTurnEngineFactory factory;
        orch.setTurnEngines(&factory);
        QVERIFY(factory.last != nullptr);

        constexpr int kItems = 7; // deliberately != the simulator's canned 3
        QVariantList items;
        for (int i = 0; i < kItems; ++i) {
            items.push_back(QVariantMap{{QStringLiteral("text"), QStringLiteral("task %1").arg(i)},
                                        {QStringLiteral("done"), i % 2 == 0}});
        }
        factory.last->emitEvents(QVariantList{
            QVariantMap{{QStringLiteral("type"), QStringLiteral("text")},
                        {QStringLiteral("text"), QStringLiteral("hi")}},
            QVariantMap{{QStringLiteral("type"), QStringLiteral("todoUpdate")},
                        {QStringLiteral("items"), items}},
        });

        QCOMPARE(orch.todos()->count(), kItems);
        for (int i = 0; i < kItems; ++i) {
            QCOMPARE(orch.todos()->textAt(i), QStringLiteral("task %1").arg(i));
            QCOMPARE(orch.todos()->doneAt(i), i % 2 == 0);
        }

        // The next update replaces the whole list (the tool returns the full current state).
        factory.last->emitEvents(QVariantList{QVariantMap{
            {QStringLiteral("type"), QStringLiteral("todoUpdate")},
            {QStringLiteral("items"),
             QVariantList{QVariantMap{{QStringLiteral("text"), QStringLiteral("Apply it")},
                                      {QStringLiteral("done"), true}}}}}});
        QCOMPARE(orch.todos()->count(), 1);
        QCOMPARE(orch.todos()->textAt(0), QStringLiteral("Apply it"));
        QVERIFY(orch.todos()->doneAt(0));
    }

    void submitAppendsUserTextToSession() {
        SyncCreateMirrorStore fx;
        SessionController controller;
        controller.setStore(fx.store.get());
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
        SyncCreateMirrorStore fx;
        SessionController controller;
        controller.setStore(fx.store.get());
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

    // #6b: the per-session profile selection (a display name in the popover/overlay) reaches the
    // turn engine as the canonical profile id the node resolves for Submit.profile.
    void sessionProfileReachesTurnEngine() {
        SyncCreateMirrorStore fx;
        SessionController controller;
        controller.setStore(fx.store.get());
        const QString sid = controller.createSession(QString());
        QVERIFY(!sid.isEmpty());

        session::MockSessionSettings settings;
        settings.setSessionId(sid);
        settings.setProfile(QStringLiteral("Coder")); // display name of the seeded prof-2

        profiles::MockProfileStore profileStore; // seeded: "Coder" -> "prof-2"

        SessionOrchestrator orch;
        orch.setSession(&controller);
        orch.setSessionSettings(&settings);
        orch.setProfileStore(&profileStore);
        RecordingTurnEngineFactory factory;
        orch.setTurnEngines(&factory);
        QVERIFY(factory.last != nullptr);

        orch.submit(QStringLiteral("hi"), QString());

        // The display-name selection resolved to the node's canonical id, not the label.
        QCOMPARE(factory.last->boundProfile, QStringLiteral("prof-2"));
        orch.cancel();
    }

    // #6b: no per-session override means empty reaches the engine, which the encoder omits from
    // Submit so the node applies its active profile.
    void emptyProfileMeansNodeActive() {
        SyncCreateMirrorStore fx;
        SessionController controller;
        controller.setStore(fx.store.get());
        const QString sid = controller.createSession(QString());
        QVERIFY(!sid.isEmpty());

        session::MockSessionSettings settings; // no override for this session (default empty)
        profiles::MockProfileStore profileStore;

        SessionOrchestrator orch;
        orch.setSession(&controller);
        orch.setSessionSettings(&settings);
        orch.setProfileStore(&profileStore);
        RecordingTurnEngineFactory factory;
        orch.setTurnEngines(&factory);
        QVERIFY(factory.last != nullptr);

        orch.submit(QStringLiteral("hi"), QString());

        QVERIFY(factory.last->boundProfile.isEmpty()); // empty = the node's active profile
        orch.cancel();
    }

    // #6b: changing the override on the bound session re-binds the engine live (via changed()),
    // without waiting for the next submit.
    void profileChangePropagatesLive() {
        SyncCreateMirrorStore fx;
        SessionController controller;
        controller.setStore(fx.store.get());
        const QString sid = controller.createSession(QString());
        QVERIFY(!sid.isEmpty());

        session::MockSessionSettings settings;
        settings.setSessionId(sid);
        settings.setProfile(QStringLiteral("Coder"));
        profiles::MockProfileStore profileStore;

        SessionOrchestrator orch;
        orch.setSession(&controller);
        orch.setSessionSettings(&settings);
        orch.setProfileStore(&profileStore);
        RecordingTurnEngineFactory factory;
        orch.setTurnEngines(&factory);
        QVERIFY(factory.last != nullptr);
        QCOMPARE(factory.last->boundProfile, QStringLiteral("prof-2"));

        settings.setProfile(QStringLiteral("Researcher")); // seeded prof-3
        QCOMPARE(factory.last->boundProfile, QStringLiteral("prof-3"));
    }

    // E1/TOOL-7: approvalModeFor(id) is a per-session read with no side effects — it reflects
    // the mode set for THAT session (default "ask"), regardless of which session is active.
    void approvalModeForReadsPerSession() {
        session::MockSessionSettings settings;
        settings.setSessionId(QStringLiteral("s1"));
        settings.setApprovalMode(QStringLiteral("auto_allow"));
        settings.setSessionId(QStringLiteral("s2")); // switch the active session away

        QCOMPARE(settings.approvalModeFor(QStringLiteral("s1")), QStringLiteral("auto_allow"));
        QCOMPARE(settings.approvalModeFor(QStringLiteral("s2")), QStringLiteral("ask"));
        // An id never bound still reads the default without materializing state.
        QCOMPARE(settings.approvalModeFor(QStringLiteral("never-seen")), QStringLiteral("ask"));
        QCOMPARE(settings.approvalMode(), QStringLiteral("ask")); // active (s2) untouched
    }
};

QTEST_GUILESS_MAIN(TestSessionOrchestrator)
#include "tst_session_orchestrator.moc"
