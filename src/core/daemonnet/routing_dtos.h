#pragma once

#include "domain/delivery.h"
#include "domain/ids.h"
#include "domain/origin.h"

#include <QString>

namespace daemonnet {

// Which precedence rung decided a resolution (mirrors RoutingRegistry::resolve in
// daemon-node routing.rs): pin > binding-rule > account-bound > node default.
enum class DecidedBy {
    Pin,
    Rule,
    AccountBound,
    Default,
};

[[nodiscard]] inline QString decidedByStr(DecidedBy d) {
    switch (d) {
    case DecidedBy::Pin:
        return QStringLiteral("pin");
    case DecidedBy::Rule:
        return QStringLiteral("rule");
    case DecidedBy::AccountBound:
        return QStringLiteral("account");
    case DecidedBy::Default:
        return QStringLiteral("default");
    }
    return {};
}

// A resolve-first chat pin (origin -> session, optional agent override). Mirrors the wire
// `ChatRoute` / host `ChatPin`. `profile` empty => falls through to deterministic precedence.
struct RoutingPin {
    domain::Origin origin;
    domain::SessionId session;
    domain::ProfileRef profile;
    QString isolation; // informational when pinned (the pinned session id is authoritative)
};

// One ordered, config-time binding rule. Read-only in the client (no runtime CRUD op exists; it is
// built from TOML at boot). Mirrors `SessionBinding`.
struct BindingRule {
    QString transportPattern;   // "matrix/@ops:hs.org" | "matrix" (family) | "*"
    QString scopeGlob;          // "#secops*" | "dm" | "api" | "*"
    QString isolation;          // perThread | perChat | perUser | shared
    domain::ProfileRef profile; // optional override (empty = none)
    QString delivery;           // "fromOrigin" | "fixed"
};

// An account -> agent baseline binding (the `instance_profiles` map, derived from a profile's
// `bound_accounts`). Precedence step 3.
struct AccountAgent {
    domain::TransportId transport;
    domain::ProfileRef profile;
};

// The outcome of resolving an origin, plus which rung decided it (the explainer payload).
struct Resolution {
    domain::SessionId session;
    domain::ProfileRef profile;
    domain::DeliveryTarget delivery;
    DecidedBy decidedBy = DecidedBy::Default;
};

// A bindable room/chat on a transport instance + its pinned session if any. Mirrors the read side
// of the `transport_rooms` op (`RoomInfo`) used by the pin picker.
struct RoomBinding {
    domain::TransportId transport;
    QString chat;                    // the chat handle / room id (the Group scope key)
    QString label;                   // display label
    domain::SessionId pinnedSession; // empty = not yet pinned
};

// The canonical, order-stable key for an origin (transport instance + scope), mirroring
// daemon-node's `origin_pin_key`. The pin map + graph nodes key on this.
[[nodiscard]] inline QString originKey(const domain::Origin& o) {
    const QString t = o.transport.toString();
    switch (o.scope.kind) {
    case domain::OriginScopeKind::Dm:
        return t + QStringLiteral("|dm|") + o.scope.user;
    case domain::OriginScopeKind::Group:
        return t + QStringLiteral("|group|") + o.scope.chat + QLatin1Char('|') + o.scope.thread;
    case domain::OriginScopeKind::Api:
        return t + QStringLiteral("|api|") + o.scope.apiKey;
    case domain::OriginScopeKind::Internal:
        return t + QStringLiteral("|internal");
    }
    return t;
}

} // namespace daemonnet
