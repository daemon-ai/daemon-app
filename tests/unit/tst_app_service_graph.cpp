// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "connection/mock_connection_service.h"
#include "daemon/app_service_graph.h"
#include "daemon/daemon_connection_service.h"
#include "daemon/mock_scenario.h"
#include "daemon/mock_scenario_host.h"
#include "domain/session.h"
#include "domain/sidebar_node.h"
#include "fleet/idashboard.h"
#include "mirror/chat_window_model.h"
#include "mirror/mirror_service.h"
#include "outbox.h"
#include "persistence/isession_store.h"

#include <algorithm>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QtTest/QtTest>

class AppServiceGraphTests : public QObject {
    Q_OBJECT

private slots:
    void mockGraphConstructsAllSharedServices() {
        QObject owner;
        const auto graph =
            daemonapp::daemon::createAppServiceGraph(daemonapp::daemon::ServiceMode::Mock, &owner);

        QVERIFY(graph.storeMirror != nullptr);
        QVERIFY(graph.settings != nullptr);
        QVERIFY(graph.connection != nullptr);
        QVERIFY(graph.daemonConfig != nullptr);
        QVERIFY(graph.fs != nullptr);
        QVERIFY(graph.memory != nullptr);
        QVERIFY(graph.nav != nullptr);
        QVERIFY(graph.firstRun != nullptr);
        QVERIFY(graph.modelCatalog != nullptr);
        QVERIFY(graph.accounts != nullptr);
        QVERIFY(graph.profiles != nullptr);
        QVERIFY(graph.roster != nullptr);
        QVERIFY(graph.fleetTree != nullptr);
        QVERIFY(graph.approvals != nullptr);
        QVERIFY(graph.dashboard != nullptr);
        QVERIFY(graph.cron != nullptr);
        QVERIFY(graph.sessionSettings != nullptr);
        QVERIFY(graph.checkpoints != nullptr);
        QVERIFY(graph.cache != nullptr);
        QVERIFY(graph.nodeApi != nullptr);
        QVERIFY(graph.sessions != nullptr);
        QVERIFY(graph.profileRepository != nullptr);
        QVERIFY(graph.models != nullptr);
        QVERIFY(graph.approvalRepository != nullptr);
        QVERIFY(graph.checkpointRepository != nullptr);
        QVERIFY(qobject_cast<connection::MockConnectionService*>(graph.connection) != nullptr);
    }

    // AD single-source: the ONE scenario seeds the mock mirror; the store every consumer binds
    // renders its sessions with authoritative string ids (and the demoted int handle).
    void mockStoreIsSeededFromScenario() {
        QObject owner;
        const auto graph =
            daemonapp::daemon::createAppServiceGraph(daemonapp::daemon::ServiceMode::Mock, &owner);

        const QList<domain::Session> sessions = graph.storeMirror->sessions(domain::ListScope{});
        QVERIFY(sessions.size() >= 1);
        for (const domain::Session& s : sessions) {
            QVERIFY2(!s.sessionId.isEmpty(), "every seeded session must carry its real SessionId");
            QVERIFY(s.id >= 0);
        }
        // The mock fleet tree is the mirror projection over the seeded fleet_units.
        QVERIFY(graph.fleetTree != nullptr);
    }

    // Live agent enablement #1: the code default is Daemon (unset env), and DAEMON_APP_SERVICE_MODE
    // =mock is the documented escape hatch. The offscreen suite is unaffected because tests pass
    // the mode explicitly; this only pins the resolver + the mock construction under the flag.
    void serviceModeDefaultsToDaemonAndMockEscapeHatch() {
        qunsetenv("DAEMON_APP_SERVICE_MODE");
        QCOMPARE(daemonapp::daemon::serviceModeFromEnvironment(),
                 daemonapp::daemon::ServiceMode::Daemon);

        qputenv("DAEMON_APP_SERVICE_MODE", "mock");
        QCOMPARE(daemonapp::daemon::serviceModeFromEnvironment(),
                 daemonapp::daemon::ServiceMode::Mock);
        qputenv("DAEMON_APP_SERVICE_MODE", "MoCk"); // case-insensitive
        QCOMPARE(daemonapp::daemon::serviceModeFromEnvironment(),
                 daemonapp::daemon::ServiceMode::Mock);
        qputenv("DAEMON_APP_SERVICE_MODE", "daemon"); // explicit daemon still honored
        QCOMPARE(daemonapp::daemon::serviceModeFromEnvironment(),
                 daemonapp::daemon::ServiceMode::Daemon);
        qunsetenv("DAEMON_APP_SERVICE_MODE");

        // The mock escape hatch still yields the mock connection seam (and thus the mock turn
        // simulator via the app graph's DaemonConnectionService gate).
        QObject owner;
        const auto graph =
            daemonapp::daemon::createAppServiceGraph(daemonapp::daemon::ServiceMode::Mock, &owner);
        QVERIFY(qobject_cast<connection::MockConnectionService*>(graph.connection) != nullptr);
        QVERIFY(qobject_cast<daemonapp::daemon::DaemonConnectionService*>(graph.connection) ==
                nullptr);
    }

