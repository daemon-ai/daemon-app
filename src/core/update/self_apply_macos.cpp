// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "update/self_apply_macos.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>
#include <QUrl>
#include <QXmlStreamReader>

#ifdef Q_OS_MACOS
#include <sys/mount.h>
#include <sys/stat.h>
#elif defined(Q_OS_UNIX)
#include <sys/stat.h>
#endif

namespace update::macos {

namespace {

constexpr int kProcTimeoutMs = 60 * 1000; // hdiutil/ditto on a bundle: generous
constexpr int kDetachAttempts = 5;
constexpr const char* kStagingPrefix = ".daemon-app-update-";

// Run a program synchronously; capture merged stdout+stderr. Returns the process
// exit code, or -1 if it failed to start / timed out.
int runProcess(const QString& program, const QStringList& args, QString* output = nullptr,
               int timeoutMs = kProcTimeoutMs) {
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(program, args);
    if (!proc.waitForStarted(timeoutMs)) {
        if (output != nullptr) {
            *output = QStringLiteral("failed to start %1").arg(program);
        }
        return -1;
    }
    if (!proc.waitForFinished(timeoutMs)) {
        proc.kill();
        proc.waitForFinished(2000);
        if (output != nullptr) {
            *output = QStringLiteral("%1 timed out").arg(program);
        }
        return -1;
    }
    if (output != nullptr) {
        *output = QString::fromLocal8Bit(proc.readAll());
    }
    if (proc.exitStatus() != QProcess::NormalExit) {
        return -1;
    }
    return proc.exitCode();
}

// Parse the mount point out of an `hdiutil attach -plist` XML plist: the first
// <string> value under a <key>mount-point</key>. Robust to the surrounding
// system-entities array shape (only the mounted volume carries a mount-point).
QString parseMountPoint(const QString& plistXml) {
    QXmlStreamReader xml(plistXml);
    bool expectMountPointValue = false;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == QLatin1String("key")) {
                const QString key = xml.readElementText();
                expectMountPointValue = (key == QLatin1String("mount-point"));
            } else if (xml.name() == QLatin1String("string") && expectMountPointValue) {
                return xml.readElementText();
            }
        }
    }
    return {};
}

// Best-effort recursive delete of a staging dir.
void removeTree(const QString& path) {
    if (path.isEmpty()) {
        return;
    }
    QDir(path).removeRecursively();
}

// True iff two existing paths sit on the same filesystem (POSIX st_dev). On a
// non-POSIX build this is never reached (SelfApply is macOS-only) so it returns
// true rather than block the compile with a Windows device probe.
bool sameDevice(const QString& a, const QString& b) {
#ifdef Q_OS_UNIX
    struct stat sa{};
    struct stat sb{};
    if (::stat(a.toLocal8Bit().constData(), &sa) != 0 ||
        ::stat(b.toLocal8Bit().constData(), &sb) != 0) {
        return false;
    }
    return sa.st_dev == sb.st_dev;
#else
    Q_UNUSED(a);
    Q_UNUSED(b);
    return true;
#endif
}

// True iff `dir` sits on a read-only mount. macOS-only (statfs + MNT_RDONLY);
// elsewhere conservatively false (never consulted off darwin).
bool isReadOnlyMount(const QString& dir) {
#ifdef Q_OS_MACOS
    struct statfs sfs{};
    if (::statfs(dir.toLocal8Bit().constData(), &sfs) != 0) {
        return false;
    }
    return (sfs.f_flags & MNT_RDONLY) != 0;
#else
    Q_UNUSED(dir);
    return false;
#endif
}

// True iff this user can create an entry in `dir` (a create-probe catches ACLs a
// mode-bit check would miss).
bool dirIsWritable(const QString& dir) {
    const QString probe = QDir(dir).filePath(
        QStringLiteral(".daemon-app-write-probe-%1").arg(QCoreApplication::applicationPid()));
    QFile f(probe);
    if (!f.open(QIODevice::WriteOnly)) {
        return false;
    }
    f.close();
    f.remove();
    return true;
}

// Read CFBundleShortVersionString from a bundle's Info.plist via plutil (the
// canonical macOS tool; keeps us out of hand-rolled plist parsing).
QString bundleShortVersion(const QString& bundlePath) {
    const QString plist = bundlePath + QStringLiteral("/Contents/Info.plist");
    QString out;
    const int rc =
        runProcess(QStringLiteral("plutil"),
                   {QStringLiteral("-extract"), QStringLiteral("CFBundleShortVersionString"),
                    QStringLiteral("raw"), QStringLiteral("-o"), QStringLiteral("-"), plist},
                   &out, 10 * 1000);
    if (rc != 0) {
        return {};
    }
    return out.trimmed();
}

} // namespace

