// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <cstdint>
#include <QString>
#include <QStringList>

// Windows NSIS SelfApply backend (design: packaging/UPDATES.md, U3).
//
// The app never replaces itself. Once the verified NSIS installer is staged,
// apply() locates daemon-updater.exe next to the app, spawns it fully detached
// with the FROZEN helper contract, and quits; the helper waits for the app PID
// to exit, relocates itself out of the install tree, re-verifies the staged
// installer's sha256, runs it silently (/S), and relaunches the installed app.
//
// The invocation this module builds (no --target; nsis-silent runs the
// installer, which owns the file swap):
//
//   daemon-updater --wait-pid <pid> --mode nsis-silent --staged <installer.exe>
//                  --sha256 <hex> --log <path> --relaunch -- <installed app.exe>
//
// The planning half is pure logic (no process spawn, filesystem existence
// checks only) so it is unit-testable on any host; the launch half is a thin
// QProcess::startDetached wrapper. The apply() dispatch that calls this is
// guarded by Q_OS_WIN in update_manager.cpp.
namespace update::win {

// The result of preflighting + building the daemon-updater invocation.
struct SelfApplyPlan {
    // All preconditions satisfied; `program`/`arguments` are ready to spawn.
    bool ok = false;
    // Human-readable reason when !ok, surfaced as the degrade-to-DownloadAndOpen
    // message ("self-apply unavailable (<reason>); opening the installer").
    QString reason;
    // Absolute path to the daemon-updater(.exe) helper next to the app.
    QString program;
    // Full argv (frozen helper CLI contract), excluding argv[0].
    QStringList arguments;
};

// Build + preflight the NSIS SelfApply invocation. Pure: touches the filesystem
// only to confirm the helper and the staged installer exist. Preconditions:
//   - daemon-updater(.exe) exists in appDir,
//   - stagedInstaller is a non-empty path to an existing regular file,
//   - sha256Hex is non-empty (the signed digest the helper re-verifies),
//   - relaunchTarget (the installed app path) is non-empty.
// waitPid is the app's own PID (the helper waits for us to exit before mutating).
[[nodiscard]] SelfApplyPlan planNsisSelfApply(const QString& appDir, const QString& stagedInstaller,
                                              const QString& sha256Hex, const QString& logPath,
                                              const QString& relaunchTarget, qint64 waitPid);

// The default helper log path: <AppDataLocation>/updates/daemon-updater.log,
// alongside the downloader's staging tree. Best-effort; the helper appends.
[[nodiscard]] QString selfApplyLogPath();

// Spawn the planned invocation fully detached (QProcess::startDetached). Returns
// true when the child launched. No-op returning false if !plan.ok. The caller
// quits the app immediately after a true return so the helper's wait-pid frees.
[[nodiscard]] bool launchDetached(const SelfApplyPlan& plan);

} // namespace update::win
