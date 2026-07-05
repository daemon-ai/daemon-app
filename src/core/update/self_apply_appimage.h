// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QString>

// AppImage self-apply backend (design: packaging/UPDATES.md, "AppImage
// (SelfApply)"). The app never mutates itself: it stages the verified new image
// next to $APPIMAGE on the SAME filesystem, copies the daemon-updater helper OUT
// of the (about-to-vanish) squashfs mount, spawns that copy detached with a
// rename-swap spec, and quits. The helper waits for the app PID to exit, atomically
// swaps $APPIMAGE, and relaunches the new image.
//
// Every function is Linux/AppImage-specific and inert elsewhere (runningAsAppImage
// is false). The apply() dispatch degrades to DownloadAndOpen whenever any
// precondition here fails - a broken state is never left behind.

namespace update {

// The staged spec handed to the daemon-updater helper. `ok == false` means a
// precondition failed and the caller must degrade to DownloadAndOpen; `reason`
// is a short, user-facing explanation.
struct AppImageApplyPreparation {
    bool ok = false;
    QString reason;
    QString helperCopyPath;  // the helper copied OUT of the mount (survives exit)
    QString stagedImagePath; // the verified new image, on the target's filesystem
    QString targetImagePath; // canonical $APPIMAGE (the file swapped + relaunched)
    QString sha256;          // expected digest (the helper re-verifies before mutating)
    QString logPath;         // helper log path
    QString stagingDir;      // the ".daemon-update-*" dir (cleaned on next boot)
};

// True when the running process is an AppImage: $APPIMAGE is set and points at an
// existing file. Everything below is a no-op / failure when this is false.
[[nodiscard]] bool runningAsAppImage();

// Stage everything the helper needs for an in-place AppImage swap. Canonicalizes
// $APPIMAGE (it may be relative), locates the helper next to the running binary
// (<mount>/usr/bin/daemon-updater) and copies it into a private staging dir on the
// TARGET filesystem (so it outlives the mount), then copies the verified
// downloaded image into that same-fs dir. `downloadedImagePath` is the
// sha256-verified artifact from UpdateDownloader; `sha256Hex` is the signed digest
// the helper re-verifies. Returns ok=false + a reason on any precondition failure
// (no $APPIMAGE, helper missing, staging dir not writable, copy failed).
[[nodiscard]] AppImageApplyPreparation prepareAppImageApply(const QString& downloadedImagePath,
                                                            const QString& sha256Hex);

// Spawn the copied helper fully detached with the rename-swap invocation
// (--wait-pid=<appPid>, --relaunch -- <target>). Returns true when the helper
// process was launched; the caller then quits so the helper can swap + relaunch.
[[nodiscard]] bool spawnAppImageHelper(const AppImageApplyPreparation& prep, qint64 appPid);

// Startup hygiene: remove leftover ".daemon-update-*" staging dirs beside
// $APPIMAGE from prior (completed or aborted) self-applies. Best-effort; a no-op
// when not running as an AppImage.
void cleanupAppImageStaging();

} // namespace update
