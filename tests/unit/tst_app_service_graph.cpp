// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "connection/mock_connection_service.h"
#include "daemon/app_service_graph.h"
#include "daemon/daemon_connection_service.h"
#include "domain/session.h"
#include "domain/sidebar_node.h"
#include "fleet/idashboard.h"
#include "mirror/chat_window_model.h"
#include "mirror/mirror_service.h"
#include "persistence/isession_store.h"

#include <QSettings>
#include <QStandardPaths>
#include <QtTest/QtTest>

class AppServiceGraphTests : public QObject {
    Q_OBJECT

private slots:
    void mockGraphConstructsAllSharedServices() {
        QObject owner;
        const auto graph =
            daemonapp::daemon::createAppServiceGraph(daemonapp::daemon::ServiceMode::Mock, &owner);

        QVERIFY(graph.store != nullptr);
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

    // P1 single-source: in mock mode the session store is re-seeded from the unified DaemonNet, so
    // the sidebar/list/transcript reflect the same source as the Fleet/Sessions pages - and every
    // session now carries its authoritative string sessionId (not just an int handle).
    void mockStoreIsSeededFromDaemonNet() {
        QObject owner;
        const auto graph =
            daemonapp::daemon::createAppServiceGraph(daemonapp::daemon::ServiceMode::Mock, &owner);

        // The supervision tree came from DaemonNet (fleet-of-fleets roots present).
        const QList<domain::UnitNode> roots = graph.store->unitChildren(domain::UnitId());
        QVERIFY(!roots.isEmpty());

        // Tags folded in from the seed (ideas / todo).
        QVERIFY(graph.store->tags().size() >= 2);

        // Sessions are present and each carries a non-empty, authoritative sessionId while also
        // holding a stable int handle (the transitional dual-identity P1 establishes).
        const QList<domain::Session> sessions = graph.store->sessions(domain::ListScope{});
        QVERIFY(sessions.size() >= 1);
        for (const domain::Session& s : sessions) {
            QVERIFY2(!s.sessionId.isEmpty(), "every seeded session must carry its real SessionId");
            QVERIFY(s.id >= 0);
        }
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
        QVERIFY(graph.store != nullptr);
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

        // Mock mode: no mirror at M2 — the chat surfaces keep the legacy mock path (§9).
        {
            QObject owner;
            const auto graph = daemonapp::daemon::createAppServiceGraph(
                daemonapp::daemon::ServiceMode::Mock, &owner);
            QVERIFY(graph.mirrorService == nullptr);
            QVERIFY(graph.outbox == nullptr);
        }
    }
};

QTEST_MAIN(AppServiceGraphTests)

#include "tst_app_service_graph.moc"
