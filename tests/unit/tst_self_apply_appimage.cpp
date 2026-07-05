// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "update/self_apply_appimage.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

using update::AppImageApplyPreparation;
using update::cleanupAppImageStaging;
using update::prepareAppImageApply;
using update::runningAsAppImage;

// The AppImage self-apply staging/precondition logic (src/core/update/
// self_apply_appimage). prepareAppImageApply is the fail-closed gate the apply()
// dispatch relies on: every precondition failure returns ok=false + a reason so
// the caller degrades to DownloadAndOpen. The spawn (QProcess::startDetached) and
// the actual swap are proven end-to-end by the nix A->B E2E, not here.
class SelfApplyAppImageTests : public QObject {
    Q_OBJECT

private:
    // A stub "daemon-updater" beside the test binary, so prepare() finds a helper
    // to copy out (the real one lives in <mount>/usr/bin next to daemon-app).
    QString helperStub() const {
        return QFileInfo(QCoreApplication::applicationFilePath()).absolutePath() +
               QStringLiteral("/daemon-updater");
    }

    void writeStubHelper() {
        QFile file(helperStub());
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("#!/bin/sh\nexit 0\n");
        file.close();
    }

    static QString sha256Of(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            return {};
        }
        QCryptographicHash hash(QCryptographicHash::Sha256);
        hash.addData(&file);
        return QString::fromLatin1(hash.result().toHex());
    }

private slots:
    void cleanup() {
        // Each test owns its APPIMAGE + helper stub state.
        qunsetenv("APPIMAGE");
        QFile::remove(helperStub());
    }

    void notAnAppImageIsInert();
    void missingDownloadDegrades();
    void missingHelperDegrades();
    void unwritableTargetDirDegrades();
    void happyPathStagesEverything();
    void cleanupRemovesLeftoverStagingDirs();
};

void SelfApplyAppImageTests::notAnAppImageIsInert() {
    qunsetenv("APPIMAGE");
    QVERIFY(!runningAsAppImage());

    const AppImageApplyPreparation prep =
        prepareAppImageApply(QStringLiteral("/does/not/matter"), QStringLiteral("deadbeef"));
    QVERIFY(!prep.ok);
    QVERIFY(prep.reason.contains(QStringLiteral("AppImage")));
    QVERIFY(prep.stagingDir.isEmpty()); // nothing was created
}

void SelfApplyAppImageTests::missingDownloadDegrades() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString target = dir.filePath(QStringLiteral("daemon-app.AppImage"));
    QFile targetFile(target);
    QVERIFY(targetFile.open(QIODevice::WriteOnly));
    targetFile.write("OLD");
    targetFile.close();
    qputenv("APPIMAGE", target.toUtf8());
    writeStubHelper();

    QVERIFY(runningAsAppImage());
    const AppImageApplyPreparation prep =
        prepareAppImageApply(dir.filePath(QStringLiteral("nope.AppImage")), QStringLiteral("ab"));
    QVERIFY(!prep.ok);
    QVERIFY(prep.reason.contains(QStringLiteral("missing")));
}

void SelfApplyAppImageTests::missingHelperDegrades() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString target = dir.filePath(QStringLiteral("daemon-app.AppImage"));
    QFile targetFile(target);
    QVERIFY(targetFile.open(QIODevice::WriteOnly));
    targetFile.write("OLD");
    targetFile.close();
    const QString download = dir.filePath(QStringLiteral("new.AppImage"));
    QFile downloadFile(download);
    QVERIFY(downloadFile.open(QIODevice::WriteOnly));
    downloadFile.write("NEW");
    downloadFile.close();
    qputenv("APPIMAGE", target.toUtf8());
    QFile::remove(helperStub()); // no helper beside the binary

    const AppImageApplyPreparation prep = prepareAppImageApply(download, sha256Of(download));
    QVERIFY(!prep.ok);
    QVERIFY(prep.reason.contains(QStringLiteral("helper")));
}

