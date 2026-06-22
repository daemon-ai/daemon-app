#include "platform/desktop/desktop_platform_services.h"

#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QSystemTrayIcon>

namespace platform {

namespace {

// A tray needs an icon to be shown on most platforms; synthesize a simple one
// so the foundation has no asset dependency yet.
QIcon placeholderTrayIcon()
{
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
    return QIcon(pm);
}

} // namespace

DesktopPlatformServices::DesktopPlatformServices(QObject* parent)
    : IPlatformServices(parent)
{
}

DesktopPlatformServices::~DesktopPlatformServices()
{
    delete m_menu; // QMenu is not parented (context menu doesn't take ownership)
}

bool DesktopPlatformServices::installTray(const QString& appName)
{
    if (m_tray) {
        return true;
    }
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return false;
    }

    m_menu = new QMenu;
    QAction* toggle = m_menu->addAction(QObject::tr("Show/Hide %1").arg(appName));
    QObject::connect(toggle, &QAction::triggered, this,
                     &IPlatformServices::toggleWindowRequested);
    m_menu->addSeparator();
    QAction* quit = m_menu->addAction(QObject::tr("Quit"));
    QObject::connect(quit, &QAction::triggered, this,
                     &IPlatformServices::quitRequested);

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

bool DesktopPlatformServices::notify(const QString& title, const QString& body)
{
    // Notifications ride the tray icon (the portable Qt path). Without a tray
    // there's nowhere to anchor the balloon, so report that nothing was shown.
    if (m_tray == nullptr || !QSystemTrayIcon::supportsMessages()) {
        return false;
    }
    m_tray->showMessage(title, body, placeholderTrayIcon(), 5000);
    return true;
}

} // namespace platform
