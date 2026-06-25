#pragma once

#include "domain/ids.h"

#include <QMetaType>
#include <QString>

namespace domain {

// Where an inbound (or attributed outbound) event came from — mirrors
// daemon-protocol `OriginScope`. The variant payload is flattened to optional
// fields keyed by `kind` (QML/value-type friendly).
enum class OriginScopeKind {
    Dm,       // a 1:1 direct message (uses `user`)
    Group,    // a group/room/channel (uses `chat` + optional `thread`)
    Api,      // a programmatic API caller (uses `apiKey`)
    Internal, // a host-internal origin (scheduler / background)
};

struct OriginScope {
    OriginScopeKind kind = OriginScopeKind::Internal;
    QString user;   // Dm
    QString chat;   // Group
    QString thread; // Group (optional)
    QString apiKey; // Api
};

// Mirrors daemon-protocol `Origin { transport, scope }`, carried per event.
struct Origin {
    TransportId transport;
    OriginScope scope;
};

} // namespace domain

Q_DECLARE_METATYPE(domain::OriginScope)
Q_DECLARE_METATYPE(domain::Origin)
