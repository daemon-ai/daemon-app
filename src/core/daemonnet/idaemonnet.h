#pragma once

#include "domain/ids.h"
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
// tree), the sessions (each with its authoritative `sessionId`, content, tags, owning unit), and the
// client-local tags. Wire-conformant (`domain::UnitNode`/`Session`/`Tag` mirror the NodeApi shapes),
// so a future daemon adapter fills the same bundle from `Tree`/`SessionsQuery`/`Subscribe`.
struct SeedBundle {
    QList<domain::UnitNode> units;
    QList<domain::Session> sessions;
    QList<domain::Tag> tags;
};

// The DaemonNet seam (daemon-app/docs/multi-protocol-client-surface.md §1): the daemon's network of
// actors (Agents / Peers / Users) and places (Rooms / Channels) joined by
// Runs/Over/Participant/InPlace/Delegation edges, with **Sessions as first-class nodes**. It is the
// single mock source every surface projects from. Projections are exposed as VariantListModels (rows
// read in QML as `entry.<field>`); the raw node/edge lists feed the future patch-bay. Mock-only
// today; a daemon adapter joining the read-only NodeApi projection endpoints replaces it later (the
// DaemonNet lives client-side, spec §1.5).
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
    // sessions/units/transports, not chat-app contacts. They are CLIENT-DERIVED projections computed
    // from each session's `Origin` / `DeliveryTarget` / participant edges (the multi-protocol-messenger
    // lens, multi-protocol-client-surface.md §1). A future daemon adapter computes them client-side too;
    // it does not fetch them.
    [[nodiscard]] virtual QObject* channels() const = 0;
    // By-Peer projection: conversations grouped by remote participant, rows
    // { id, peer, kind(nature), count }. Client-derived (see the note above).
    [[nodiscard]] virtual QObject* byPeer() const = 0;

    // Raw graph for the future patch-bay: nodes { id, kind, label, ... }; edges
    // { id, source, target, edgeKind, role }.
    [[nodiscard]] Q_INVOKABLE virtual QVariantList nodes() const = 0;
    [[nodiscard]] Q_INVOKABLE virtual QVariantList edges() const = 0;

    // The typed seed bundle the session store copies (units + sessions + tags). The single source for
    // the sidebar/list/transcript; the projections above are derived from the same data.
    [[nodiscard]] virtual SeedBundle seed() const = 0;

    // --- Typed read seam (the single source surfaced as a formal API) ---
    // Mirrors the read side of persistence::ISessionStore, but SessionId-keyed where identity matters,
    // so a later DaemonNetSessionStore (roadmap P3a) can delegate here and a daemon adapter can fill
    // the same shapes from the read-only NodeApi projection endpoints. All are O(n) over the seed.

    // Direct children of `parent` (empty parent = the top-level roots), mirroring TreeReport.
    [[nodiscard]] virtual QList<domain::UnitNode>
    unitChildren(const domain::UnitId& parent) const = 0;
    // One unit by id (invalid UnitNode if unknown).
    [[nodiscard]] virtual domain::UnitNode unit(const domain::UnitId& id) const = 0;
    // The client-local cross-cutting tags.
    [[nodiscard]] virtual QList<domain::Tag> tags() const = 0;
    // Sessions matching a sidebar scope: All/Archived, a unit's whole subtree (Unit), a Tag, or the
    // DaemonNet lens scopes ByTransport/ByPeer (grouping by `scope.lensKey`).
    [[nodiscard]] virtual QList<domain::Session>
    sessionsInScope(const domain::ListScope& scope) const = 0;
    // One session's full metadata by its authoritative SessionId (invalid Session if unknown).
    [[nodiscard]] virtual domain::Session
    sessionDetail(const domain::SessionId& id) const = 0;
    // The session's transcript markdown by SessionId (roadmap P4 swaps this for a SessionLogEntry
    // sequence; the seam shape stays SessionId-keyed).
    [[nodiscard]] virtual QString content(const domain::SessionId& id) const = 0;

signals:
    void changed();
};

} // namespace daemonnet
