#pragma once

#include <QString>

namespace daemonnet {

// The DaemonNet node/edge kinds (daemon-app/docs/multi-protocol-client-surface.md §1). A **Session
// is a first-class node** (own id/transcript/lifecycle); "an agent has many sessions" is N `Runs`
// edges from one Agent node to its Session nodes. Actor kinds: `Agent` (daemon-managed), `Peer` (an
// external party reached over a transport whose nature we cannot verify), `User` (a daemon
// operator/principal — the Admin is the implicit omnipresent viewer and is NOT a node).
enum class NodeKind {
    Agent,
    Peer,
    User,
    Session,
    Room,
    Channel,
    Transport,
    Host,
};

enum class EdgeKind {
    Runs,        // Agent -> Session (the agent-has-many-sessions 1:many)
    Over,        // Session -> Transport
    Participant, // Session -> Peer/Agent/User (role author|primary|spectator)
    InPlace,     // Session -> Room/Channel
    Delegation,  // Agent -> Agent
};

[[nodiscard]] inline QString nodeKindStr(NodeKind k)
{
    switch (k) {
    case NodeKind::Agent: return QStringLiteral("agent");
    case NodeKind::Peer: return QStringLiteral("peer");
    case NodeKind::User: return QStringLiteral("user");
    case NodeKind::Session: return QStringLiteral("session");
    case NodeKind::Room: return QStringLiteral("room");
    case NodeKind::Channel: return QStringLiteral("channel");
    case NodeKind::Transport: return QStringLiteral("transport");
    case NodeKind::Host: return QStringLiteral("host");
    }
    return QStringLiteral("unknown");
}

[[nodiscard]] inline QString edgeKindStr(EdgeKind k)
{
    switch (k) {
    case EdgeKind::Runs: return QStringLiteral("runs");
    case EdgeKind::Over: return QStringLiteral("over");
    case EdgeKind::Participant: return QStringLiteral("participant");
    case EdgeKind::InPlace: return QStringLiteral("inPlace");
    case EdgeKind::Delegation: return QStringLiteral("delegation");
    }
    return QStringLiteral("unknown");
}

} // namespace daemonnet
