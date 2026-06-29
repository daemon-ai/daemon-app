// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>

namespace platform {

class IPlatformServices;

// Returns the platform-appropriate services implementation (desktop tray vs
// mobile no-op), owned by `parent`.
IPlatformServices* createPlatformServices(QObject* parent = nullptr);

} // namespace platform
