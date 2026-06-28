#include "daemon/daemon_connection_service.h"
#include "daemon/daemon_transport.h"
#include "daemon/local_daemon_launcher.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "wire_mux_fixture.h" // shared multiplexed daemon stand-in (wire L0 envelope)

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::DaemonConnectionService;
using daemonapp::daemon::DaemonTransport;
using daemonapp::daemon::LocalDaemonLauncher;
using daemonapp::daemon::NodeApiClient;
using daemonapp::daemon::NodeApiCodec;
using daemonapp::test::WireMuxServer;

namespace {

// A healthy Health response: {"Health": {"all_ok": true, "services": []}} (the same bytes the codec
// decode tests use). The fixture wraps these in a Reply to drive the connection service to "ready".
QByteArray healthResponse() {
    return {"\xA1\x66Health\xA2\x66"
            "all_ok\xF5\x68services\x80",
            27};
}

} // namespace

// WP1/WP2/WP4 of the connection-resilience phase, exercised against a real transport+client+service
// stack over a controllable QLocalServer fixture (the most honest path): the per-request timeout
// unsticks a hung daemon, the connection service self-corrects out of stale "ready" after a drop
// and reconnects, and a turn resumes from its cursor across a mid-stream drop instead of dying.
class TestConnectionResilience : public QObject {
    Q_OBJECT

private slots:
    // WP1: a connected-but-silent daemon must not stall the queue forever; the request timeout
    // fails the in-flight request (unsticking the queue) and a subsequent request still dispatches.
    void clientTimesOutHungRequest() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("hang.sock"));
        WireMuxServer fake;
        fake.setHang(true);
        QVERIFY2(fake.start(path), "fixture must listen");

        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        client.setRequestTimeoutMs(300); // shrink the 30s default so the test is fast

        QSignalSpy failures(&client, &NodeApiClient::failed);
        QVERIFY(failures.isValid());

        client.sendRequest(NodeApiCodec::encodeHealthRequest(), QStringLiteral("c1"));
        QTRY_COMPARE_WITH_TIMEOUT(failures.count(), 1, 3000);
        QCOMPARE(failures.at(0).at(0).toString(), QStringLiteral("c1"));
        QCOMPARE(failures.at(0).at(1).toString(), QStringLiteral("request timed out"));

        // The queue is unstuck: a second request dispatches and times out in turn rather than
        // stalling behind the first.
        client.sendRequest(NodeApiCodec::encodeHealthRequest(), QStringLiteral("c2"));
        QTRY_COMPARE_WITH_TIMEOUT(failures.count(), 2, 3000);
        QCOMPARE(failures.at(1).at(0).toString(), QStringLiteral("c2"));
    }

    // WP2: a mid-session transport drop flips the service out of "ready" into the reconnecting
    // episode ("connecting" + "Reconnecting..."), and bringing the daemon back auto-recovers it to
    // "ready" via the backoff Health reprobe - no user action.
    void serviceRecoversAfterTransportDrop() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("daemon.sock"));
        WireMuxServer fake;
        fake.setReplyPayload(healthResponse());
        QVERIFY2(fake.start(path), "fixture must listen");

        DaemonConnectionService svc; // attach-only (no settings -> the backoff reprobe path)
        svc.connectTo(QStringLiteral("local"), path);
        QVERIFY(QTest::qWaitFor([&] { return svc.ready(); }, 5000));

        // Drop: stopping the fixture closes the peer; the service must leave stale "ready".
        fake.stop();
        QVERIFY(QTest::qWaitFor(
            [&] { return svc.state() == QLatin1String("connecting") && !svc.ready(); }, 5000));
        QCOMPARE(svc.statusMessage(), QStringLiteral("Reconnecting..."));

        // Restore: the reprobe reconnects on its own and the service returns to ready.
        QVERIFY2(fake.start(path), "fixture must relisten");
        QVERIFY(QTest::qWaitFor([&] { return svc.ready(); }, 15000));
        QVERIFY(svc.statusMessage().isEmpty());
    }

    // (The mid-turn-drop / resume behaviour moved to tst_sync_resync, which drives the L2 push
    // subscription the turn engine now uses instead of the old 120ms Subscribe poll.)

    // WP3: the restart budget caps re-spawns within its window (pure logic, no spawning): the first
    // kMax spawns are allowed, the next is refused, and a spawn aging out of the window frees a
    // slot.
    void restartBudgetCapsRespawns() {
        QList<qint64> history;
        const qint64 base = 1'000'000;
        const qint64 window = 60'000;
        const int maxRestarts = 3;

        // Three spawns close together are allowed; the fourth inside the window is refused.
        QVERIFY(LocalDaemonLauncher::evaluateRestartBudget(history, base, window, maxRestarts));
        QVERIFY(
            LocalDaemonLauncher::evaluateRestartBudget(history, base + 10, window, maxRestarts));
        QVERIFY(
            LocalDaemonLauncher::evaluateRestartBudget(history, base + 20, window, maxRestarts));
        QVERIFY(
            !LocalDaemonLauncher::evaluateRestartBudget(history, base + 30, window, maxRestarts));

        // Once the oldest spawns age out of the window, a slot frees up again.
        QVERIFY(LocalDaemonLauncher::evaluateRestartBudget(history, base + window + 100, window,
                                                           maxRestarts));
    }
};

QTEST_GUILESS_MAIN(TestConnectionResilience)
#include "tst_connection_resilience.moc"
