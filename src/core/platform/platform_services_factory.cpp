// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "platform/platform_services_factory.h"

// Mobile and the browser share the no-op services (no tray, no OS
// notifications); the desktop implementation is not even compiled there
// (see CMakeLists.txt), so the include must be gated too.
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS) || defined(Q_OS_WASM)
#include "platform/mobile/noop_platform_services.h"
#else
#include "platform/desktop/desktop_platform_services.h"
#endif

namespace platform {

IPlatformServices* createPlatformServices(QObject* parent) {
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS) || defined(Q_OS_WASM)
    return new NoopPlatformServices(parent);
#else
    return new DesktopPlatformServices(parent);
#endif
}

} // namespace platform
