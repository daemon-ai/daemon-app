// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "update/self_apply_windows.h"

#include <QDir>
#include <QFileInfo>
#include <QObject>
#include <QProcess>
#include <QStandardPaths>

namespace update::win {

namespace {

// The helper is packaged as bin/daemon-updater.exe next to daemon-app.exe
// (install(TARGETS daemon-updater RUNTIME) -> ${CMAKE_INSTALL_BINDIR}). The
// name is fixed by the target; the .exe suffix is the Windows convention this
// backend targets (a Linux unit test creates a matching stub).
constexpr QLatin1StringView kHelperName("daemon-updater.exe");

} // namespace

SelfApplyPlan planNsisSelfApply(const QString& appDir, const QString& stagedInstaller,
                                const QString& sha256Hex, const QString& logPath,
                                const QString& relaunchTarget, qint64 waitPid) {
    SelfApplyPlan plan;

    if (appDir.isEmpty()) {
        plan.reason = QObject::tr("application directory is unknown");
        return plan;
    }
    const QString helper = QDir(appDir).filePath(kHelperName);
    if (!QFileInfo(helper).isFile()) {
        plan.reason = QObject::tr("update helper (daemon-updater) not found next to the app");
        return plan;
    }
    if (stagedInstaller.isEmpty() || !QFileInfo(stagedInstaller).isFile()) {
        plan.reason = QObject::tr("no staged installer to run");
        return plan;
    }
    if (sha256Hex.isEmpty()) {
        plan.reason = QObject::tr("missing installer checksum");
        return plan;
    }
    if (relaunchTarget.isEmpty()) {
        plan.reason = QObject::tr("relaunch target (installed app) is unknown");
        return plan;
    }

    plan.program = helper;
    // Frozen helper CLI contract (src/tools/updater/updater.cpp). nsis-silent
    // takes no --target: the installer performs the in-use file swap itself.
    plan.arguments = QStringList{
        QStringLiteral("--wait-pid"),
        QString::number(waitPid),
        QStringLiteral("--mode"),
        QStringLiteral("nsis-silent"),
        QStringLiteral("--staged"),
        stagedInstaller,
        QStringLiteral("--sha256"),
        sha256Hex,
        QStringLiteral("--log"),
        logPath,
        QStringLiteral("--relaunch"),
        QStringLiteral("--"),
        relaunchTarget,
    };
    plan.ok = true;
    return plan;
}

QString selfApplyLogPath() {
    // <AppDataLocation>/updates/daemon-updater.log — the updates/ dir already
    // exists once a download has staged (UpdateDownloader::stagingDir). Fall
    // back to a bare name if the location cannot be resolved.
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) {
        return QStringLiteral("daemon-updater.log");
    }
    QDir(base).mkpath(QStringLiteral("updates"));
    return QDir(base).filePath(QStringLiteral("updates/daemon-updater.log"));
}

bool launchDetached(const SelfApplyPlan& plan) {
    if (!plan.ok) {
        return false;
    }
    // Fully detached: the child must outlive us so its wait-pid can observe our
    // exit, then run the installer /S and relaunch. No working directory tie to
    // the install tree (the installer rewrites it).
    return QProcess::startDetached(plan.program, plan.arguments);
}

} // namespace update::win
