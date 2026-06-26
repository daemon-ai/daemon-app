#include "connection/mock_connection_service.h"

#include <QSignalSpy>
#include <QTemporaryFile>
#include <QtTest/QtTest>

using connection::MockConnectionService;

// Guards the mock connection seam's liveness state machine and probe: a plausible
// target drives checking -> connecting -> ready; an implausible one ends offline;
// testConnection reports success/failure; disconnect drops to offline.
class TestConnectionService : public QObject {
    Q_OBJECT

private slots:
    void reachesReadyForGoodTarget() {
        QTemporaryFile socketStandIn;
        QVERIFY(socketStandIn.open());
        MockConnectionService c;
        QSignalSpy spy(&c, &connection::IConnectionService::stateChanged);
        c.connectTo(QStringLiteral("local"), socketStandIn.fileName());
        QVERIFY(QTest::qWaitFor([&] { return c.state() == QStringLiteral("ready"); }, 3000));
        QVERIFY(c.ready());
        // It passed through "connecting" on the way.
        QVERIFY(spy.count() >= 2);
    }

    void goesOfflineForBadTarget() {
        MockConnectionService c;
        c.connectTo(QStringLiteral("local"), QStringLiteral("/run/daemon.sock"));
        QVERIFY(QTest::qWaitFor([&] { return c.state() == QStringLiteral("offline"); }, 3000));
        QVERIFY(!c.ready());
    }

    void embeddedModeIsGatedOffline() {
        MockConnectionService c;
        c.connectTo(QStringLiteral("embedded"), QStringLiteral("ffi"));
        QVERIFY(QTest::qWaitFor([&] { return c.state() == QStringLiteral("offline"); }, 3000));
    }

    void testConnectionReportsResult() {
        MockConnectionService c;
        QSignalSpy ok(&c, &connection::IConnectionService::testResult);
        c.testConnection(QStringLiteral("remote"), QStringLiteral("https://mock.local"));
        QVERIFY(ok.wait(2000));
        QCOMPARE(ok.takeFirst().at(0).toBool(), true);

        QSignalSpy bad(&c, &connection::IConnectionService::testResult);
        c.testConnection(QStringLiteral("remote"), QStringLiteral("https://example.invalid"));
        QVERIFY(bad.wait(2000));
        QCOMPARE(bad.takeFirst().at(0).toBool(), false);
    }

    void disconnectGoesOffline() {
        QTemporaryFile socketStandIn;
        QVERIFY(socketStandIn.open());
        MockConnectionService c;
        c.connectTo(QStringLiteral("local"), socketStandIn.fileName());
        QVERIFY(QTest::qWaitFor([&] { return c.ready(); }, 3000));
        c.disconnect();
        QCOMPARE(c.state(), QStringLiteral("offline"));
    }
};

QTEST_MAIN(TestConnectionService)
#include "tst_connection_service.moc"