GuardVerdict evaluateGuards(const GuardFacts& facts) {
    if (facts.translocated) {
        return {false,
                QStringLiteral("the app is running translocated by macOS Gatekeeper; move it "
                               "to your Applications folder and reopen it")};
    }
    if (facts.readOnlyVolume) {
        return {false, QStringLiteral("the app is running from a read-only volume; copy it to your "
                                      "Applications folder first")};
    }
    if (!facts.bundleParentWritable) {
        return {false,
                QStringLiteral("the application folder is not writable by this user account")};
    }
    if (!facts.sameFilesystem) {
        return {false, QStringLiteral("the update staging area is on a different filesystem than "
                                      "the app")};
    }
    return {true, {}};
}

bool pathIsTranslocated(const QString& bundlePath) {
    return bundlePath.contains(QStringLiteral("/AppTranslocation/"));
}

bool pathUnderVolumes(const QString& bundlePath) {
    return bundlePath.startsWith(QStringLiteral("/Volumes/"));
}

QString runningBundlePath() {
    // applicationDirPath() is .../daemon-app.app/Contents/MacOS inside a bundle.
    QDir dir(QCoreApplication::applicationDirPath());
    if (dir.dirName() != QStringLiteral("MacOS")) {
        return {};
    }
    if (!dir.cdUp() || dir.dirName() != QStringLiteral("Contents")) {
        return {};
    }
    if (!dir.cdUp()) {
        return {};
    }
    QString bundle = dir.absolutePath();
    if (!bundle.endsWith(QStringLiteral(".app"))) {
        return {};
    }
    return bundle;
}

GuardFacts probeGuardFacts(const QString& bundlePath, const QString& stagingParent) {
    GuardFacts facts;
    facts.translocated = pathIsTranslocated(bundlePath);
    facts.readOnlyVolume = isReadOnlyMount(bundlePath) ||
                           (pathUnderVolumes(bundlePath) && isReadOnlyMount(bundlePath));
    facts.bundleParentWritable = dirIsWritable(stagingParent);
    facts.sameFilesystem = sameDevice(stagingParent, QFileInfo(bundlePath).absolutePath());
    return facts;
}

StageResult stageDmg(const QString& dmgPath, const QString& targetBundlePath,
                     const QString& expectedVersion) {
    StageResult result;

    const QString targetParent = QFileInfo(targetBundlePath).absolutePath();
    // Stage on the TARGET filesystem: a sibling dir of the bundle, so the helper's
    // two-move rename never crosses a mount. 0700 owns the staged bundle's
    // integrity (the helper takes no --sha256 for a directory).
    const QString stagingRoot = QDir(targetParent)
                                    .filePath(QStringLiteral("%1%2")
                                                  .arg(QLatin1String(kStagingPrefix))
                                                  .arg(QCoreApplication::applicationPid()));
    removeTree(stagingRoot); // a stale root from a crashed prior attempt
    QDir dir;
    if (!dir.mkpath(stagingRoot)) {
        result.error = QStringLiteral("could not create the update staging directory");
        return result;
    }
    QFile::setPermissions(stagingRoot,
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);

    // Mount the verified dmg read-only, no browser window, skip the checksum
    // (we already sha256-gated the file); -plist for a robust mount-point parse.
    QString attachOut;
    const int attachRc = runProcess(QStringLiteral("hdiutil"),
                                    {QStringLiteral("attach"), QStringLiteral("-nobrowse"),
                                     QStringLiteral("-noverify"), QStringLiteral("-readonly"),
                                     QStringLiteral("-plist"), dmgPath},
                                    &attachOut);
    if (attachRc != 0) {
        removeTree(stagingRoot);
        result.error = QStringLiteral("hdiutil attach failed: %1").arg(attachOut.trimmed());
        return result;
    }
    const QString mountPoint = parseMountPoint(attachOut);
    if (mountPoint.isEmpty()) {
        removeTree(stagingRoot);
        result.error = QStringLiteral("could not determine the dmg mount point");
        return result;
    }

    auto detach = [&]() {
        for (int i = 0; i < kDetachAttempts; ++i) {
            if (runProcess(QStringLiteral("hdiutil"), {QStringLiteral("detach"), mountPoint},
                           nullptr, 15 * 1000) == 0) {
                return;
            }
            // Busy volume: force on the final attempts.
            if (runProcess(QStringLiteral("hdiutil"),
                           {QStringLiteral("detach"), mountPoint, QStringLiteral("-force")},
                           nullptr, 15 * 1000) == 0) {
                return;
            }
        }
    };

    // Locate the single top-level .app in the dmg root.
    const QStringList apps =
        QDir(mountPoint).entryList({QStringLiteral("*.app")}, QDir::Dirs | QDir::NoDotAndDotDot);
    if (apps.size() != 1) {
        detach();
        removeTree(stagingRoot);
        result.error =
            QStringLiteral("expected exactly one .app in the dmg, found %1").arg(apps.size());
        return result;
    }
    const QString srcApp = QDir(mountPoint).filePath(apps.first());
    const QString stagedApp = QDir(stagingRoot).filePath(apps.first());

    // ditto copies a .app faithfully: symlinks, POSIX perms, ACLs, extended
    // attributes and resource forks are all preserved (a plain recursive copy or
    // QDir clone would flatten symlinks and drop xattrs, corrupting the bundle
    // and voiding its ad-hoc signature). This is the Apple-blessed bundle copy.
    QString dittoOut;
    const int dittoRc = runProcess(QStringLiteral("ditto"), {srcApp, stagedApp}, &dittoOut);
    detach(); // unmount as soon as the copy is done, before verifying the staged copy
    if (dittoRc != 0) {
        removeTree(stagingRoot);
        result.error =
            QStringLiteral("ditto copy of the staged bundle failed: %1").arg(dittoOut.trimmed());
        return result;
    }

    // Strip com.apple.quarantine defensively (our own download is not
    // quarantine-flagged, but a dmg's contents could be). Best-effort: xattr
    // returns non-zero when the attr is simply absent, which is fine.
    runProcess(QStringLiteral("xattr"),
               {QStringLiteral("-dr"), QStringLiteral("com.apple.quarantine"), stagedApp}, nullptr,
               30 * 1000);

    // Superficial verification of the staged bundle before we let the helper
    // swap it in: the executable must be present + runnable and the advertised
    // version must match the signed manifest.
    const QString exe = stagedApp + QStringLiteral("/Contents/MacOS/daemon-app");
    const QFileInfo exeInfo(exe);
    if (!exeInfo.exists() || !exeInfo.isExecutable()) {
        removeTree(stagingRoot);
        result.error =
            QStringLiteral("staged bundle is missing an executable Contents/MacOS/daemon-app");
        return result;
    }
    if (!expectedVersion.isEmpty()) {
        const QString staged = bundleShortVersion(stagedApp);
        if (staged != expectedVersion) {
            removeTree(stagingRoot);
            result.error =
                QStringLiteral("staged bundle version %1 does not match the manifest %2")
                    .arg(staged.isEmpty() ? QStringLiteral("<unknown>") : staged, expectedVersion);
            return result;
        }
    }

    result.ok = true;
    result.stagingRoot = stagingRoot;
    result.stagedApp = stagedApp;
    return result;
}