void SelfApplyAppImageTests::unwritableTargetDirDegrades() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString subdir = dir.filePath(QStringLiteral("ro"));
    QVERIFY(QDir().mkpath(subdir));
    const QString target = subdir + QStringLiteral("/daemon-app.AppImage");
    QFile targetFile(target);
    QVERIFY(targetFile.open(QIODevice::WriteOnly));
    targetFile.write("OLD");
    targetFile.close();
    const QString download = dir.filePath(QStringLiteral("new.AppImage"));
    QFile downloadFile(download);
    QVERIFY(downloadFile.open(QIODevice::WriteOnly));
    downloadFile.write("NEW");
    downloadFile.close();
    qputenv("APPIMAGE", target.toUtf8());
    writeStubHelper();

    // Make the AppImage's directory read-only so the staging mkdir fails.
    QVERIFY(QFile::setPermissions(subdir, QFileDevice::ReadOwner | QFileDevice::ExeOwner));
    const AppImageApplyPreparation prep = prepareAppImageApply(download, sha256Of(download));
    // Restore perms so QTemporaryDir can clean up.
    QFile::setPermissions(subdir,
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    if (prep.ok) {
        // A privileged runner (root) can write through 0500; the writability guard
        // is genuinely bypassed there, so only assert the degrade off-root.
        QSKIP("target dir is writable despite 0500 (running privileged)");
    }
    QVERIFY(prep.reason.contains(QStringLiteral("writable")));
}

void SelfApplyAppImageTests::happyPathStagesEverything() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString target = dir.filePath(QStringLiteral("daemon-app.AppImage"));
    QFile targetFile(target);
    QVERIFY(targetFile.open(QIODevice::WriteOnly));
    targetFile.write("OLD-IMAGE");
    targetFile.close();

    // The downloaded image lives in a SEPARATE dir (mimicking AppDataLocation,
    // possibly a different filesystem): prepare must copy it onto the target's fs.
    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());
    const QString download = downloadDir.filePath(QStringLiteral("daemon-app-2.AppImage"));
    QFile downloadFile(download);
    QVERIFY(downloadFile.open(QIODevice::WriteOnly));
    const QByteArray payload = QByteArrayLiteral("NEW-IMAGE-BYTES");
    downloadFile.write(payload);
    downloadFile.close();
    const QString sha = sha256Of(download);

    qputenv("APPIMAGE", target.toUtf8());
    writeStubHelper();

    const AppImageApplyPreparation prep = prepareAppImageApply(download, sha);
    QVERIFY2(prep.ok, qPrintable(prep.reason));
    QCOMPARE(prep.targetImagePath, QFileInfo(target).canonicalFilePath());
    QCOMPARE(prep.sha256, sha);

    // The helper was copied OUT of the "mount" and is executable.
    QVERIFY(QFile::exists(prep.helperCopyPath));
    QVERIFY(QFileInfo(prep.helperCopyPath).isExecutable());

    // The new image was staged on the target's filesystem (same directory tree),
    // byte-identical, and executable.
    QVERIFY(QFile::exists(prep.stagedImagePath));
    QVERIFY(QFileInfo(prep.stagedImagePath).isExecutable());
    QFile staged(prep.stagedImagePath);
    QVERIFY(staged.open(QIODevice::ReadOnly));
    QCOMPARE(staged.readAll(), payload);
    staged.close();

    // The staging dir is a sibling of the target (same filesystem for the atomic
    // rename), and both staged artifacts live inside it.
    const QString targetDir = QFileInfo(target).absolutePath();
    QCOMPARE(QFileInfo(prep.stagingDir).absolutePath(), QFileInfo(targetDir).absoluteFilePath());
    QVERIFY(prep.stagedImagePath.startsWith(prep.stagingDir));
    QVERIFY(prep.helperCopyPath.startsWith(prep.stagingDir));
    QVERIFY(prep.logPath.startsWith(prep.stagingDir));

    // cleanupAppImageStaging removes the leftover staging dir on a later boot.
    QVERIFY(QDir(prep.stagingDir).exists());
    cleanupAppImageStaging();
    QVERIFY(!QDir(prep.stagingDir).exists());
}

void SelfApplyAppImageTests::cleanupRemovesLeftoverStagingDirs() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString target = dir.filePath(QStringLiteral("daemon-app.AppImage"));
    QFile targetFile(target);
    QVERIFY(targetFile.open(QIODevice::WriteOnly));
    targetFile.write("OLD");
    targetFile.close();
    qputenv("APPIMAGE", target.toUtf8());

    QDir base(dir.path());
    QVERIFY(base.mkdir(QStringLiteral(".daemon-update-111-abc")));
    QVERIFY(base.mkdir(QStringLiteral(".daemon-update-222-def")));
    // An unrelated sibling must be left untouched.
    QVERIFY(base.mkdir(QStringLiteral("keep-me")));

    cleanupAppImageStaging();

    QVERIFY(!base.exists(QStringLiteral(".daemon-update-111-abc")));
    QVERIFY(!base.exists(QStringLiteral(".daemon-update-222-def")));
    QVERIFY(base.exists(QStringLiteral("keep-me")));
}

QTEST_MAIN(SelfApplyAppImageTests)
#include "tst_self_apply_appimage.moc"
