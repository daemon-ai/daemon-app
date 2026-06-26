#include "daemon/local_daemon_launcher.h"

#include <QCoreApplication>
#include <QLocalServer>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::LocalDaemonLauncher;

// Guards the managed local-daemon launcher's pure pieces (CON-1b): binary discovery precedence and
// the liveness probe. Spawning + the poll loop are exercised end-to-end by the system-tests suite.
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
};

QTEST_GUILESS_MAIN(TestLocalDaemonLauncher)
#include "tst_local_daemon_launcher.moc"
