#pragma once

#include "platform/iplatform_services.h"

namespace platform {

// Mobile / headless implementation: no tray, no desktop integrations.
class NoopPlatformServices : public IPlatformServices {
    Q_OBJECT

public:
    using IPlatformServices::IPlatformServices;

    bool installTray(const QString& /*appName*/) override { return false; }
};

} // namespace platform
