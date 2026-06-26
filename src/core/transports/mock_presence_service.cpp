#include "transports/mock_presence_service.h"

namespace transports {

QString MockPresenceService::connectionState(const QString& /*transport*/) const {
    return QStringLiteral("offline");
}

QString MockPresenceService::presence(const QString& /*transport*/) const {
    return QStringLiteral("unknown");
}

} // namespace transports
