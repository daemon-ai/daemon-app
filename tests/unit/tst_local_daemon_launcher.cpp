// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/local_daemon_launcher.h"

#include <QCoreApplication>
#include <QFile>
#include <QLocalServer>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::LocalDaemonLauncher;

// Guards the managed local-daemon launcher's pure pieces (CON-1b): binary discovery precedence,
// the liveness probe, and the pidfile ownership seam (any app instance can manage a daemon a
// previous instance spawned). Spawning + the poll loop are exercised end-to-end by the
// system-tests suite.
class TestLocalDaemonLauncher : public QObject {
    Q_OBJECT

private slots:
    // An explicit override that resolves to an executable wins and is returned absolute.
    void discoverHonorsExistingOverride() {
        const QString selfExe = QCoreApplication::applicationFilePath();
        QCOMPARE(LocalDaemonLauncher::discoverDaemonBinary(selfExe), selfExe);
    }

    // An explicit override that does not resolve yields empty (no silent fallback to a different
    // binary): an operator who pinned a path expects that one or an actionable error.
    void discoverRejectsMissingOverride() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString missing = tmp.filePath(QStringLiteral("definitely-not-here"));
        QVERIFY(LocalDaemonLauncher::discoverDaemonBinary(missing).isEmpty());
    }

    // No server listening on a fresh path: the probe reports not-listening promptly.
    void probeFalseWhenNothingListens() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("absent.sock"));
        QVERIFY(!LocalDaemonLauncher::isDaemonListening(path, 200));
    }

    // A real local server on the path: the probe connects and reports listening.
    void probeTrueWhenServerListens() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("present.sock"));
        QLocalServer server;
        QVERIFY2(server.listen(path), qPrintable(server.errorString()));
        QVERIFY(LocalDaemonLauncher::isDaemonListening(path, 500));
    }

    // The pidfile lives in the managed socket's own directory (the same runtime-dir family), so
    // every app instance targeting that socket resolves the same ownership record.
    void pidFilePathSitsNextToSocket() {
        QCOMPARE(
            LocalDaemonLauncher::pidFilePath(QStringLiteral("/run/user/1000/daemon/daemon.sock")),
            QStringLiteral("/run/user/1000/daemon/daemon.pid"));
    }

    // kill(pid,0) liveness: this process is alive; pid 0/negatives are inert (a corrupt pidfile
    // must never target the process group).
    void processLivenessProbe() {
        QVERIFY(LocalDaemonLauncher::isProcessAlive(QCoreApplication::applicationPid()));
        QVERIFY(!LocalDaemonLauncher::isProcessAlive(0));
        QVERIFY(!LocalDaemonLauncher::isProcessAlive(-7));
    }

    // readManagedPid returns a LIVE recorded pid, keeps its pidfile, and cleans anything stale:
    // a dead process, garbage content, or a missing file all yield 0.
    void readManagedPidChecksLivenessAndCleansStale() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString sock = tmp.filePath(QStringLiteral("daemon.sock"));
        const QString pidPath = LocalDaemonLauncher::pidFilePath(sock);

        // Live: this test process itself.
        {
            QFile f(pidPath);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(QByteArray::number(QCoreApplication::applicationPid()));
        }
        QCOMPARE(LocalDaemonLauncher::readManagedPid(sock),
                 qint64(QCoreApplication::applicationPid()));
        QVERIFY(QFile::exists(pidPath)); // a live record is kept

        // Dead: a helper child that already exited.
        QProcess helper;
        helper.start(QStringLiteral("true"), {});
        QVERIFY2(helper.waitForStarted(5000), "helper must start");
        const qint64 deadPid = helper.processId();
        QVERIFY(QTest::qWaitFor([&] { return helper.state() == QProcess::NotRunning; }, 5000));
        {
            QFile f(pidPath);
            QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
            f.write(QByteArray::number(deadPid));
        }
        QCOMPARE(LocalDaemonLauncher::readManagedPid(sock), qint64(0));
        QVERIFY(!QFile::exists(pidPath)); // the stale record was cleaned

        // Garbage content is inert and cleaned too.
        {
            QFile f(pidPath);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("not-a-pid");
        }
        QCOMPARE(LocalDaemonLauncher::readManagedPid(sock), qint64(0));
        QVERIFY(!QFile::exists(pidPath));

        // No pidfile at all.
        QCOMPARE(LocalDaemonLauncher::readManagedPid(sock), qint64(0));
    }
};

QTEST_GUILESS_MAIN(TestLocalDaemonLauncher)
#include "tst_local_daemon_launcher.moc"
