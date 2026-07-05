// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "update/self_apply_appimage.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRandomGenerator>
#include <QStringList>

namespace update {

namespace {

// $APPIMAGE, canonicalized. The AppImage runtime exports the absolute path, but
// it can be relative when the app was launched by a relative name; canonicalize
// so the swap target is never the ephemeral /tmp/.mount_* path and survives a cwd
// change. Empty when unset or the file is gone.
QString canonicalAppImagePath() {
    const QString raw = qEnvironmentVariable("APPIMAGE");
    if (raw.isEmpty()) {
        return {};
    }
    return QFileInfo(raw).canonicalFilePath();
}

// Owner-executable (plus the read/write the file already carries), so a freshly
// copied helper / staged image is launchable. The helper also chmod +x's the
// target after the swap; this makes the staged copy runnable in its own right.
void makeExecutable(const QString& path) {
    const QFileDevice::Permissions perms = QFile::permissions(path);
    QFile::setPermissions(path, perms | QFileDevice::ExeOwner | QFileDevice::ExeGroup |
                                    QFileDevice::ExeOther);
}

} // namespace

bool runningAsAppImage() {
    return !canonicalAppImagePath().isEmpty();
}

AppImageApplyPreparation prepareAppImageApply(const QString& downloadedImagePath,
                                              const QString& sha256Hex) {
    AppImageApplyPreparation prep;

    // 1. The swap target: canonical $APPIMAGE.
    prep.targetImagePath = canonicalAppImagePath();
    if (prep.targetImagePath.isEmpty()) {
        prep.reason = QStringLiteral("not running as an AppImage");
        return prep;
    }
    prep.sha256 = sha256Hex;

    // 2. The verified download must exist (the caller only reaches here in
    // ReadyToApply, but stay defensive).
    if (downloadedImagePath.isEmpty() || !QFileInfo::exists(downloadedImagePath)) {
        prep.reason = QStringLiteral("the downloaded image is missing");
        return prep;
    }

    // 3. Locate the helper INSIDE the mount (next to the running binary) - it
    // vanishes when we exit, so it must be copied out before we spawn it.
    const QString helperSrc =
        QCoreApplication::applicationDirPath() + QStringLiteral("/daemon-updater");
    if (!QFileInfo::exists(helperSrc)) {
        prep.reason = QStringLiteral("update helper not found in this build");
        return prep;
    }

    // 4. A private staging dir on the TARGET filesystem (a sibling of $APPIMAGE,
    // so an atomic rename can never cross a mount). If the AppImage's own
    // directory is not writable we cannot self-apply - degrade.
    const QDir targetDir = QFileInfo(prep.targetImagePath).absoluteDir();
    const QString stagingName =
        QStringLiteral(".daemon-update-%1-%2")
            .arg(QCoreApplication::applicationPid())
            .arg(QRandomGenerator::global()->generate(), 8, 16, QLatin1Char('0'));
    if (!targetDir.mkdir(stagingName)) {
        prep.reason = QStringLiteral("the AppImage directory is not writable");
        return prep;
    }
    prep.stagingDir = targetDir.filePath(stagingName);
    const QDir stagingDir(prep.stagingDir);
    // Owner-only, defensively (world-writable staging would be an apply-time TOCTOU).
    QFile::setPermissions(prep.stagingDir,
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);

    // 5. Copy the helper out of the mount into the staging dir.
    prep.helperCopyPath = stagingDir.filePath(QStringLiteral("daemon-updater"));
    if (!QFile::copy(helperSrc, prep.helperCopyPath)) {
        QDir(prep.stagingDir).removeRecursively();
        prep.stagingDir.clear();
        prep.reason = QStringLiteral("could not copy the update helper out of the image");
        return prep;
    }
    makeExecutable(prep.helperCopyPath);

    // 6. Copy the verified image onto the target filesystem (the download's own
    // staging area may be a different mount, which a rename cannot cross).
    prep.stagedImagePath =
        stagingDir.filePath(QStringLiteral("new-") + QFileInfo(prep.targetImagePath).fileName());
    if (!QFile::copy(downloadedImagePath, prep.stagedImagePath)) {
        QDir(prep.stagingDir).removeRecursively();
        prep.stagingDir.clear();
        prep.reason = QStringLiteral("could not stage the new image next to the current one");
        return prep;
    }
    makeExecutable(prep.stagedImagePath);

    prep.logPath = stagingDir.filePath(QStringLiteral("apply.log"));
    prep.ok = true;
    return prep;
}

bool spawnAppImageHelper(const AppImageApplyPreparation& prep, qint64 appPid) {
    if (!prep.ok) {
        return false;
    }
    const QStringList args = {
        QStringLiteral("--wait-pid"), QString::number(appPid),
        QStringLiteral("--mode"),     QStringLiteral("rename-swap"),
        QStringLiteral("--staged"),   prep.stagedImagePath,
        QStringLiteral("--target"),   prep.targetImagePath,
        QStringLiteral("--sha256"),   prep.sha256,
        QStringLiteral("--log"),      prep.logPath,
        QStringLiteral("--relaunch"), QStringLiteral("--"),
        prep.targetImagePath,
    };
    // Fully detached: the helper must outlive us (it waits for our PID to exit,
    // then swaps + relaunches). It inherits our environment (e.g.
    // APPIMAGE_EXTRACT_AND_RUN) so the relaunched image starts the same way.
    return QProcess::startDetached(prep.helperCopyPath, args);
}

void cleanupAppImageStaging() {
    const QString target = canonicalAppImagePath();
    if (target.isEmpty()) {
        return;
    }
    QDir dir = QFileInfo(target).absoluteDir();
    const QList<QFileInfo> leftovers = dir.entryInfoList(
        {QStringLiteral(".daemon-update-*")}, QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);
    for (const QFileInfo& entry : leftovers) {
        QDir(entry.absoluteFilePath()).removeRecursively();
    }
}

} // namespace update
