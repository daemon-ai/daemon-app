// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <cstdint>
#include <QString>

// macOS DMG SelfApply (design: packaging/UPDATES.md, U3). The app has already
// downloaded + sha256-verified the .dmg (UpdateDownloader). This module mounts
// it, copies the single .app into a fresh 0700 staging dir on the TARGET
// bundle's filesystem, strips quarantine, then hands a two-move swap spec to the
// in-bundle daemon-updater helper and lets the caller quit. If the environment
// is unsafe for an in-place swap (translocated, read-only volume, unwritable
// application folder, cross-filesystem staging) it falls back to DownloadAndOpen
// (open the dmg) with a human reason.
//
// The whole file compiles on every platform (it only uses Qt + QProcess), but is
// only ever CALLED behind Q_OS_MACOS from update_manager. The guard decision and
// the path predicates are pure so they can be unit-tested on the Linux CI host.
namespace update::macos {

// The four environment facts a SelfApply decision rests on. Gathered by
// probeGuardFacts(); the decision (evaluateGuards) is kept pure for testing.
struct GuardFacts {
    // This user can create/rename entries in the bundle's parent directory
    // (so the helper can two-move the bundle in and out of place).
    bool bundleParentWritable = false;
    // The staging root and the target bundle share one filesystem (an atomic
    // rename cannot cross a mount; the helper refuses a cross-fs move).
    bool sameFilesystem = false;
    // The app is running translocated by Gatekeeper (path under
    // /AppTranslocation/): the on-disk original is elsewhere, so swapping the
    // translocated copy is ineffective and unsafe.
    bool translocated = false;
    // The bundle sits on a read-only mount (typically launched straight from
    // the DMG under /Volumes): nothing can be written in place.
    bool readOnlyVolume = false;
};

// The SelfApply-vs-fallback verdict. `reason` is a human, user-facing string
// (empty iff selfApply).
struct GuardVerdict {
    bool selfApply = false;
    QString reason;
};

// Pure: decide whether an in-place swap is safe from the gathered facts.
[[nodiscard]] GuardVerdict evaluateGuards(const GuardFacts& facts);

// Pure path predicate: a Gatekeeper-translocated bundle path (contains
// "/AppTranslocation/", e.g. /private/var/folders/…/AppTranslocation/<uuid>/d/…).
[[nodiscard]] bool pathIsTranslocated(const QString& bundlePath);

// Pure path predicate: a bundle mounted under /Volumes (a DMG or external
// volume). Combined with the read-only probe this flags "running from the dmg".
[[nodiscard]] bool pathUnderVolumes(const QString& bundlePath);

// The running app's .app bundle path, derived from applicationDirPath()
// (.../daemon-app.app/Contents/MacOS -> .../daemon-app.app). Empty when the
// on-disk layout is not a .app bundle (e.g. a plain dev build).
[[nodiscard]] QString runningBundlePath();

// Gather the guard facts for a target `bundlePath` staged from `stagingParent`.
[[nodiscard]] GuardFacts probeGuardFacts(const QString& bundlePath, const QString& stagingParent);

// The result of staging the .app out of the mounted dmg.
struct StageResult {
    bool ok = false;
    QString stagingRoot; // the 0700 dir on the target filesystem (also holds the log + parked old)
    QString stagedApp;   // <stagingRoot>/<Name>.app, quarantine-stripped and verified
    QString error;       // populated iff !ok
};

// Mount `dmgPath` (already sha256-verified), copy the single top-level .app into
// a fresh 0700 staging dir beside `targetBundlePath` (same filesystem), detach,
// strip com.apple.quarantine, and superficially verify the staged bundle
// (Contents/MacOS/daemon-app is executable, Info.plist version == expectedVersion).
[[nodiscard]] StageResult stageDmg(const QString& dmgPath, const QString& targetBundlePath,
                                   const QString& expectedVersion);

// The outcome of a SelfApply attempt.
enum class Outcome : std::uint8_t {
    Applied,  // helper spawned; the caller MUST quit so the swap+relaunch proceeds
    FellBack, // guard failed; the dmg was opened (DownloadAndOpen); see message
    Failed,   // staging/spawn error; nothing swapped; see message
};

struct ApplyResult {
    Outcome outcome = Outcome::Failed;
    QString message; // fallback reason (FellBack) or error detail (Failed); empty on Applied
};

// Full SelfApply entry point. Evaluates the guards; on success stages the dmg,
// spawns the in-bundle daemon-updater (two-move, wait-pid=appPid, relaunch to the
// new binary) and returns Applied. On a guard failure opens the dmg and returns
// FellBack. On a staging/spawn error returns Failed.
[[nodiscard]] ApplyResult selfApply(const QString& dmgPath, const QString& expectedVersion,
                                    qint64 appPid);

// Startup hygiene, extending the core's cleanupStale: delete leftover
// ".daemon-app-update-*" staging dirs (and the old bundles parked inside them)
// next to the target bundle from a prior apply. Best-effort; no-op off a bundle.
void cleanupStagingArtifacts(const QString& targetBundlePath);

} // namespace update::macos
