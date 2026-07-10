// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "platform/autostart/autostart_status.h"

#include <QString>

// The thin per-OS autostart mechanism, selected at build time (one TU per OS
// in CMake): Windows = HKCU Run key + StartupApproved, macOS = SMAppService
// agent (bundle-embedded plist), Linux = XDG ~/.config/autostart entry,
// wasm/mobile = noop. `program` is the executable the entry launches (see
// autostart_command.h); the macOS backend ignores it (the agent plist is
// bundle-relative) but still requires it non-empty.
namespace autostart::backend {

// The current OS-level state.
[[nodiscard]] Status query(const QString& program);

// Register / unregister the login entry; returns the resulting state.
[[nodiscard]] Status apply(const QString& program, bool enable);

// Rewrite an existing-but-stale entry so it launches `program` (AppImage
// self-update, nix store churn, moved install). Never creates an entry.
// Returns true when something was rewritten.
bool repair(const QString& program);

// macOS: unregister + register so launchd re-reads the executable after an
// in-place app update (Apple's recommendation). No-op elsewhere.
void reregister(const QString& program);

// macOS: open System Settings > General > Login Items for the
// RequiresApproval flow. No-op elsewhere.
void openApprovalSettings();

} // namespace autostart::backend
