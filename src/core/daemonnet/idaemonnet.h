// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "daemonnet/routing_dtos.h"
#include "domain/delivery.h"
#include "domain/ids.h"
#include "domain/origin.h"
#include "domain/participant.h"
#include "domain/session.h"
#include "domain/sidebar_node.h"
#include "domain/tag.h"
#include "domain/unit_node.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QVariantList>

namespace daemonnet {

// The typed seed the session store copies at construction: the supervision units (the delegation
// tree), the sessions (each with its authoritative `sessionId`, content, tags, owning unit), and
// the client-local tags. Wire-conformant (`domain::UnitNode`/`Session`/`Tag` mirror the NodeApi
// shapes), so a future daemon adapter fills the same bundle from
// `Tree`/`SessionsQuery`/`Subscribe`.
struct SeedBundle {
    QList<domain::UnitNode> units;
    QList<domain::Session> sessions;
    QList<domain::Tag> tags;
    QList<domain::Participant> participants;
};

// One pre-order row of the Transports tree (the events-IO axis: transport account ->
// capability-driven taxonomy -> session leaf). The transport's adapter capabilities declare the
// shape (daemon-messaging-adapter-spec.md): a MESSAGING transport (matrix/rooms) expands to its
// conversations grouped by ConversationType; a GENERIC transport (cron/http) expands to its
// origin-tagged sessions. `kind` discriminates the row; a row with a non-empty `sessionId` is an
// openable session leaf (the shared leaf, cross-linked to the Fleet tree).
struct TransportTreeRow {
    int depth = 0; // 0 = transport account/instance
    QString id;    // stable node id (expand/collapse + selection): "tx:matrix", "tx:matrix/ch", ...
    QString parentId;  // owning row id ("" for account rows)
    QString kind;      // "account" | "convGroup" | "conversation" | "job" | "caller"
    QString convType;  // conversation kind: "channel" | "groupdm" | "dm" | "thread" (else "")
    QString label;     // display label ("matrix /@bot:hs.org", "Channels", "#secops", ...)
    QString sublabel;  // inline session title / "(N agents)" (else "")
    QString sessionId; // openable session leaf (empty = no active conversation/session)
    QString scopeKey; // ByTransport/ByPeer key (transport instance / peer id) for account/conv rows
    QString presence; // account rows: PresencePrimitive ("available"/"away"/... ; else "")
    int memberCount = 0; // occupant count badge (0 = none)
    bool hasChildren = false;
    // Conversation rows (B4): the owning transport instance + the conversation id, so an
    // UNPINNED leaf's activation can open the Channels page scoped to it (browse-only — pins
    // stay explicit routing state; no lazy SessionCreate).
    QString transportId;
    QString conversationId;
};

// The DaemonNet seam (daemon-app/docs/multi-protocol-client-surface.md §1): the daemon's network of
// actors (Agents / Peers / Users) and places (Rooms / Channels) joined by
// Runs/Over/Participant/InPlace/Delegation edges, with **Sessions as first-class nodes**. It is the
// single mock source every surface projects from. Projections are exposed as VariantListModels
// (rows read in QML as `entry.<field>`); the raw node/edge lists feed the future patch-bay.
// Mock-only today; a daemon adapter joining the read-only NodeApi projection endpoints replaces it
// later (the DaemonNet lives client-side, spec §1.5).
class IDaemonNet : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* fleet READ fleet CONSTANT)
    Q_PROPERTY(QObject* sessions READ sessions CONSTANT)
    Q_PROPERTY(QObject* channels READ channels CONSTANT)
    Q_PROPERTY(QObject* byPeer READ byPeer CONSTANT)