    void daemonGraphSwapsConnectionOnlyForNow() {
        QObject owner;
        const auto graph = daemonapp::daemon::createAppServiceGraph(
            daemonapp::daemon::ServiceMode::Daemon, &owner);

        QVERIFY(qobject_cast<daemonapp::daemon::DaemonConnectionService*>(graph.connection) !=
                nullptr);
        QVERIFY(graph.nodeApi != nullptr);
        QVERIFY(graph.sessions != nullptr);
        QVERIFY(graph.cache != nullptr);
        QVERIFY(graph.fs != nullptr);
        QVERIFY(graph.storeMirror != nullptr);
    }

    // Regression: the daemon-mode seam replacements delete the mock fleet tree the dashboard was
    // built over; the dashboard must be rebuilt over the final pointers or its counters read
    // deleted memory (SIGSEGV in MockDashboard::runningAgents() when the Dashboard page opened).
    void daemonGraphDashboardObservesDaemonSeams() {
        QObject owner;
        const auto graph = daemonapp::daemon::createAppServiceGraph(
            daemonapp::daemon::ServiceMode::Daemon, &owner);

        QVERIFY(graph.dashboard != nullptr);
        // Reads through roster + fleet tree + approvals; crashes on a dangling seam pointer.
        QVERIFY(graph.dashboard->runningAgents() >= 0);
        QVERIFY(graph.dashboard->activeSessions() >= 0);
        QVERIFY(graph.dashboard->pendingApprovals() >= 0);
    }

    // mirror A5 (spec 09 wave M2, E1): the REAL daemon graph persists the mirror per identity and
    // a cold "reboot" with NO connection renders last-known state — M tables AND the chat
    // window's persisted tail — from mirror.db. Mock mode gets NO mirror at M2 (legacy fallback;
    // A8's seeder changes that).
    void daemonGraphMirrorPersistsAndRebootsOffline() {
        QStandardPaths::setTestModeEnabled(true);
        // Deterministic start: default identity + a fresh default mirror.db in the test AppData.
        QSettings().remove(QStringLiteral("mirror/lastIdentity"));
        const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QFile::remove(QDir(appData).filePath(QStringLiteral("mirror.db")));

        const mirror::ChatMessageScope scope{QStringLiteral("m"), QStringLiteral("!room")};
        {
            QObject owner;
            const auto graph = daemonapp::daemon::createAppServiceGraph(
                daemonapp::daemon::ServiceMode::Daemon, &owner);
            QVERIFY(graph.mirrorService != nullptr);
            QVERIFY(graph.mirrorExecutor != nullptr);
            QVERIFY(graph.outbox != nullptr);
            QVERIFY(graph.mirrorService->persistent()); // per-identity file, not in-memory
            // "Online session": the ingestor applies node truth; flush-on-commit persists it.
            mirror::Conversation c;
            c.transport = QStringLiteral("m");
            c.id = QStringLiteral("!room");
            c.title = QStringLiteral("The Room");
            graph.mirrorService->ingestor().deliverConversations(QStringLiteral("m"), {c},
                                                                 /*isFinalPage=*/true);
            mirror::ChatMessage m1;
            m1.transport = QStringLiteral("m");
            m1.conv = QStringLiteral("!room");
            m1.cursor = 1;
            m1.text = QStringLiteral("hello");
            graph.mirrorService->ingestor().deliverChatPage(QStringLiteral("m"),
                                                            QStringLiteral("!room"), {m1},
                                                            /*nextCursor=*/1, /*headCursor=*/1);
        }
        // Cold reboot, still offline (nothing ever connected): the graph re-opens the same
        // per-identity file and the surfaces render last-known state end-to-end.
        {
            QObject owner;
            const auto graph = daemonapp::daemon::createAppServiceGraph(
                daemonapp::daemon::ServiceMode::Daemon, &owner);
            QVERIFY(graph.mirrorService != nullptr);
            const auto& snap = graph.mirrorService->store().snapshot();
            QCOMPARE(snap.conversations.size(), std::size_t(1));
            QVERIFY(snap.chat.count(scope) != 0U);
            mirror::ChatWindowModel timeline(graph.mirrorService->store(), scope);
            QCOMPARE(timeline.rowCount(), 1);
            QCOMPARE(
                timeline.data(timeline.index(0, 0), mirror::ChatWindowModel::TextRole).toString(),
                QStringLiteral("hello"));
        }

        // mirror A8 (M5): mock mode now runs the LIVE mock mirror — in-memory (never persisted),
        // scenario-seeded, with a live outbox. The A5 null-mirror fallback is deleted.
        {
            QObject owner;
            const auto graph = daemonapp::daemon::createAppServiceGraph(
                daemonapp::daemon::ServiceMode::Mock, &owner);
            QVERIFY(graph.mirrorService != nullptr);
            QVERIFY(!graph.mirrorService->persistent());
            QVERIFY(graph.outbox != nullptr);
            QVERIFY(graph.mockHost != nullptr);
        }
    }

