// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

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

    bool installTray(const QString& appName) override;
    bool notify(const QString& title, const QString& body) override;

private:
    QSystemTrayIcon* m_tray = nullptr;
    QMenu* m_menu = nullptr;
};

} // namespace platform