public:
    using QObject::QObject;
    ~IDaemonNet() override = default;

    // Fleet projection: the delegation spanning tree (pre-order), rows
    // { depth, id, name, kind(orchestrator|worker), status, model } — the IFleetTree row contract.
    [[nodiscard]] virtual QObject* fleet() const = 0;
    // Flat session projection (the roster), rows
    // { id, title, profile, state, lifecycle, lastActivity, tokens, rewindable } — ISessionRoster.
    [[nodiscard]] virtual QObject* sessions() const = 0;
    // Channels projection: one row per conversation grouped by transport, rows
    // { id, transport, peer, scope(dm|group|solo), presence, session }.
    //
    // NOTE: `Channel` / `Peer` / `User` / `By-Peer` have NO wire equivalent - the NodeApi exposes
    // sessions/units/transports, not chat-app contacts. They are CLIENT-DERIVED projections
    // computed from each session's `Origin` / `DeliveryTarget` / participant edges (the
    // multi-protocol-messenger lens, multi-protocol-client-surface.md §1). A future daemon adapter
    // computes them client-side too; it does not fetch them.
    [[nodiscard]] virtual QObject* channels() const = 0;
    // By-Peer projection: conversations grouped by remote participant, rows
    // { id, peer, kind(nature), count }. Client-derived (see the note above).
    [[nodiscard]] virtual QObject* byPeer() const = 0;

    // Raw graph for the future patch-bay: nodes { id, kind, label, ... }; edges
    // { id, source, target, edgeKind, role }.
    [[nodiscard]] Q_INVOKABLE virtual QVariantList nodes() const = 0;
    [[nodiscard]] Q_INVOKABLE virtual QVariantList edges() const = 0;

    // The typed seed bundle the session store copies (units + sessions + tags). The single source
    // for the sidebar/list/transcript; the projections above are derived from the same data.
    [[nodiscard]] virtual SeedBundle seed() const = 0;

    // --- Typed read seam (the single source surfaced as a formal API) ---
    // Mirrors the read side of persistence::ISessionStore, but SessionId-keyed where identity
    // matters, so a later DaemonNetSessionStore (roadmap P3a) can delegate here and a daemon
    // adapter can fill the same shapes from the read-only NodeApi projection endpoints. All are
    // O(n) over the seed.

    // Direct children of `parent` (empty parent = the top-level roots), mirroring TreeReport.
    [[nodiscard]] virtual QList<domain::UnitNode>
    unitChildren(const domain::UnitId& parent) const = 0;
    // One unit by id (invalid UnitNode if unknown).
    [[nodiscard]] virtual domain::UnitNode unit(const domain::UnitId& id) const = 0;
    // The client-local cross-cutting tags.
    [[nodiscard]] virtual QList<domain::Tag> tags() const = 0;
    // The participants of the active chat/transcript (the daemon `ConversationMember` roster,
    // client-derived). Mock today; a daemon adapter fills the same shape from
    // `conv_get`/`conv_list`.
    [[nodiscard]] virtual QList<domain::Participant> participants() const = 0;
    // Sessions matching a sidebar scope: All/Archived, a unit's whole subtree (Unit), a Tag, or the
    // DaemonNet lens scopes ByTransport/ByPeer (grouping by `scope.lensKey`).
    [[nodiscard]] virtual QList<domain::Session>
    sessionsInScope(const domain::ListScope& scope) const = 0;
    // One session's full metadata by its authoritative SessionId (invalid Session if unknown).
    [[nodiscard]] virtual domain::Session sessionDetail(const domain::SessionId& id) const = 0;
    // The session's transcript markdown by SessionId (roadmap P4 swaps this for a SessionLogEntry
    // sequence; the seam shape stays SessionId-keyed).
    [[nodiscard]] virtual QString content(const domain::SessionId& id) const = 0;

    // The Transports tree (events-IO axis): a flattened pre-order list of TransportTreeRow, each
    // transport instance expanding into its capability-declared taxonomy down to session leaves.
    // The sidebar renders this as a co-equal section beside the Fleet tree.
    [[nodiscard]] virtual QList<TransportTreeRow> transportsTree() const = 0;

    // --- Routing (the routing-manager surface; routing-manager-design.md) ---
    // Reads = projections of the routing state; writes = mutations (mock today, the wire
    // `routing_*`/`delivery_*`/`handover` ops later). All SessionId/Origin-keyed.

    // The explicit chat pins (origin -> session [+agent]); mirrors `routing_list_chats`.
    [[nodiscard]] virtual QList<RoutingPin> routes() const = 0;
    // The ordered, config-time binding rules (read-only; no runtime wire CRUD).
    [[nodiscard]] virtual QList<BindingRule> bindingRules() const = 0;
    // The account -> agent baseline bindings (`instance_profiles`).
    [[nodiscard]] virtual QList<AccountAgent> accountsAgents() const = 0;
    // A session's outbound delivery targets (exactly one Primary); mirrors `delivery_targets`.
    [[nodiscard]] virtual QList<domain::DeliveryTarget>
    deliveryTargets(const domain::SessionId& session) const = 0;
    // The bindable rooms/chats on a transport instance (+ pinned session); mirrors
    // `transport_rooms`.
    [[nodiscard]] virtual QList<RoomBinding>
    transportRooms(const domain::TransportId& transport) const = 0;
    // Resolve an origin to {session, profile, delivery} + which rung decided it (the explainer).
    // Mock-complete over the full precedence (pin > rule > account-bound > default).
    [[nodiscard]] virtual Resolution resolve(const domain::Origin& origin) const = 0;

    // --- QML chip helpers (concrete; derived from routes()) ------------------------------------
    // The pins targeting `sessionId`, as QML rows { key, transport, label } — the session-header
    // route chip (B6/EIO-12) binds this. Empty when the session is not a pin target.
    [[nodiscard]] Q_INVOKABLE QVariantList pinsForSession(const QString& sessionId) const {
        QVariantList rows;
        const QList<RoutingPin> pins = routes();
        for (const RoutingPin& pin : pins) {
            if (pin.session.toString() != sessionId) {
                continue;
            }
            QVariantMap row;
            row[QStringLiteral("key")] = originKey(pin.origin);
            row[QStringLiteral("transport")] = pin.origin.transport.toString();
            switch (pin.origin.scope.kind) {
            case domain::OriginScopeKind::Dm:
                row[QStringLiteral("label")] = QStringLiteral("@") + pin.origin.scope.user;
                break;
            case domain::OriginScopeKind::Group:
                row[QStringLiteral("label")] = pin.origin.scope.chat;
                break;
            case domain::OriginScopeKind::Api:
                row[QStringLiteral("label")] = QStringLiteral("api:") + pin.origin.scope.apiKey;
                break;
            case domain::OriginScopeKind::Internal:
                row[QStringLiteral("label")] = QStringLiteral("internal");
                break;
            }
            rows.append(row);
        }
        return rows;
    }
    // The session a (transport, chat) group origin is pinned to ("" when unpinned) — the room-row
    // route chip binds this.
    [[nodiscard]] Q_INVOKABLE QString pinnedSessionFor(const QString& transport,
                                                       const QString& chat) const {
        const QList<RoutingPin> pins = routes();
        for (const RoutingPin& pin : pins) {
            if (pin.origin.transport.toString() == transport &&
                pin.origin.scope.kind == domain::OriginScopeKind::Group &&
                pin.origin.scope.chat == chat) {
                return pin.session.toString();
            }
        }
        return {};
    }
    // The session a (transport, user) DM origin is pinned to ("" when unpinned).
    [[nodiscard]] Q_INVOKABLE QString pinnedDmSessionFor(const QString& transport,
                                                         const QString& user) const {
        const QList<RoutingPin> pins = routes();
        for (const RoutingPin& pin : pins) {
            if (pin.origin.transport.toString() == transport &&
                pin.origin.scope.kind == domain::OriginScopeKind::Dm &&
                pin.origin.scope.user == user) {
                return pin.session.toString();
            }
        }
        return {};
    }

    // Pin an origin to a session (+ optional agent); mirrors `routing_bind_chat`.
    virtual void bindChat(const domain::Origin& origin, const domain::SessionId& session,
                          const domain::ProfileRef& profile) = 0;
    // Remove an origin's pin; mirrors `routing_unbind_chat`.
    virtual void unbindChat(const domain::Origin& origin) = 0;
    // Re-point a session's Primary delivery (prior Primary demotes to Spectator); mirrors
    // `handover`.
    virtual void handover(const domain::SessionId& session,
                          const domain::DeliveryTarget& target) = 0;
    // Bind a transport account to an agent baseline; mirrors `profile_update(bound_accounts)`.
    virtual void bindAccount(const domain::TransportId& transport,
                             const domain::ProfileRef& profile) = 0;

signals:
    void changed();
};

} // namespace daemonnet
