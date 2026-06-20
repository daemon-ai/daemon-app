#pragma once

#include <QMetaType>
#include <QString>

namespace domain {

// What a node in the supervision tree is, for display only. Mirrors
// `daemon_api::UnitKind`. This is COSMETIC: it picks an icon and nothing else.
// Structure (who contains whom, to any depth) is driven purely by parent/child
// links, never by kind. A foreign agent and a daemon-core engine are both
// `Engine` - the GUI cannot, and need not, tell them apart.
enum class AgentNodeKind {
    Engine,       // a leaf brain (daemon-core engine or a foreign agent)
    Host,         // a host running a unit
    Orchestrator, // an engine that also supervises children (a sub-fleet)
};

// A node's lifecycle state, mirroring `daemon_api::UnitState` (rendered).
enum class AgentState {
    Running,  // attached, no terminal outcome yet
    Finished, // reached a terminal outcome
    Unknown,  // state could not be resolved
};

// One uniform node in the agent supervision tree, mirroring
// `daemon_api::UnitNode`. The tree is arbitrary-depth and uniformly recursive:
// ANY node may contain ANY number of children, to ANY depth and composition
// (fleet -> fleet -> agent -> fleet -> ...). A root may be a lone agent.
//
// There is intentionally NO stored depth or "is-leaf" flag: leaf-ness is
// computed from whether the node has children. `id` is a string to mirror the
// daemon `UnitId`; `parentId` empty means a top-level root.
struct AgentNode {
    QString id;
    QString parentId; // empty == root
    QString name;
    AgentNodeKind kind = AgentNodeKind::Engine;
    AgentState state = AgentState::Unknown;
    QString work; // short description of current work, when known

    [[nodiscard]] bool isValid() const { return !id.isEmpty(); }
};

} // namespace domain

Q_DECLARE_METATYPE(domain::AgentNode)
