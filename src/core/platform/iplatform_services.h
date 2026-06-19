#pragma once

#include <QObject>
#include <QString>

namespace platform {

// The OS-integration seam. App code talks only to this interface; desktop and
// mobile provide implementations, keeping the rest of the app #ifdef-free.
//
// Tray is implemented now. Autostart / updates / single-instance are declared
// as no-op defaults and filled in later behind this same interface.
class IPlatformServices : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    ~IPlatformServices() override = default;

    // Install a system-tray icon with Show/Hide + Quit. No-op where unsupported.
    virtual void installTray(const QString& appName) = 0;

    // Deferred capabilities (no-op defaults until implemented).
    virtual void setAutostartEnabled(bool /*enabled*/) {}
    virtual void checkForUpdates() {}

signals:
    void toggleWindowRequested();
    void showWindowRequested();
    void quitRequested();
};

} // namespace platform