    // mirror A8 (M5) → AD: storeMirror is the REAL MirrorSessionStore projection in BOTH modes
    // (the legacy delegate stores are deleted). The mock projection renders the scenario's
    // seeded sessions with mirror-served transcript content.
    void storeMirrorIsTheMirrorProjectionInBothModes() {
        {
            QObject owner;
            const auto graph = daemonapp::daemon::createAppServiceGraph(
                daemonapp::daemon::ServiceMode::Mock, &owner);
            QVERIFY(graph.storeMirror != nullptr);
            const QList<domain::Session> mirrorRows =
                graph.storeMirror->sessions(domain::ListScope{});
            QVERIFY(!mirrorRows.isEmpty());
            // The seeded transcript window serves content for the scenario's sessions.
            const domain::SessionId scratch{QStringLiteral("s-scratch")};
            QVERIFY(!graph.storeMirror->content(scratch).isEmpty());
            QVERIFY(graph.storeMirror->content(scratch).contains(QStringLiteral("```msg")));
        }
        {
            QObject owner;
            const auto graph = daemonapp::daemon::createAppServiceGraph(
                daemonapp::daemon::ServiceMode::Daemon, &owner);
            QVERIFY(graph.storeMirror != nullptr);
            // The projection starts empty (no connection): reads answer, never crash.
            QCOMPARE(graph.storeMirror->sessionCount(domain::ListScope{}), 0);
        }
    }

    // mirror A8 (M5): the mock mirror renders the scenario's chat/conversation/routing seed —
    // the surfaces that went EMPTY in mock at M2/M3 (chat timeline, routing manager) have data.
    void mockMirrorSeedsScenarioData() {
        QObject owner;
        const auto graph =
            daemonapp::daemon::createAppServiceGraph(daemonapp::daemon::ServiceMode::Mock, &owner);
        QVERIFY(graph.mirrorService != nullptr);
        const auto& snap = graph.mirrorService->store().snapshot();
        QVERIFY(snap.conversations.size() >= 2);
        QVERIFY(snap.route_pins.size() >= 1);
        QVERIFY(snap.rooms.size() >= 2);
        QVERIFY(snap.transport_accounts.size() >= 1);

        const mirror::ChatMessageScope scope{QStringLiteral("matrix/@you:matrix.org"),
                                             QStringLiteral("!daemon-dev:matrix.org")};
        QVERIFY(snap.chat.count(scope) != 0U);
        mirror::ChatWindowModel timeline(graph.mirrorService->store(), scope);
        QVERIFY(timeline.rowCount() >= 4);

        // Every seeded journal record is stamped origin = seeder (§9 — no mock-only apply).
        for (const mirror::JournalRecord& rec : graph.mirrorService->store().journal().records()) {
            QCOMPARE(rec.origin, mirror::JournalOrigin::Seeder);
        }
    }

