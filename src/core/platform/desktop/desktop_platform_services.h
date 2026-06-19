#pragma once

#include "platform/iplatform_services.h"

QT_BEGIN_NAMESPACE
class QSystemTrayIcon;
class QMenu;
QT_END_NAMESPACE

namespace platform {

// Desktop implementation: a QSystemTrayIcon (QtWidgets) with a Show/Hide + Quit
// menu. Coexists with the QQuickWindow UI (no rendering conflict).
class DesktopPlatformServices : public IPlatformServices {
    Q_OBJECT

public:
    explicit DesktopPlatformServices(QObject* parent = nullptr);
    ~DesktopPlatformServices() override;

    void installTray(const QString& appName) override;

private:
    QSystemTrayIcon* m_tray = nullptr;
    QMenu* m_menu = nullptr;
};

} // namespace platform
