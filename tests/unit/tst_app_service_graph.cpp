#include "daemon/app_service_graph.h"

#include "connection/mock_connection_service.h"
#include "daemon/daemon_connection_service.h"

#include <QtTest/QtTest>

class AppServiceGraphTests : public QObject {
    Q_OBJECT

private slots:
    void mockGraphConstructsAllSharedServices()
    {
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

    void daemonGraphSwapsConnectionOnlyForNow()
    {
        QObject owner;
        const auto graph =
            daemonapp::daemon::createAppServiceGraph(daemonapp::daemon::ServiceMode::Daemon, &owner);

        QVERIFY(qobject_cast<daemonapp::daemon::DaemonConnectionService*>(graph.connection) != nullptr);
        QVERIFY(graph.nodeApi != nullptr);
        QVERIFY(graph.sessions != nullptr);
        QVERIFY(graph.cache != nullptr);
        QVERIFY(graph.fs != nullptr);
        QVERIFY(graph.store != nullptr);
    }
};

QTEST_MAIN(AppServiceGraphTests)

#include "tst_app_service_graph.moc"