ApplyResult selfApply(const QString& dmgPath, const QString& expectedVersion, qint64 appPid) {
    ApplyResult result;

    const QString targetBundle = runningBundlePath();
    if (targetBundle.isEmpty()) {
        result.outcome = Outcome::Failed;
        result.message = QStringLiteral("the app is not running from a .app bundle");
        return result;
    }
    const QString stagingParent = QFileInfo(targetBundle).absolutePath();

    // Decide BEFORE mutating anything: any failed guard degrades to
    // DownloadAndOpen (open the dmg) with a visible reason.
    const GuardVerdict verdict = evaluateGuards(probeGuardFacts(targetBundle, stagingParent));
    if (!verdict.selfApply) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(dmgPath));
        result.outcome = Outcome::FellBack;
        result.message = verdict.reason;
        return result;
    }

    const StageResult staged = stageDmg(dmgPath, targetBundle, expectedVersion);
    if (!staged.ok) {
        result.outcome = Outcome::Failed;
        result.message = staged.error;
        return result;
    }

    // The helper ships inside the bundle (Contents/MacOS); it is explicitly safe
    // to spawn it from within the target directory it will move (macOS keeps the
    // running inode alive across the rename).
    const QString helper = targetBundle + QStringLiteral("/Contents/MacOS/daemon-updater");
    const QString relaunchExe = targetBundle + QStringLiteral("/Contents/MacOS/daemon-app");
    // Park the displaced old bundle inside the staging root (same filesystem);
    // it is swept on the next successful start (cleanupStagingArtifacts).
    const QString oldAside = QDir(staged.stagingRoot).filePath(QStringLiteral("old-bundle.app"));
    const QString logPath = QDir(staged.stagingRoot).filePath(QStringLiteral("daemon-updater.log"));

    const QStringList args = {
        QStringLiteral("--wait-pid"),
        QString::number(appPid),
        QStringLiteral("--mode"),
        QStringLiteral("two-move"),
        QStringLiteral("--staged"),
        staged.stagedApp,
        QStringLiteral("--target"),
        targetBundle,
        QStringLiteral("--old-aside"),
        oldAside,
        QStringLiteral("--log"),
        logPath,
        QStringLiteral("--relaunch"),
        QStringLiteral("--"),
        relaunchExe,
    };

    if (!QProcess::startDetached(helper, args)) {
        removeTree(staged.stagingRoot);
        result.outcome = Outcome::Failed;
        result.message = QStringLiteral("could not launch the update helper");
        return result;
    }

    result.outcome = Outcome::Applied;
    return result;
}

void cleanupStagingArtifacts(const QString& targetBundlePath) {
    if (targetBundlePath.isEmpty()) {
        return;
    }
    const QString parent = QFileInfo(targetBundlePath).absolutePath();
    QDir dir(parent);
    const QStringList leftovers =
        dir.entryList({QStringLiteral("%1*").arg(QLatin1String(kStagingPrefix))},
                      QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);
    for (const QString& name : leftovers) {
        removeTree(dir.filePath(name));
    }
}

} // namespace update::macos
