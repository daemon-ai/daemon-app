// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QString>

// Launch-command resolution for the autostart entry: which executable the OS
// should start at login. The reliability crux across distribution lanes - an
// AppImage's applicationFilePath() is a FUSE mount that is gone next boot, and
// the TUI registers its GUI sibling, not itself.
namespace autostart {

// The argument the OS entry passes so the app starts minimized to the tray.
[[nodiscard]] QString hiddenArg();

// Pure precedence: a non-empty $APPIMAGE wins (the persistent .AppImage path),
// else the running executable.
[[nodiscard]] QString resolveProgramFromParts(const QString& appImagePath,
                                              const QString& applicationFilePath);

// resolveProgramFromParts over the live process ($APPIMAGE +
// QCoreApplication::applicationFilePath()).
[[nodiscard]] QString resolveProgramForCurrentApp();

// The GUI binary co-located with `dir` (all bundles co-locate daemon-app next
// to daemon-tui); empty when absent - autostart is then Unsupported for the
// TUI and its settings row is hidden.
[[nodiscard]] QString resolveSiblingGuiProgramInDir(const QString& dir);

// resolveSiblingGuiProgramInDir next to the running executable, honoring
// $APPIMAGE the same way the GUI does (a bundled TUI inside an AppImage still
// registers the AppImage itself).
[[nodiscard]] QString resolveSiblingGuiProgram();

// True inside a Flatpak sandbox (/.flatpak-info): direct autostart-dir writes
// land in the sandbox, so the feature reports Unsupported there (the Background
// portal is the documented follow-up).
[[nodiscard]] bool runningInsideFlatpak();

} // namespace autostart
