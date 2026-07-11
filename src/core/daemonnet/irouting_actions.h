// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "domain/ids.h"
#include "domain/origin.h"

namespace daemonnet {

// The node-authoritative routing mutation seam (spec 09 §7: routing pins are DIRECT verbs, not an
// outbox lane — the §6.4 lane census is closed). The routing manager view-model calls this to pin
// / unpin an origin; a daemon adapter (`RoutingRepository`) sends the RoutingBindChat /
// RoutingUnbindChat wire op, and the ingestor re-fetches the node-acked result into the mirror
// (single writer preserved — the mutation never writes a mirror row itself). Null in mock mode
// (mutations are inert until A8 seeds the mirror). A pure interface (no QObject): the view-model
// dynamic_casts the bound QObject* to it, keeping the Pages layer free of the daemon adapter type.
class IRoutingActions {
public:
    virtual ~IRoutingActions() = default;

    // Pin an origin to a session (+ optional agent override). Empty `profile` = no override.
    virtual void routingBindChat(const domain::Origin& origin, const domain::SessionId& session,
                                 const domain::ProfileRef& profile) = 0;
    // Remove an origin's pin.
    virtual void routingUnbindChat(const domain::Origin& origin) = 0;
};

} // namespace daemonnet