    // mirror A8 (M5): the app-level api/<N> mock Hello — the DEFAULT scenario advertises api/39:
    // ready ⇒ FULL mode (Bootstrap probe, no degraded scan) + auto-replay drains a pre-enqueued
    // op, which then lands through the scripted ok-outcome's provenance echo (§5.6/§6.6/§6.8).
    void mockFullModeBootstrapAndAutoReplayAtAppLevel() {
        qunsetenv("DAEMON_APP_MOCK_SCENARIO");
        QObject owner;
        const auto graph =
            daemonapp::daemon::createAppServiceGraph(daemonapp::daemon::ServiceMode::Mock, &owner);
        QCOMPARE(graph.mockHost->apiVersion(), 39);

        // Enqueue BEFORE ready: durable, held (the gate requires a ready connection).
        const QString opId = graph.outbox->enqueue(
            QStringLiteral("ConvSend"),
            QStringLiteral("matrix/@you:matrix.org\x1f!daemon-dev:matrix.org"),
            QJsonDocument(
                QJsonObject{{QStringLiteral("transport"), QStringLiteral("matrix/@you:matrix.org")},
                            {QStringLiteral("conv"), QStringLiteral("!daemon-dev:matrix.org")},
                            {QStringLiteral("message"), QStringLiteral("hello from mock")}})
                .toJson(QJsonDocument::Compact),
            QStringLiteral("hello from mock"));
        QVERIFY(!opId.isEmpty());
        graph.outbox->drain(); // gated: not ready yet
        QCOMPARE(graph.outbox->op(opId).state, mirror::OpState::Pending);

        // Drive the mock connection to ready ("mock"/"ready" is the deterministic liveness path).
        graph.connection->connectTo(QStringLiteral("mock"), QStringLiteral("ready"));
        QTRY_VERIFY_WITH_TIMEOUT(graph.connection->ready(), 5000);

        // FULL mode: the Bootstrap probe was issued. (A SessionsQuery may ALSO run — the
        // MirrorSessionStore's beginObserving("sessions") visibility declaration drives a §5.8
        // staleness fetch in both modes; the Bootstrap probe is the mode discriminator.)
        bool sawBootstrap = false;
        for (const mirror::FetchJob& j : graph.mockHost->executedJobs()) {
            sawBootstrap = sawBootstrap || j.op == mirror::FetchOp::Bootstrap;
        }
        QVERIFY(sawBootstrap);

        // Auto-replay (§6.8) drained unattended; the scripted ok echo lands the op (§6.6).
        QTRY_VERIFY_WITH_TIMEOUT(!graph.outbox->contains(opId), 5000);
        // The echoed line is a provenance-stamped mirror row in the conversation's window.
        const mirror::ChatMessageScope scope{QStringLiteral("matrix/@you:matrix.org"),
                                             QStringLiteral("!daemon-dev:matrix.org")};
        mirror::ChatWindowModel timeline(graph.mirrorService->store(), scope);
        QCOMPARE(
            timeline
                .data(timeline.index(timeline.rowCount() - 1, 0), mirror::ChatWindowModel::TextRole)
                .toString(),
            QStringLiteral("hello from mock"));
    }

    // mirror A8 (M5): the api38 scenario — degraded reconnect (staleness scan, no Bootstrap) and
    // the auto-replay gate HOLDS (manual drain only; a blind resend can duplicate at v38).
    void mockDegradedModeHoldsAutoReplayAtAppLevel() {
        qputenv("DAEMON_APP_MOCK_SCENARIO", "api38");
        QObject owner;
        const auto graph =
            daemonapp::daemon::createAppServiceGraph(daemonapp::daemon::ServiceMode::Mock, &owner);
        qunsetenv("DAEMON_APP_MOCK_SCENARIO");
        QCOMPARE(graph.mockHost->apiVersion(), 38);

        const QString opId = graph.outbox->enqueue(
            QStringLiteral("ConvSend"),
            QStringLiteral("matrix/@you:matrix.org\x1f!design:matrix.org"),
            QJsonDocument(
                QJsonObject{{QStringLiteral("transport"), QStringLiteral("matrix/@you:matrix.org")},
                            {QStringLiteral("conv"), QStringLiteral("!design:matrix.org")},
                            {QStringLiteral("message"), QStringLiteral("held until tap")}})
                .toJson(QJsonDocument::Compact),
            QStringLiteral("held until tap"));

        graph.connection->connectTo(QStringLiteral("mock"), QStringLiteral("ready"));
        QTRY_VERIFY_WITH_TIMEOUT(graph.connection->ready(), 5000);

        // DEGRADED mode: staleness scan ran; no Bootstrap probe.
        bool sawBootstrap = false;
        bool sawSessionsScan = false;
        for (const mirror::FetchJob& j : graph.mockHost->executedJobs()) {
            sawBootstrap = sawBootstrap || j.op == mirror::FetchOp::Bootstrap;
            sawSessionsScan =
                sawSessionsScan || (j.op == mirror::FetchOp::SessionsQuery && j.scope.isEmpty());
        }
        QVERIFY(!sawBootstrap);
        QVERIFY(sawSessionsScan);

        // The lane HELD (no unattended replay at api/38) — the op is still pending.
        QTest::qWait(150); // give any (wrong) auto-drain a chance to surface
        QCOMPARE(graph.outbox->op(opId).state, mirror::OpState::Pending);

        // An explicit user tap always drains; against api/38 the ack is terminal (§6.6).
        graph.outbox->drain();
        QTRY_VERIFY_WITH_TIMEOUT(!graph.outbox->contains(opId), 5000);
    }

