// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "daemonnet/mock_daemonnet.h"

#include <QString>

namespace persistence {
class ISessionStore;
}

namespace daemonapp::daemon {
class RoutingRepository;
class TransportRepository;
class SessionRepository;

// The daemon-backed IDaemonNet slice (B6/ROU + B4/EIO-8, wire v28): overrides the routing surface
// (routes/transportRooms/resolve/bindChat/unbindChat) with the RoutingRepository's real
// RoutingListChats/TransportRooms/RoutingBindChat/RoutingUnbindChat ops, projects accountsAgents
// from the TransportRepository's instances (bound_profile), and builds transportsTree() live from
// instances + conversations + pins (the roster merge). Subclasses the EMPTY-seeded mock so the
// remaining projections (fleet/sessions/channels/byPeer, the patch-bay graph) stay inert — in
// daemon mode those surfaces are served by DaemonFleetTree/DaemonSessionRoster, not this seam.
//
// Honesty notes (thin client):
//  - resolve() answers from the PIN table only (pin -> Pin, else Default). The node's full
//    precedence (rules/account-bound) has no read op at v28, so the client never re-derives it.
//  - bindingRules() is empty: the config-time rules have no wire read op.
//  - handover()/bindAccount() are inert no-ops here: handover has no wire read-back; delivery
//    targets are now read node-authoritatively via SessionGet (CHA-9), see sessionDetail() below.
//
// CHA-9 detail hydration: sessionDetail()/deliveryTargets() project the SessionRepository's
// on-demand SessionGet -> SessionDetail cache (lazy: a first read for an un-hydrated session
// triggers the fetch and returns the best-known value; sessionDetailLoaded re-emits changed() so
// the routing manager re-reads). This replaces the inherited empty-seed mock behaviour for those
// two reads only.
class DaemonDaemonNet : public daemonnet::MockDaemonNet {
    Q_OBJECT

public:
    DaemonDaemonNet(RoutingRepository* routing, TransportRepository* transports,
                    persistence::ISessionStore* sessions, SessionRepository* sessionRepo,
                    QObject* parent = nullptr);

    // The pin dialog's session picker reads seed().sessions: project the REAL (cached) session
    // store instead of the mock's empty seed.
    [[nodiscard]] daemonnet::SeedBundle seed() const override;
    [[nodiscard]] QList<daemonnet::RoutingPin> routes() const override;
    [[nodiscard]] QList<daemonnet::BindingRule> bindingRules() const override;
    [[nodiscard]] QList<daemonnet::AccountAgent> accountsAgents() const override;
    [[nodiscard]] QList<daemonnet::RoomBinding>
    transportRooms(const domain::TransportId& transport) const override;
    [[nodiscard]] daemonnet::Resolution resolve(const domain::Origin& origin) const override;
    [[nodiscard]] QList<daemonnet::TransportTreeRow> transportsTree() const override;

    // CHA-9: node-authoritative per-session detail + outbound delivery targets, projected from the
    // SessionRepository's SessionGet cache (lazy-hydrated on first read / on focus).
    [[nodiscard]] domain::Session sessionDetail(const domain::SessionId& id) const override;
    [[nodiscard]] QList<domain::DeliveryTarget>
    deliveryTargets(const domain::SessionId& session) const override;

    void bindChat(const domain::Origin& origin, const domain::SessionId& session,
                  const domain::ProfileRef& profile) override;
    void unbindChat(const domain::Origin& origin) override;
    void handover(const domain::SessionId& session, const domain::DeliveryTarget& target) override;
    void bindAccount(const domain::TransportId& transport,
                     const domain::ProfileRef& profile) override;

private:
    RoutingRepository* m_routing = nullptr;
    TransportRepository* m_transports = nullptr;
    persistence::ISessionStore* m_sessions = nullptr;
    SessionRepository* m_sessionRepo = nullptr; // CHA-9 SessionGet detail hydration source
};

} // namespace daemonapp::daemon
