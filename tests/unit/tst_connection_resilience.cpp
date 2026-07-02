// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "connection/iconnection_service.h"
#include "daemon/daemon_connection_service.h"
#include "daemon/daemon_transport.h"
#include "daemon/local_daemon_launcher.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "settings/isettings_store.h"
#include "wire_mux_fixture.h" // shared multiplexed daemon stand-in (wire L0 envelope)

#include <QFile>
#include <QProcess>
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

// A memory-only ISettingsStore so the managed-daemon paths (managedLocalDaemon defaults ON) run
// without touching the user's real QSettings.
class MemorySettingsStore : public settings::ISettingsStore {
public:
    using settings::ISettingsStore::ISettingsStore;
    [[nodiscard]] QVariant value(const QString& key, const QVariant& fallback = {}) const override {
        return m_values.value(key, fallback);
    }
    void setValue(const QString& key, const QVariant& value) override {
        m_values.insert(key, value);
        emit changed(key);
        emit changedAny();
    }

private:
    QHash<QString, QVariant> m_values;
};

// The Hello features of a stale daemon speaking an older api contract than this client.
QStringList staleDaemonFeatures() {
    return {QStringLiteral("mux"), QStringLiteral("stream"),
            QStringLiteral("api/") + QString::number(NodeApiCodec::kDaemonApiVersion - 1)};
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

    // Auth: a node that requires SASL drives the service to "authenticating" (no credentials yet),
    // and a subsequent login() with the right password reaches "ready". The shell never mounts
    // pre-auth because ready() stays false until AuthOk lands.
    void authRequiredThenLoginReachesReady() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("auth.sock"));
        WireMuxServer fake;
        fake.setReplyPayload(healthResponse());
        fake.setAuthMechanisms({QStringLiteral("SCRAM-SHA-256")});
        fake.setCredentials("user", "pencil");
        QVERIFY2(fake.start(path), "fixture must listen");

        DaemonConnectionService svc; // attach-only (no settings)
        QSignalSpy authReq(&svc, &connection::IConnectionService::authRequired);
        QSignalSpy authedOk(&svc, &connection::IConnectionService::authenticated);

        svc.connectTo(QStringLiteral("local"), path);
        // No credentials: the service surfaces the login form and never goes ready.
        QVERIFY(
            QTest::qWaitFor([&] { return svc.state() == QLatin1String("authenticating"); }, 5000));
        QVERIFY(authReq.count() >= 1);
        QVERIFY(!svc.ready());

        // Correct password -> SCRAM -> AuthOk -> Health -> ready.
        svc.login(QStringLiteral("user"), QStringLiteral("pencil"));
        QVERIFY(QTest::qWaitFor([&] { return svc.ready(); }, 5000));
        QVERIFY(authedOk.count() >= 1);
        QCOMPARE(svc.client()->principal().username, QStringLiteral("user"));
    }

    // Wrong password stays disconnected with an error, and the password is NEVER written to any log
    // (qDebug/qInfo/qWarning) nor to the surfaced status message.
    void wrongPasswordStaysDisconnectedAndNeverLogsSecret() {
        static QStringList captured;
        captured.clear();
        QtMessageHandler prev = qInstallMessageHandler(
            [](QtMsgType, const QMessageLogContext&, const QString& msg) { captured.append(msg); });

        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("authbad.sock"));
        WireMuxServer fake;
        fake.setReplyPayload(healthResponse());
        fake.setAuthMechanisms({QStringLiteral("SCRAM-SHA-256")});
        fake.setCredentials("user", "pencil");
        QVERIFY2(fake.start(path), "fixture must listen");

        DaemonConnectionService svc;
        QSignalSpy failedAuth(&svc, &connection::IConnectionService::authFailed);
        svc.connectTo(QStringLiteral("local"), path);
        QVERIFY(
            QTest::qWaitFor([&] { return svc.state() == QLatin1String("authenticating"); }, 5000));

        const QString secret = QStringLiteral("hunter2-secret-pw");
        svc.login(QStringLiteral("user"), secret);
        QVERIFY(QTest::qWaitFor([&] { return failedAuth.count() >= 1; }, 5000));
        QVERIFY(!svc.ready());

        qInstallMessageHandler(prev);
        for (const QString& line : captured) {
            QVERIFY2(!line.contains(secret), qPrintable("secret leaked to log: " + line));
        }
        QVERIFY2(!svc.statusMessage().contains(secret), "secret leaked to status message");
    }

    // Version gate, attach mode: a daemon advertising an older api contract (or none at all) is
    // refused with an explicit user-visible error and the service never reaches "ready" - the
    // silent stale-attach incident this gate exists for.
    void attachRefusesIncompatibleDaemon() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        // Older contract ("api/<N-1>").
        const QString stalePath = tmp.filePath(QStringLiteral("stale.sock"));
        WireMuxServer stale;
        stale.setReplyPayload(healthResponse());
        stale.setHelloFeatures(staleDaemonFeatures());
        QVERIFY2(stale.start(stalePath), "fixture must listen");

        DaemonConnectionService svc; // attach-only (no settings -> no launcher)
        svc.connectTo(QStringLiteral("local"), stalePath);
        QVERIFY(QTest::qWaitFor([&] { return svc.state() == QLatin1String("offline"); }, 5000));
        QVERIFY2(svc.statusMessage().contains(QStringLiteral("Incompatible daemon")),
                 qPrintable("unexpected status: " + svc.statusMessage()));
        QVERIFY(!svc.ready());
        // The buffered Health Reply must never flip the service to ready after the gate closed
        // the connection.
        QTest::qWait(250);
        QVERIFY(!svc.ready());

        // No advertisement at all (a pre-"api/<N>" daemon) counts as a mismatch too.
        const QString oldPath = tmp.filePath(QStringLiteral("preapi.sock"));
        WireMuxServer old;
        old.setReplyPayload(healthResponse());
        old.setHelloFeatures({QStringLiteral("mux"), QStringLiteral("stream")});
        QVERIFY2(old.start(oldPath), "fixture must listen");

        DaemonConnectionService svc2;
        svc2.connectTo(QStringLiteral("local"), oldPath);
        QVERIFY(QTest::qWaitFor([&] { return svc2.state() == QLatin1String("offline"); }, 5000));
        QVERIFY(svc2.statusMessage().contains(QStringLiteral("Incompatible daemon")));
        QVERIFY(svc2.statusMessage().contains(QStringLiteral("unknown")));
        QVERIFY(!svc2.ready());
    }

    // Version gate, managed mode, no pidfile: an incompatible daemon on the managed socket that no
    // app instance recorded (pre-pidfile spawn) is never killed blindly - the user gets a clear
    // error naming the stale socket.
    void managedIncompatibleWithoutPidfileSurfacesError() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("managed.sock"));
        WireMuxServer fake;
        fake.setReplyPayload(healthResponse());
        fake.setHelloFeatures(staleDaemonFeatures());
        QVERIFY2(fake.start(path), "fixture must listen");

        MemorySettingsStore settings;
        DaemonConnectionService svc(nullptr, &settings); // managedLocalDaemon defaults ON
        svc.connectTo(QStringLiteral("local"), path);

        QVERIFY(QTest::qWaitFor([&] { return svc.state() == QLatin1String("offline"); }, 5000));
        QVERIFY2(svc.statusMessage().contains(QStringLiteral("no pidfile")),
                 qPrintable("unexpected status: " + svc.statusMessage()));
        QVERIFY(svc.statusMessage().contains(path)); // the user learns WHICH socket is stale
        QVERIFY(!svc.ready());
    }

    // Version gate, managed mode, pidfile present: the recorded stale daemon is SIGTERMed via its
    // pidfile (spawned by "a previous app instance" - here a stand-in process), and the respawn is
    // bounded to ONCE: the fixture keeps serving the old contract after the kill, so the second
    // mismatch must land on the explicit error instead of a terminate/respawn loop.
    void managedReplacesViaPidfileAndBoundsRespawn() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("managed2.sock"));
        WireMuxServer fake;
        fake.setReplyPayload(healthResponse());
        fake.setHelloFeatures(staleDaemonFeatures());
        QVERIFY2(fake.start(path), "fixture must listen");

        // The "stale daemon" process the pidfile records (the fixture serves its socket).
        QProcess staleDaemon;
        staleDaemon.start(QStringLiteral("sleep"), {QStringLiteral("300")});
        QVERIFY2(staleDaemon.waitForStarted(5000), "sleep helper must start");
        const qint64 stalePid = staleDaemon.processId();
        {
            QFile pidfile(LocalDaemonLauncher::pidFilePath(path));
            QVERIFY(pidfile.open(QIODevice::WriteOnly));
            pidfile.write(QByteArray::number(stalePid));
        }

        MemorySettingsStore settings;
        DaemonConnectionService svc(nullptr, &settings);
        svc.connectTo(QStringLiteral("local"), path);

        // The gate terminates the recorded pid (never by name/pattern - exactly this pid).
        QVERIFY(QTest::qWaitFor([&] { return staleDaemon.state() == QProcess::NotRunning; }, 5000));
        // The launcher re-attaches to the still-incompatible fixture; the once-only budget turns
        // the second mismatch into the explicit attach error, not another kill/respawn round.
        QVERIFY(QTest::qWaitFor([&] { return svc.state() == QLatin1String("offline"); }, 10000));
        QVERIFY2(svc.statusMessage().contains(QStringLiteral("Incompatible daemon")),
                 qPrintable("unexpected status: " + svc.statusMessage()));
        QVERIFY(!svc.ready());
        QVERIFY(!QFile::exists(LocalDaemonLauncher::pidFilePath(path))); // spent pidfile cleaned
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
