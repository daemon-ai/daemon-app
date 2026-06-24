#pragma once

#include "domain/ids.h"

#include <QMetaType>
#include <QString>

namespace domain {

// What a node in the supervision tree is, for display only. Mirrors
// `daemon-protocol::UnitKind`. This is COSMETIC: it picks an icon and nothing
// else. Structure (who contains whom, to any depth) is driven purely by
// parent/child links, never by kind. A foreign agent and a daemon-core engine are
// both `Engine` - the GUI cannot, and need not, tell them apart.
enum class UnitKind {
    Engine,       // a leaf brain (daemon-core engine or a foreign agent)
    Host,         // a host running a unit
    Orchestrator, // an engine that also supervises children (a sub-fleet)
};

// A unit's lifecycle state, mirroring `daemon-protocol::UnitState` (rendered).
enum class UnitState {
    Running,  // attached, no terminal outcome yet
    Finished, // reached a terminal outcome
    Unknown,  // state could not be resolved
};

// A unit's hierarchy role, mirroring `daemon-protocol::SessionRole`. Lets a
// client keep stable nodes pinned and collapse ephemeral subagent churn.
enum class SessionRole {
    Primary,           // a top-level conversation (an inbox/operator session)
    ManagedChild,      // a long-lived delegated child, stable in the tree
    EphemeralSubagent, // a transient subagent (high churn)
};

// One uniform node in the supervision tree, mirroring `daemon-protocol::UnitNode`.
// The tree is arbitrary-depth and uniformly recursive: ANY node may contain ANY
// number of children, to ANY depth and composition. A root may be a lone unit.
//
// There is intentionally NO stored depth or "is-leaf" flag: leaf-ness is computed
// from whether the node has children. `id` is a `UnitId` mirroring the daemon
// `UnitId`; `parentId` empty means a top-level root. The unit's agent identity is
// its `profile` (a `ProfileRef`); a unit RUNS UNDER a profile (they are not the
// same - many units can share one profile).
struct UnitNode {
    UnitId id;
    UnitId parentId; // empty == root
    QString name;
    UnitKind kind = UnitKind::Engine;
    UnitState state = UnitState::Unknown;
    QString work; // short description of current work, when known
    // The profile this unit's engine runs under == the agent identity
    // (`UnitNode.profile`). Memory + profile settings are keyed by this. Empty
    // when the unit has no resolvable profile (e.g. a Host or a synthetic root).
    ProfileRef profile = {};
    // The session id backing this unit when it maps to one (`UnitNode.session`).
    // On the durable path `UnitId == SessionId`, so this usually equals `id`.
    SessionId session = {};
    // This unit's hierarchy role (`UnitNode.role`).
    SessionRole role = SessionRole::Primary;
    // The terminal end reason when `state == Finished` (`UnitState::Finished`).
    QString endReason = {};

    [[nodiscard]] bool isValid() const { return !id.isEmpty(); }
};

} // namespace domain

Q_DECLARE_METATYPE(domain::UnitNode)
