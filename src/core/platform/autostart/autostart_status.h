// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <cstdint>
#include <QString>

namespace autostart {

// The OS-level launch-at-login state as reported by the per-OS backend
// (autostart_backend.h). The OS entry is the single source of truth: users can
// flip it externally (Task Manager, System Settings > Login Items, a DE's own
// autostart tool), so the settings UI re-queries live state instead of trusting
// a persisted preference.
enum class State : std::uint8_t {
    // The mechanism does not exist in this environment (wasm/mobile, Flatpak
    // sandbox, no launchable GUI program). The settings row is hidden.
    Unsupported,
    // The mechanism exists but registration is impossible right now; Detail
    // says why (e.g. macOS translocation). The row renders disabled + reason.
    Blocked,
    Disabled,
    Enabled,
    // macOS only: registered, pending the user's approval under
    // System Settings > General > Login Items.
    RequiresApproval,
};

// Why a state is Unsupported/Blocked, as a code: the controller owns the
// translated user-facing wording, the backends stay string-free and pure.
enum class Detail : std::uint8_t {
    None,
    // No launchable program could be resolved (e.g. the TUI found no GUI
    // sibling binary to register).
    NoProgram,
    // macOS: not running from an installed .app bundle (plain dev build).
    NeedsAppBundle,
    // macOS: Gatekeeper-translocated or running straight off the DMG/volume -
    // registering would bind the login item to an ephemeral path.
    Translocated,
    RunningFromDmg,
    // Linux: inside a Flatpak sandbox. The correct mechanism there is the
    // Background portal (RequestBackground), a documented follow-up.
    Flatpak,
    // The OS call itself failed; Status::message carries the OS error text.
    RegistrationFailed,
};

struct Status {
    State state = State::Unsupported;
    Detail detail = Detail::None;
    // Untranslated OS error detail (RegistrationFailed only).
    QString message;
};

} // namespace autostart
