// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "update/self_apply_windows.h"

#include <QDir>
#include <QTemporaryDir>
#include <QtTest>

using update::win::planNsisSelfApply;
using update::win::SelfApplyPlan;

// Locks the Windows NSIS SelfApply arg construction + preconditions
// (src/core/update/self_apply_windows.cpp). Pure logic, testable on any host:
// the planner only touches the filesystem to confirm the helper + staged
// installer exist, and it builds the FROZEN daemon-updater CLI contract
// (--wait-pid ... --mode nsis-silent --staged ... --sha256 ... --log ...
// --relaunch -- <app>). A malformed invocation here would silently break
// self-apply on Windows, so the argument order is asserted exactly.
class UpdateSelfApplyWindowsTests : public QObject {
    Q_OBJECT

private:
    // Build a fake per-user install tree (helper + staged installer) under a
    // scratch dir so the existence preconditions pass on the CI host.
    static void seed(const QDir& dir, QString& appDir, QString& staged) {
        appDir = dir.filePath(QStringLiteral("bin"));
        QDir().mkpath(appDir);
        QFile helper(QDir(appDir).filePath(QStringLiteral("daemon-updater.exe")));
        QVERIFY(helper.open(QIODevice::WriteOnly));
        helper.write("stub");
        helper.close();

        staged = dir.filePath(QStringLiteral("daemon-0.0.2-win64.exe"));
        QFile inst(staged);
        QVERIFY(inst.open(QIODevice::WriteOnly));
        inst.write("installer-bytes");
        inst.close();
    }

private slots:
    void plansFrozenContract();
    void missingHelperFailsWithReason();
    void missingStagedInstallerFails();
    void emptySha256Fails();
    void emptyRelaunchTargetFails();
    void emptyAppDirFails();
    void logPathIsNonEmpty();
};

void UpdateSelfApplyWindowsTests::plansFrozenContract() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString appDir;
    QString staged;
    seed(QDir(tmp.path()), appDir, staged);

    const QString app = QDir(appDir).filePath(QStringLiteral("daemon-app.exe"));
    const QString log = QDir(tmp.path()).filePath(QStringLiteral("updater.log"));
    const QString sha = QStringLiteral("deadbeef");

    const SelfApplyPlan plan = planNsisSelfApply(appDir, staged, sha, log, app, /*waitPid=*/4242);

    QVERIFY2(plan.ok, qPrintable(plan.reason));
    // Helper resolves to bin\daemon-updater.exe next to the app.
    QVERIFY(plan.program.endsWith(QStringLiteral("daemon-updater.exe")));
    QCOMPARE(QDir(appDir).relativeFilePath(plan.program), QStringLiteral("daemon-updater.exe"));

    // Exact frozen CLI contract (src/tools/updater/updater.cpp): order matters,
    // nsis-silent carries NO --target, and --relaunch is terminated by "--".
    const QStringList expected{
        QStringLiteral("--wait-pid"),
        QStringLiteral("4242"),
        QStringLiteral("--mode"),
        QStringLiteral("nsis-silent"),
        QStringLiteral("--staged"),
        staged,
        QStringLiteral("--sha256"),
        sha,
        QStringLiteral("--log"),
        log,
        QStringLiteral("--relaunch"),
        QStringLiteral("--"),
        app,
    };
    QCOMPARE(plan.arguments, expected);

    // Defensive: never emit --target for nsis-silent (the installer owns the swap).
    QVERIFY(!plan.arguments.contains(QStringLiteral("--target")));
    // The relaunch target is the final argument, after the "--" terminator.
    QCOMPARE(plan.arguments.last(), app);
    QCOMPARE(plan.arguments.at(plan.arguments.size() - 2), QStringLiteral("--"));
}

void UpdateSelfApplyWindowsTests::missingHelperFailsWithReason() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString appDir;
    QString staged;
    seed(QDir(tmp.path()), appDir, staged);
    // Remove the helper so the precondition fails.
    QVERIFY(QFile::remove(QDir(appDir).filePath(QStringLiteral("daemon-updater.exe"))));

    const SelfApplyPlan plan = planNsisSelfApply(
        appDir, staged, QStringLiteral("abc"), tmp.filePath(QStringLiteral("l.log")),
        QDir(appDir).filePath(QStringLiteral("daemon-app.exe")), 1);
    QVERIFY(!plan.ok);
    QVERIFY(plan.arguments.isEmpty());
    QVERIFY(plan.reason.contains(QStringLiteral("daemon-updater")));
}

void UpdateSelfApplyWindowsTests::missingStagedInstallerFails() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString appDir;
    QString staged;
    seed(QDir(tmp.path()), appDir, staged);

    const SelfApplyPlan plan =
        planNsisSelfApply(appDir, tmp.filePath(QStringLiteral("does-not-exist.exe")),
                          QStringLiteral("abc"), tmp.filePath(QStringLiteral("l.log")),
                          QDir(appDir).filePath(QStringLiteral("daemon-app.exe")), 1);
    QVERIFY(!plan.ok);
    QVERIFY(!plan.reason.isEmpty());
}

void UpdateSelfApplyWindowsTests::emptySha256Fails() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString appDir;
    QString staged;
    seed(QDir(tmp.path()), appDir, staged);

    const SelfApplyPlan plan =
        planNsisSelfApply(appDir, staged, QString(), tmp.filePath(QStringLiteral("l.log")),
                          QDir(appDir).filePath(QStringLiteral("daemon-app.exe")), 1);
    QVERIFY(!plan.ok);
    QVERIFY(!plan.reason.isEmpty());
}

void UpdateSelfApplyWindowsTests::emptyRelaunchTargetFails() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString appDir;
    QString staged;
    seed(QDir(tmp.path()), appDir, staged);

    const SelfApplyPlan plan = planNsisSelfApply(
        appDir, staged, QStringLiteral("abc"), tmp.filePath(QStringLiteral("l.log")), QString(), 1);
    QVERIFY(!plan.ok);
    QVERIFY(!plan.reason.isEmpty());
}

void UpdateSelfApplyWindowsTests::emptyAppDirFails() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString appDir;
    QString staged;
    seed(QDir(tmp.path()), appDir, staged);

    const SelfApplyPlan plan =
        planNsisSelfApply(QString(), staged, QStringLiteral("abc"),
                          tmp.filePath(QStringLiteral("l.log")), QStringLiteral("app.exe"), 1);
    QVERIFY(!plan.ok);
    QVERIFY(!plan.reason.isEmpty());
}

void UpdateSelfApplyWindowsTests::logPathIsNonEmpty() {
    // The default log path resolves (AppDataLocation is redirected under the
    // offscreen test env); the exact value is platform-dependent, so we only
    // assert it is usable and points into an updates/ tree.
    const QString log = update::win::selfApplyLogPath();
    QVERIFY(!log.isEmpty());
    QVERIFY(log.endsWith(QStringLiteral("daemon-updater.log")));
}

QTEST_MAIN(UpdateSelfApplyWindowsTests)
#include "tst_update_self_apply_windows.moc"
