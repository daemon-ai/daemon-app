// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "platform/iplatform_services.h"

QT_BEGIN_NAMESPACE
class QSystemTrayIcon;
class QMenu;
class QTimer;
QT_END_NAMESPACE

namespace platform {

// Desktop implementation: a QSystemTrayIcon (QtWidgets) with a Show/Hide + Quit
// menu. Coexists with the QQuickWindow UI (no rendering conflict).
class DesktopPlatformServices : public IPlatformServices {
    Q_OBJECT

public:
    explicit DesktopPlatformServices(QObject* parent = nullptr);
    ~DesktopPlatformServices() override;

    bool installTray(const QString& appName) override;
    void watchForTray(const QString& appName) override;
    bool notify(const QString& title, const QString& body) override;

private:
    QSystemTrayIcon* m_tray = nullptr;
    QMenu* m_menu = nullptr;
    // Bounded post-login retry (watchForTray): poll isSystemTrayAvailable()
    // until the DE's tray host appears or the window elapses. Polling (rather
    // than a DBus service watch) covers every backend Qt itself supports -
    // StatusNotifier AND legacy XEmbed - with no extra Qt module.
    QTimer* m_trayWatch = nullptr;
    int m_trayWatchAttempts = 0;
};

} // namespace platform
