#include "connection/mock_connection_service.h"
#include "daemon/app_service_graph.h"
#include "daemon/daemon_connection_service.h"
#include "domain/session.h"
#include "domain/sidebar_node.h"
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
        QVERIFY(graph.files != nullptr);
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
};

QTEST_MAIN(AppServiceGraphTests)

#include "tst_app_service_graph.moc"
