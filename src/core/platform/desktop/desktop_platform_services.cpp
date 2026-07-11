// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "platform/desktop/desktop_platform_services.h"

#include "platform/app_icon.h"

#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QSystemTrayIcon>

namespace platform {

namespace {

// The tray icon: the branded app icon (platform::appIcon() - installed hicolor
// theme entry, else the multi-size PNGs embedded in the app resource). Falls
// back to a synthesized blue rounded square only if neither is available, so
// the tray is never blank on a platform that needs an icon to show it.
QIcon trayIcon() {
    QIcon branded = appIcon();
    if (!branded.isNull()) {
        return branded;
    }
    QPixmap pm(32, 32);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setBrush(QColor(0x23, 0x83, 0xe2));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(pm.rect().adjusted(2, 2, -2, -2), 8, 8);
    p.end();
    return {pm};
}

} // namespace

DesktopPlatformServices::DesktopPlatformServices(QObject* parent) : IPlatformServices(parent) {}

DesktopPlatformServices::~DesktopPlatformServices() {
    delete m_menu; // QMenu is not parented (context menu doesn't take ownership)
}

bool DesktopPlatformServices::installTray(const QString& appName) {
    if (m_tray) {
        return true;
    }
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return false;
    }

    m_menu = new QMenu;
    QAction* toggle = m_menu->addAction(QObject::tr("Show/Hide %1").arg(appName));
    QObject::connect(toggle, &QAction::triggered, this, &IPlatformServices::toggleWindowRequested);
    m_menu->addSeparator();
    QAction* quit = m_menu->addAction(QObject::tr("Quit"));
    QObject::connect(quit, &QAction::triggered, this, &IPlatformServices::quitRequested);

    m_tray = new QSystemTrayIcon(trayIcon(), this);
    m_tray->setToolTip(appName);
    m_tray->setContextMenu(m_menu);
    QObject::connect(m_tray, &QSystemTrayIcon::activated, this,
                     [this](QSystemTrayIcon::ActivationReason reason) {
                         if (reason == QSystemTrayIcon::Trigger) {
                             emit toggleWindowRequested();
                         }
                     });
    m_tray->show();
    return true;
}

bool DesktopPlatformServices::notify(const QString& title, const QString& body) {
    // Notifications ride the tray icon (the portable Qt path). Without a tray
    // there's nowhere to anchor the balloon, so report that nothing was shown.
    if (m_tray == nullptr || !QSystemTrayIcon::supportsMessages()) {
        return false;
    }
    m_tray->showMessage(title, body, trayIcon(), 5000);
    return true;
}

} // namespace platform
