// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "connection/mock_connection_service.h"
#include "daemon/app_service_graph.h"
#include "daemon/daemon_connection_service.h"
#include "daemonnet/idaemonnet.h"
#include "domain/session.h"
#include "domain/sidebar_node.h"
#include "fleet/idashboard.h"
#include "persistence/isession_store.h"

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
        QVERIFY(graph.routing != nullptr);
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

    // W3 (plan 2d): Daemon mode must not pass the mock transports tree off as live Integrations.
    // The DaemonNet still constructs (the roster/routing mocks project from it), but its events-IO
    // tree seeds empty until the daemon-backed projection lands; DAEMON_APP_MOCK_INTEGRATIONS is
    // the developer escape hatch, and mock mode keeps the demo tree unconditionally.
    void daemonGraphSeedsEmptyIntegrationsUnlessFlagged() {
        qunsetenv("DAEMON_APP_MOCK_INTEGRATIONS");
        QObject owner;
        const auto daemonGraph = daemonapp::daemon::createAppServiceGraph(
            daemonapp::daemon::ServiceMode::Daemon, &owner);
        QVERIFY(daemonGraph.daemonNet != nullptr);
        QVERIFY(daemonGraph.daemonNet->transportsTree().isEmpty());

        qputenv("DAEMON_APP_MOCK_INTEGRATIONS", "1");
        const auto flaggedGraph = daemonapp::daemon::createAppServiceGraph(
            daemonapp::daemon::ServiceMode::Daemon, &owner);
        QVERIFY(!flaggedGraph.daemonNet->transportsTree().isEmpty());
        qunsetenv("DAEMON_APP_MOCK_INTEGRATIONS");

        const auto mockGraph =
            daemonapp::daemon::createAppServiceGraph(daemonapp::daemon::ServiceMode::Mock, &owner);
        QVERIFY(!mockGraph.daemonNet->transportsTree().isEmpty());
    }
};

QTEST_MAIN(AppServiceGraphTests)

#include "tst_app_service_graph.moc"
