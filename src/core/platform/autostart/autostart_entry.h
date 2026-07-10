// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QString>
#include <QStringList>

// Pure XDG autostart desktop-entry logic (render / parse / staleness), shared
// by the Linux backend and the unit tests. Compiled on every platform (QtCore
// only) so the Linux behavior is covered by CI regardless of host OS; only
// autostart_backend_xdg.cpp touches the filesystem.
namespace autostart {

// Quote one argument for an Exec= value per the desktop-entry spec: always
// double-quoted, with `"` ` ` `$` `\` backslash-escaped inside.
[[nodiscard]] QString quoteExecArg(const QString& arg);

// The parsed facts of an autostart entry this app owns.
struct EntryFacts {
    QString execProgram;  // unquoted first Exec token; empty when unparseable
    QStringList execArgs; // remaining Exec tokens, unquoted
    QString tryExec;
    // Both default to the spec's "entry is active" reading for absent keys.
    bool hidden = false;
    bool gnomeEnabled = true;
};

// Render the complete .desktop file content for `program` + `args`. `hidden` /
// `gnomeEnabled` are carried through on repair so a user's external disable
// (a DE writing Hidden=true / X-GNOME-Autostart-enabled=false) survives.
[[nodiscard]] QString renderAutostartEntry(const QString& program, const QStringList& args,
                                           bool hidden = false, bool gnomeEnabled = true);

// Parse `content` (full file text). Unknown keys are ignored; missing keys
// yield the EntryFacts defaults above.
[[nodiscard]] EntryFacts parseAutostartEntry(const QString& content);

// The spec'd "will the DE launch this at login" reading of a parsed entry.
[[nodiscard]] bool entryEnablesAutostart(const EntryFacts& facts);

// True when the entry no longer launches `program` with `hiddenArg` (moved
// binary, AppImage self-update, nix store churn) and must be rewritten.
[[nodiscard]] bool entryNeedsRewrite(const EntryFacts& facts, const QString& program,
                                     const QString& hiddenArg);

} // namespace autostart
