// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "platform/autostart/autostart_status.h"

#include <QObject>
#include <QString>

namespace settings {
class ISettingsStore;
}

namespace autostart {

// The shared GUI/TUI seam for "launch at login, minimized to tray". The GUI
// registers one instance as the `Autostart` context property; the TUI passes
// one into its settings hub. All registration happens here at runtime (no
// installer logic), against the OS mechanism in autostart_backend.h, and the
// OS entry stays the source of truth: every property reflects the last live
// query, refreshed after each operation.
//
// `program` is the executable the login entry launches (the GUI passes
// itself via resolveProgramForCurrentApp(); the TUI passes its GUI sibling via
// resolveSiblingGuiProgram()). The store only records two bookkeeping keys:
// app/autostartConfigured (the one-time first-run default marker) and
// app/autostartRegisteredVersion (macOS re-register-on-update).
class AutostartController : public QObject {
    Q_OBJECT
    // Row visible at all (false on wasm/mobile, Flatpak, missing GUI sibling).
    Q_PROPERTY(bool supported READ supported NOTIFY statusChanged)
    // Toggle interactive now (false when Blocked, e.g. macOS run from the DMG).
    Q_PROPERTY(bool available READ available NOTIFY statusChanged)
    Q_PROPERTY(bool enabled READ enabled NOTIFY statusChanged)
    // macOS: registered but pending System Settings > Login Items approval.
    Q_PROPERTY(bool requiresApproval READ requiresApproval NOTIFY statusChanged)
    // Human-readable Blocked reason / approval hint; empty otherwise.
    Q_PROPERTY(QString reason READ reason NOTIFY statusChanged)

public:
    explicit AutostartController(QString program, settings::ISettingsStore* settings,
                                 QObject* parent = nullptr);

    [[nodiscard]] bool supported() const { return m_status.state != State::Unsupported; }
    [[nodiscard]] bool available() const {
        return m_status.state == State::Disabled || m_status.state == State::Enabled ||
               m_status.state == State::RequiresApproval;
    }
    [[nodiscard]] bool enabled() const { return m_status.state == State::Enabled; }
    [[nodiscard]] bool requiresApproval() const {
        return m_status.state == State::RequiresApproval;
    }
    [[nodiscard]] QString reason() const;
    [[nodiscard]] const Status& status() const { return m_status; }

    // Register/unregister the OS entry. Any explicit call also sets the
    // one-time configured marker so the first-run default never overrides a
    // user's later choice.
    Q_INVOKABLE void setEnabled(bool on);
    // Re-query the OS (the user may have flipped the entry externally).
    Q_INVOKABLE void refresh();
    // macOS RequiresApproval affordance; no-op elsewhere.
    Q_INVOKABLE void openApprovalSettings();

    // Startup self-heal: rewrite a stale entry (moved binary / AppImage
    // update / nix store churn) and, on macOS, re-register once after an app
    // update so launchd re-reads the swapped executable.
    void repairOnBoot();

    // Default-on exactly once, at first-run completion: if the marker is
    // absent and the state is cleanly Disabled (not Blocked/Unsupported),
    // enable and set the marker. Never re-forces after any explicit choice.
    void applyFirstRunDefault();

signals:
    void statusChanged();

private:
    void markConfigured();
    void noteRegisteredVersion();

    QString m_program;
    settings::ISettingsStore* m_settings = nullptr;
    Status m_status;
};

} // namespace autostart
