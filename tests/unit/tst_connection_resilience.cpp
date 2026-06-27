#include "daemon/daemon_connection_service.h"
#include "daemon/daemon_transport.h"
#include "daemon/local_daemon_launcher.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon_turn_engine.h" // daemon-app::turn exposes its include dir

#include <QHash>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::DaemonConnectionService;
using daemonapp::daemon::DaemonTransport;
using daemonapp::daemon::LocalDaemonLauncher;
using daemonapp::daemon::NodeApiClient;
using daemonapp::daemon::NodeApiCodec;

namespace {

// A healthy Health response: {"Health": {"all_ok": true, "services": []}} (the same bytes the codec
// decode tests use). The fixture replies these to drive the connection service to "ready".
QByteArray healthResponse() {
    return {"\xA1\x66Health\xA2\x66"
            "all_ok\xF5\x68services\x80",
            27};
}

// An empty-but-valid LogPage: {"LogPage": {"entries": [], "next_seq": 5, "head_seq": 5}}. An empty
// page (no TurnFinished) keeps a DaemonTurnEngine polling, so the turn stays in-flight.
QByteArray logPageResponse() {
    return {"\xA1\x67LogPage\xA3\x67"
            "entries\x80\x68next_seq\x05\x68head_seq\x05",
            39};
}

// A controllable stand-in for the daemon host: it speaks the length-prefixed frame protocol over a
// QLocalServer and answers every complete request frame with a fixed payload - or, when set to
// hang, accepts the frame and never replies (the silent-daemon case the request timeout guards).
class FakeDaemon : public QObject {
    Q_OBJECT

public:
    bool start(const QString& path) {
        m_path = path;
        QLocalServer::removeServer(path); // clear a stale socket file from a prior listen
        m_server = new QLocalServer(this);
        connect(m_server, &QLocalServer::newConnection, this, &FakeDaemon::onNewConnection);
        return m_server->listen(path);
    }

    void stop() {
        for (QLocalSocket* conn : std::as_const(m_conns)) {
            conn->abort();
            conn->deleteLater();
        }
        m_conns.clear();
        m_buffers.clear();
        if (m_server != nullptr) {
            m_server->close();
            m_server->deleteLater();
            m_server = nullptr;
        }
    }

    void setReplyPayload(const QByteArray& payload) { m_payload = payload; }
    void setHang(bool hang) { m_hang = hang; }
    [[nodiscard]] int requestCount() const { return m_requestCount; }

private slots:
    void onNewConnection() {
        while (m_server != nullptr && m_server->hasPendingConnections()) {
            QLocalSocket* conn = m_server->nextPendingConnection();
            m_conns.append(conn);
            connect(conn, &QLocalSocket::readyRead, this, [this, conn] { onReadyRead(conn); });
        }
    }

private:
    void onReadyRead(QLocalSocket* conn) {
        m_buffers[conn].append(conn->readAll());
        QByteArray request;
        while (DaemonTransport::tryTakeFrame(m_buffers[conn], &request)) {
            ++m_requestCount;
            if (m_hang) {
                continue; // accept-but-never-reply: leave the request in flight forever
            }
            conn->write(DaemonTransport::framePayload(m_payload));
            conn->flush();
        }
    }

    QLocalServer* m_server = nullptr;
    QList<QLocalSocket*> m_conns;
    QHash<QLocalSocket*, QByteArray> m_buffers;
    QByteArray m_payload;
    bool m_hang = false;
    int m_requestCount = 0;
    QString m_path;
};

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
        FakeDaemon fake;
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
        FakeDaemon fake;
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

    // WP4: a Subscribe failure mid-turn must NOT finish the turn; the engine suspends into
    // "stalled" and retries from its cursor, recovering when the daemon returns.
    void turnResumesAfterMidStreamDrop() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("turn.sock"));
        FakeDaemon fake;
        fake.setReplyPayload(logPageResponse()); // empty pages keep the turn polling
        QVERIFY2(fake.start(path), "fixture must listen");

        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonTurnEngine engine(&client);
        engine.setSessionId(QStringLiteral("s1"));

        engine.start(QStringLiteral("hello"));
        // Wait until the turn is past Submit and actively polling Subscribe (>= 2 frames: the
        // StartTurn submit + at least one Subscribe), so the drop below lands mid-stream rather
        // than in the Submit phase (a Submit failure is correctly unrecoverable, not what this
        // guards).
        QVERIFY(QTest::qWaitFor([&] { return engine.active() && fake.requestCount() >= 2; }, 5000));

        // Drop mid-turn: the engine must suspend into "stalled", not finish.
        fake.stop();
        QVERIFY(
            QTest::qWaitFor([&] { return engine.turnState() == QLatin1String("stalled"); }, 5000));
        QVERIFY(engine.active()); // still in-flight on the daemon - resumable

        // Restore: the resume backoff re-Subscribes from the cursor and the turn recovers.
        QVERIFY2(fake.start(path), "fixture must relisten");
        QVERIFY(
            QTest::qWaitFor([&] { return engine.turnState() == QLatin1String("thinking"); }, 8000));
        QVERIFY(engine.active());

        engine.cancel();
    }

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