    // AD (§6.4 session-meta lane) at app level in MOCK: a pin routes through the durable outbox
    // lane; the scripted ok acks + echoes the patched row provenance-stamped (§6.6), so the op
    // LANDS and the roster re-projects event-driven — never an optimistic local write. A
    // "/reject" rename pauses the lane (§6.5) and relays metaUpdateFailed — the exact signal the
    // GUI toast + TUI notification already bind.
    void mockSessionMetaLaneRoundTripAndRejection() {
        qunsetenv("DAEMON_APP_MOCK_SCENARIO");
        QObject owner;
        const auto graph =
            daemonapp::daemon::createAppServiceGraph(daemonapp::daemon::ServiceMode::Mock, &owner);
        graph.connection->connectTo(QStringLiteral("mock"), QStringLiteral("ready"));
        QTRY_VERIFY_WITH_TIMEOUT(graph.connection->ready(), 5000);

        const QList<domain::Session> rows = graph.storeMirror->sessions(domain::ListScope{});
        QVERIFY(!rows.isEmpty());
        const domain::SessionId id = rows.first().sessionId;
        const bool target = !graph.storeMirror->isPinned(id);

        graph.storeMirror->setPinned(id, target);
        // The scripted ok (delay 0) acks; the 120 ms echo applies the patched row through the
        // seeder with origin_op = op_id → the mirror row flips AND the op lands (§6.6).
        QTRY_COMPARE_WITH_TIMEOUT(graph.storeMirror->isPinned(id), target, 5000);
        QTRY_VERIFY_WITH_TIMEOUT(
            [&] {
                const QList<mirror::PendingOp> ops = graph.outbox->ops();
                return std::none_of(ops.cbegin(), ops.cend(), [](const mirror::PendingOp& o) {
                    return o.verb == QStringLiteral("SessionUpdateMeta");
                });
            }(),
            5000);

        // The MANDATORY rejection fixture (§9): "/reject" rename → Forbidden → lane pause +
        // the verb seam's failure signal; the title never changes (no local mutation to undo).
        QSignalSpy failed(graph.storeMirror, &persistence::ISessionStore::metaUpdateFailed);
        graph.storeMirror->renameSession(id, QStringLiteral("/reject me"));
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 5000);
        QCOMPARE(failed.first().at(0).toString(), id.toString());
        QVERIFY(failed.first().at(1).toString().startsWith(QStringLiteral("Forbidden")));
        QVERIFY(
            graph.outbox->lanePaused(QStringLiteral("session-meta") + QChar(0x1f) + id.toString()));
        QVERIFY(graph.storeMirror->title(id) != QStringLiteral("/reject me"));
    }

    // mirror A8 (M5): the MANDATORY rejection script at app level — "/reject" pauses the lane
    // with the typed api-error (§6.5); retry/edit/discard affordances stay reachable.
    void mockRejectionScriptPausesLaneAtAppLevel() {
        qunsetenv("DAEMON_APP_MOCK_SCENARIO");
        QObject owner;
        const auto graph =
            daemonapp::daemon::createAppServiceGraph(daemonapp::daemon::ServiceMode::Mock, &owner);
        graph.connection->connectTo(QStringLiteral("mock"), QStringLiteral("ready"));
        QTRY_VERIFY_WITH_TIMEOUT(graph.connection->ready(), 5000);

        const QString lane = QStringLiteral("matrix/@you:matrix.org\x1f!daemon-dev:matrix.org");
        const QString opId = graph.outbox->enqueue(
            QStringLiteral("ConvSend"), lane,
            QJsonDocument(
                QJsonObject{{QStringLiteral("transport"), QStringLiteral("matrix/@you:matrix.org")},
                            {QStringLiteral("conv"), QStringLiteral("!daemon-dev:matrix.org")},
                            {QStringLiteral("message"), QStringLiteral("/reject this")}})
                .toJson(QJsonDocument::Compact),
            QStringLiteral("/reject this"));
        graph.outbox->drain();
        QTRY_COMPARE_WITH_TIMEOUT(graph.outbox->op(opId).state, mirror::OpState::Rejected, 5000);
        QVERIFY(graph.outbox->lanePaused(graph.outbox->op(opId).lane));
        // lastError carries the typed api-error: "<kind>: <message>" (§6.5).
        QVERIFY(graph.outbox->op(opId).lastError.startsWith(QStringLiteral("Forbidden: ")));
    }
};

QTEST_MAIN(AppServiceGraphTests)

#include "tst_app_service_graph.moc"
