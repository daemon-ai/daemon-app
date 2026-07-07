// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_daemonnet.h"

#include "daemon/repositories.h"
#include "domain/delivery.h"
#include "domain/session.h"
#include "persistence/isession_store.h"

namespace daemonapp::daemon {
namespace {

// CHA-9: project a hydrated SessionInfo row into the domain::Session the IDaemonNet seam returns.
// Mirrors CachedSessionStore's row projection (the string->enum mapping the node's wire strings
// use); kept local to avoid exporting that store's file-private helpers.
domain::Session sessionFromRow(const CachedSessionRow& row) {
    domain::Session session;
    session.sessionId = domain::SessionId(row.sessionId);
    session.title = row.title;
    session.isArchived = row.archived;
    session.isPinned = row.pinned;
    session.boundProfile = domain::ProfileRef(row.profileRef);
    if (row.state == QStringLiteral("Active")) {
        session.state = domain::SessionState::Active;
    } else if (row.state == QStringLiteral("Suspended")) {
        session.state = domain::SessionState::Suspended;
    } else if (row.state == QStringLiteral("Ready")) {
        session.state = domain::SessionState::Ready;
    } else if (row.state == QStringLiteral("Completed")) {
        session.state = domain::SessionState::Completed;
    }
    session.lifecycle = row.lifecycle == QStringLiteral("Live") ? domain::Lifecycle::Live
                                                                : domain::Lifecycle::Durable;
    if (row.role == QStringLiteral("ManagedChild")) {
        session.role = domain::SessionRole::ManagedChild;
    } else if (row.role == QStringLiteral("EphemeralSubagent")) {
        session.role = domain::SessionRole::EphemeralSubagent;
    }
    if (!row.parentSessionId.isEmpty()) {
        session.parent = domain::SessionId(row.parentSessionId);
    }
    return session;
}

// The wire SinkKind string -> the domain enum.
domain::SinkKind sinkKindFromString(const QString& kind) {
    return kind == QStringLiteral("Spectator") ? domain::SinkKind::Spectator
                                               : domain::SinkKind::Primary;
}

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
                                 persistence::ISessionStore* sessions,
                                 SessionRepository* sessionRepo, QObject* parent)
    : daemonnet::MockDaemonNet(daemonnet::TransportsSeed::Empty, parent), m_routing(routing),
      m_transports(transports), m_sessions(sessions), m_sessionRepo(sessionRepo) {
    if (m_sessions != nullptr) {
        // The pin dialog's session picker projects the live store; re-announce on roster change.
        connect(m_sessions, &persistence::ISessionStore::changed, this, &IDaemonNet::changed);
    }
    if (m_sessionRepo != nullptr) {
        // CHA-9: a lazily-hydrated SessionDetail landing must re-announce so the routing manager's
        // delivery panel (and any other detail reader) re-projects the now-known targets.
        connect(m_sessionRepo, &SessionRepository::sessionDetailLoaded, this,
                [this](const QString&) { emit changed(); });
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
    // The roster merge (B4/EIO-8): live external conversations appear in the unified sidebar
    // beside internal sessions — account -> convGroup (Channels/DMs) -> conversation leaves,
    // following the taxonomy the mock established. Dedup vs pins: a conversation with a
    // ChatRoute pin renders ONE leaf carrying the pinned sessionId (activation opens the
    // transcript); an unpinned one is a browse-only leaf (activation opens the Channels page —
    // pins stay explicit routing state, never a lazy SessionCreate).
    QList<daemonnet::TransportTreeRow> tree;
    if (m_transports == nullptr) {
        return tree;
    }
    const QList<CachedTransportInstanceRow> instances = m_transports->cachedInstances();
    for (const CachedTransportInstanceRow& account : instances) {
        const QString accountId = QStringLiteral("tx:") + account.transport;
        const QList<CachedConversationRow> convs =
            m_transports->cachedConversations(account.transport);

        daemonnet::TransportTreeRow acct;
        acct.depth = 0;
        acct.id = accountId;
        acct.kind = QStringLiteral("account");
        acct.label = account.displayName.isEmpty() ? account.transport : account.displayName;
        acct.scopeKey = account.transport; // ByTransport list scope
        acct.presence = account.presence;
        acct.hasChildren = !convs.isEmpty();
        acct.transportId = account.transport;
        tree.append(acct);
        if (convs.isEmpty()) {
            continue;
        }

        // Split by conversation type: channels/group chats vs 1:1 DMs (the mock's taxonomy).
        QList<CachedConversationRow> channels;
        QList<CachedConversationRow> dms;
        for (const CachedConversationRow& c : convs) {
            (c.kind == QStringLiteral("dm") ? dms : channels).append(c);
        }
        const auto appendGroup = [&](const QString& groupKey, const QString& label,
                                     const QList<CachedConversationRow>& rows) {
            if (rows.isEmpty()) {
                return;
            }
            daemonnet::TransportTreeRow group;
            group.depth = 1;
            group.id = accountId + QLatin1Char('/') + groupKey;
            group.parentId = accountId;
            group.kind = QStringLiteral("convGroup");
            group.label = label;
            group.hasChildren = true;
            group.transportId = account.transport;
            tree.append(group);
            for (const CachedConversationRow& c : rows) {
                daemonnet::TransportTreeRow leaf;
                leaf.depth = 2;
                leaf.id = accountId + QStringLiteral("/c/") + c.convId;
                leaf.parentId = group.id;
                leaf.kind = QStringLiteral("conversation");
                leaf.convType = c.kind.isEmpty() ? QStringLiteral("channel") : c.kind;
                leaf.label = c.title.isEmpty() ? c.convId : c.title;
                leaf.transportId = account.transport;
                leaf.conversationId = c.convId;
                // Pin dedup: a pinned conversation IS its session leaf (one row, no duplicate).
                leaf.sessionId = c.kind == QStringLiteral("dm")
                                     ? pinnedDmSessionFor(account.transport, c.convId)
                                     : pinnedSessionFor(account.transport, c.convId);
                tree.append(leaf);
            }
        };
        appendGroup(QStringLiteral("ch"), tr("Channels"), channels);
        appendGroup(QStringLiteral("dm"), tr("DMs"), dms);
    }
    return tree;
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

domain::Session DaemonDaemonNet::sessionDetail(const domain::SessionId& id) const {
    if (m_sessionRepo == nullptr || id.isEmpty()) {
        return {};
    }
    DecodedSessionDetail detail;
    if (m_sessionRepo->cachedDetail(id.toString(), &detail)) {
        return sessionFromRow(detail.info);
    }
    // Lazy: kick off the hydration; sessionDetailLoaded -> changed() re-drives the read.
    m_sessionRepo->getSessionDetail(id.toString());
    return {};
}

QList<domain::DeliveryTarget>
DaemonDaemonNet::deliveryTargets(const domain::SessionId& session) const {
    QList<domain::DeliveryTarget> out;
    if (m_sessionRepo == nullptr || session.isEmpty()) {
        return out;
    }
    DecodedSessionDetail detail;
    if (!m_sessionRepo->cachedDetail(session.toString(), &detail)) {
        // Lazy: kick off the hydration; sessionDetailLoaded -> changed() re-drives the read.
        m_sessionRepo->getSessionDetail(session.toString());
        return out;
    }
    for (const DecodedDeliveryTarget& dt : detail.deliveryTargets) {
        domain::DeliveryTarget target;
        target.transport = domain::TransportId(dt.transport);
        target.route = domain::RouteAddr(dt.route);
        target.kind = sinkKindFromString(dt.kind);
        out.append(target);
    }
    return out;
}

} // namespace daemonapp::daemon
