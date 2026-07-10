// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "platform/desktop/desktop_platform_services.h"

#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QSystemTrayIcon>
#include <QTimer>

namespace platform {

namespace {

// watchForTray cadence: the StatusNotifier host typically appears within a few
// seconds of login; give it 20 s before the caller falls back to showing the
// window (never an unreachable, invisible app).
constexpr int kTrayWatchIntervalMs = 2000;
constexpr int kTrayWatchMaxAttempts = 10;

// A tray needs an icon to be shown on most platforms; synthesize a simple one
// so the foundation has no asset dependency yet.
QIcon placeholderTrayIcon() {
    QIcon themed = QIcon::fromTheme(QStringLiteral("application-x-executable"));
    if (!themed.isNull()) {
        return themed;
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

    m_tray = new QSystemTrayIcon(placeholderTrayIcon(), this);
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

void DesktopPlatformServices::watchForTray(const QString& appName) {
    if (m_tray != nullptr || m_trayWatch != nullptr) {
        return;
    }
    m_trayWatchAttempts = 0;
    m_trayWatch = new QTimer(this);
    m_trayWatch->setInterval(kTrayWatchIntervalMs);
    connect(m_trayWatch, &QTimer::timeout, this, [this, appName] {
        const auto finish = [this] {
            m_trayWatch->stop();
            m_trayWatch->deleteLater();
            m_trayWatch = nullptr;
        };
        if (installTray(appName)) {
            finish();
            emit trayAvailable();
            return;
        }
        if (++m_trayWatchAttempts >= kTrayWatchMaxAttempts) {
            finish();
        }
    });
    m_trayWatch->start();
}

bool DesktopPlatformServices::notify(const QString& title, const QString& body) {
    // Notifications ride the tray icon (the portable Qt path). Without a tray
    // there's nowhere to anchor the balloon, so report that nothing was shown.
    if (m_tray == nullptr || !QSystemTrayIcon::supportsMessages()) {
        return false;
    }
    m_tray->showMessage(title, body, placeholderTrayIcon(), 5000);
    return true;
}

} // namespace platform
