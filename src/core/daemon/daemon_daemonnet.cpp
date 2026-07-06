// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_daemonnet.h"

#include "daemon/repositories.h"
#include "persistence/isession_store.h"

namespace daemonapp::daemon {
namespace {

// domain::Origin -> the codec's flattened DecodedOrigin (what the Routing* encoders take).
DecodedOrigin toDecoded(const domain::Origin& o) {
    DecodedOrigin d;
    d.transport = o.transport.toString();
    switch (o.scope.kind) {
    case domain::OriginScopeKind::Dm:
        d.scopeKind = QStringLiteral("dm");
        d.user = o.scope.user;
        break;
    case domain::OriginScopeKind::Group:
        d.scopeKind = QStringLiteral("group");
        d.chat = o.scope.chat;
        d.hasThread = !o.scope.thread.isEmpty();
        d.thread = o.scope.thread;
        break;
    case domain::OriginScopeKind::Api:
        d.scopeKind = QStringLiteral("api");
        d.apiKey = o.scope.apiKey;
        break;
    case domain::OriginScopeKind::Internal:
        d.scopeKind = QStringLiteral("internal");
        break;
    }
    return d;
}

// The codec's DecodedOrigin -> domain::Origin (what the QML-facing DTOs carry).
domain::Origin toDomain(const DecodedOrigin& d) {
    domain::Origin o;
    o.transport = domain::TransportId(d.transport);
    if (d.scopeKind == QStringLiteral("dm")) {
        o.scope.kind = domain::OriginScopeKind::Dm;
        o.scope.user = d.user;
    } else if (d.scopeKind == QStringLiteral("group")) {
        o.scope.kind = domain::OriginScopeKind::Group;
        o.scope.chat = d.chat;
        o.scope.thread = d.hasThread ? d.thread : QString();
    } else if (d.scopeKind == QStringLiteral("api")) {
        o.scope.kind = domain::OriginScopeKind::Api;
        o.scope.apiKey = d.apiKey;
    } else {
        o.scope.kind = domain::OriginScopeKind::Internal;
    }
    return o;
}

} // namespace

DaemonDaemonNet::DaemonDaemonNet(RoutingRepository* routing, TransportRepository* transports,
                                 persistence::ISessionStore* sessions, QObject* parent)
    : daemonnet::MockDaemonNet(daemonnet::TransportsSeed::Empty, parent), m_routing(routing),
      m_transports(transports), m_sessions(sessions) {
    if (m_sessions != nullptr) {
        // The pin dialog's session picker projects the live store; re-announce on roster change.
        connect(m_sessions, &persistence::ISessionStore::changed, this, &IDaemonNet::changed);
    }
    if (m_routing != nullptr) {
        connect(m_routing, &RoutingRepository::routesRefreshed, this, &IDaemonNet::changed);
        connect(m_routing, &RoutingRepository::roomsRefreshed, this,
                [this](const QString&) { emit changed(); });
    }
    if (m_transports != nullptr) {
        // The accounts/rooms projections ride the transport reads: when the instance list lands,
        // fetch each account's bindable rooms so the pin picker + tree have them.
        connect(m_transports, &TransportRepository::instancesRefreshed, this, [this] {
            if (m_routing != nullptr) {
                const auto instances = m_transports->cachedInstances();
                for (const CachedTransportInstanceRow& row : instances) {
                    m_routing->refreshRooms(row.transport);
                }
            }
            emit changed();
        });
        connect(m_transports, &TransportRepository::conversationsRefreshed, this,
                [this](const QString&) { emit changed(); });
    }
}

daemonnet::SeedBundle DaemonDaemonNet::seed() const {
    daemonnet::SeedBundle bundle;
    if (m_sessions != nullptr) {
        // The AllSessions scope (non-archived): what a pin can sensibly target.
        bundle.sessions = m_sessions->sessions(domain::ListScope{});
    }
    return bundle;
}

QList<daemonnet::RoutingPin> DaemonDaemonNet::routes() const {
    QList<daemonnet::RoutingPin> pins;
    if (m_routing == nullptr) {
        return pins;
    }
    const QList<DecodedChatRoute>& routes = m_routing->routes();
    pins.reserve(routes.size());
    for (const DecodedChatRoute& r : routes) {
        daemonnet::RoutingPin pin;
        pin.origin = toDomain(r.origin);
        pin.session = domain::SessionId(r.session);
        pin.profile = domain::ProfileRef(r.profile);
        pin.isolation = r.isolation;
        pins.append(pin);
    }
    return pins;
}

QList<daemonnet::BindingRule> DaemonDaemonNet::bindingRules() const {
    // The config-time binding rules have NO wire read op at v28 — render none rather than the
    // mock's illustrative rows.
    return {};
}

QList<daemonnet::AccountAgent> DaemonDaemonNet::accountsAgents() const {
    QList<daemonnet::AccountAgent> out;
    if (m_transports == nullptr) {
        return out;
    }
    const auto instances = m_transports->cachedInstances();
    for (const CachedTransportInstanceRow& row : instances) {
        if (row.boundProfile.isEmpty()) {
            continue;
        }
        daemonnet::AccountAgent binding;
        binding.transport = domain::TransportId(row.transport);
        binding.profile = domain::ProfileRef(row.boundProfile);
        out.append(binding);
    }
    return out;
}

QList<daemonnet::RoomBinding>
DaemonDaemonNet::transportRooms(const domain::TransportId& transport) const {
    QList<daemonnet::RoomBinding> out;
    if (m_routing == nullptr) {
        return out;
    }
    const QList<DecodedRoomInfo> rooms = m_routing->roomsFor(transport.toString());
    out.reserve(rooms.size());
    for (const DecodedRoomInfo& r : rooms) {
        daemonnet::RoomBinding binding;
        binding.transport = domain::TransportId(r.transport);
        binding.chat = r.room;
        binding.label = r.name.isEmpty() ? r.room : r.name;
        binding.pinnedSession = domain::SessionId(r.session);
        out.append(binding);
    }
    return out;
}

daemonnet::Resolution DaemonDaemonNet::resolve(const domain::Origin& origin) const {
    // Thin client: answer from the PIN table only. The node owns the full precedence
    // (rules/account-bound/default) and exposes no read op for the lower rungs at v28, so an
    // unpinned origin honestly resolves as Default with no fabricated session.
    daemonnet::Resolution res;
    const QString key = daemonnet::originKey(origin);
    const auto pins = routes();
    for (const daemonnet::RoutingPin& pin : pins) {
        if (daemonnet::originKey(pin.origin) == key) {
            res.session = pin.session;
            res.profile = pin.profile;
            res.decidedBy = daemonnet::DecidedBy::Pin;
            return res;
        }
    }
    res.decidedBy = daemonnet::DecidedBy::Default;
    return res;
}

QList<daemonnet::TransportTreeRow> DaemonDaemonNet::transportsTree() const {
    // B6: inert (empty). The live projection (instances + conversations + pins -> the sidebar
    // Integrations tree) lands with the roster merge (B4).
    return daemonnet::MockDaemonNet::transportsTree();
}

void DaemonDaemonNet::bindChat(const domain::Origin& origin, const domain::SessionId& session,
                               const domain::ProfileRef& profile) {
    if (m_routing != nullptr) {
        // Node-authoritative: RoutingBindChat -> Ok -> re-list (changed() fires on the refresh).
        m_routing->bindChat(toDecoded(origin), session.toString(), profile.toString());
    }
}

void DaemonDaemonNet::unbindChat(const domain::Origin& origin) {
    if (m_routing != nullptr) {
        m_routing->unbindChat(toDecoded(origin));
    }
}

void DaemonDaemonNet::handover(const domain::SessionId& session,
                               const domain::DeliveryTarget& target) {
    // Not wired in this slice (the wire `handover` op exists; the delivery panel re-point is a
    // follow-up). An inert no-op beats the mock's fake-success local mutation.
    Q_UNUSED(session)
    Q_UNUSED(target)
}

void DaemonDaemonNet::bindAccount(const domain::TransportId& transport,
                                  const domain::ProfileRef& profile) {
    // Not wired in this slice (account->agent baselines mutate via profile_update
    // bound_accounts; the ProfileEditor owns that flow). Inert, never fake-success.
    Q_UNUSED(transport)
    Q_UNUSED(profile)
}

} // namespace daemonapp::daemon
