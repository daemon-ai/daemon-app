#pragma once

#include "domain/ids.h"

#include <QMetaType>

namespace domain {

// Whether an outbound delivery target is the authoritative reply sink or a
// read-only observer — mirrors daemon-protocol `SinkKind`.
enum class SinkKind {
    Primary,
    Spectator,
};

// Where a session's outbound replies go — mirrors daemon-protocol `DeliveryTarget`.
struct DeliveryTarget {
    TransportId transport;
    RouteAddr route;
    SinkKind kind = SinkKind::Primary;
};

} // namespace domain

Q_DECLARE_METATYPE(domain::DeliveryTarget)
