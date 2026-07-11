// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "platform/app_icon.h"

#include <initializer_list>
#include <QPixmap>
#include <QString>

namespace platform {

QIcon appIcon() {
    // The installed hicolor theme entry wins when present (Linux desktops with
    // an icon theme + the shipped share/icons/hicolor/... set), so the DE serves
    // its own scaled/themed rendering.
    QIcon themed = QIcon::fromTheme(QStringLiteral("daemon-app"));
    if (!themed.isNull()) {
        return themed;
    }

    // Fallback: the multi-size PNGs compiled into the executable resource
    // (App/CMakeLists.txt aliases packaging/linux/icons/daemon-app-<N>.png under
    // ":/icons/"). Adding several sizes lets Qt pick the sharpest per surface
    // (16/32 for the tray + title bar, larger for the taskbar/alt-tab).
    QIcon icon;
    for (int size : {16, 32, 48, 64, 128, 256}) {
        QPixmap pm(QStringLiteral(":/icons/daemon-app-%1.png").arg(size));
        if (!pm.isNull()) {
            icon.addPixmap(pm);
        }
    }
    return icon;
}

} // namespace platform
