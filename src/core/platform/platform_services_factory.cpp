#include "platform/platform_services_factory.h"

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
#include "platform/mobile/noop_platform_services.h"
#else
#include "platform/desktop/desktop_platform_services.h"
#endif

namespace platform {

IPlatformServices* createPlatformServices(QObject* parent) {
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    return new NoopPlatformServices(parent);
#else
    return new DesktopPlatformServices(parent);
#endif
}

} // namespace platform
