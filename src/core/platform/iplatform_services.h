// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>

namespace platform {

// The OS-integration seam. App code talks only to this interface; desktop and
// mobile provide implementations, keeping the rest of the app #ifdef-free.
//
// Tray is implemented now. Launch-at-login is deliberately NOT here: it lives
// in its own widgets-free seam (platform/autostart/autostart_controller.h) so
// the TUI can surface the toggle without linking QtWidgets, which this
// interface's desktop implementation publicly drags in for QSystemTrayIcon.
// Updates remain a declared no-op default until implemented. The GUI's
// single-instance guard is platform/single_instance.h.
class IPlatformServices : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    ~IPlatformServices() override = default;

    // Install a system-tray icon with Show/Hide + Quit. No-op where unsupported.
    // Returns true only if a tray icon was actually shown (lets the app gate
    // close-to-tray on the tray really being there).
    virtual bool installTray(const QString& appName) = 0;

    // After a failed installTray, retry for a bounded window and emit
    // trayAvailable() when a later attempt succeeds. Covers the login race: an
    // autostarted app usually comes up before the DE's tray host (the
    // StatusNotifier watcher on Linux), so the first attempt legitimately
    // fails on the machines that need the tray most. No-op default.
    virtual void watchForTray(const QString& /*appName*/) {}

    // Raise a native, transient OS notification (e.g. when a turn hits an
    // approval/clarify gate while the window is hidden). No-op where unsupported.
    // Returns true only if a notification was actually shown.
    virtual bool notify(const QString& /*title*/, const QString& /*body*/) { return false; }

    // Deferred capability (no-op default until implemented).
    virtual void checkForUpdates() {}

signals:
    void toggleWindowRequested();
    void showWindowRequested();
    void quitRequested();
    // A watchForTray() retry succeeded: the tray icon is installed and close-
    // to-tray semantics can be armed late.
    void trayAvailable();
};

} // namespace platform
