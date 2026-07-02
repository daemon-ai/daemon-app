// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "persistence/in_memory_session_store.h"

#include "daemonnet/idaemonnet.h"
#include "daemonnet/seed_transcripts.h"

#include <algorithm>
#include <QDateTime>
#include <QUuid>

namespace persistence {

using domain::ListScope;
using domain::NodeType;
using domain::ProfileRef;
using domain::Session;
using domain::SessionId;
using domain::SessionRole;
using domain::Tag;
using domain::UnitId;
using domain::UnitKind;
using domain::UnitNode;
using domain::UnitState;

void InMemorySessionStore::applyUnitMeta(UnitNode& n) {
    // UnitId == SessionId on the durable path.
    n.session = SessionId(n.id.toString());

    // Role: roots are the top-level (Primary) sessions; children are managed,
    // with the deepest demo worker flagged ephemeral to exercise that styling.
    if (n.parentId.isEmpty()) {
        n.role = SessionRole::Primary;
    } else if (n.id == UnitId(QStringLiteral("n-worker"))) {
        n.role = SessionRole::EphemeralSubagent;
    } else {
        n.role = SessionRole::ManagedChild;
    }

    // Hosts are substrate, not profile-backed agents (mirrors the daemon: Host
    // units carry no profile). Everything else binds to a seeded profile id.
    if (n.kind == UnitKind::Host) {
        n.profile = ProfileRef();
        return;
    }
    static const QHash<QString, QString> kUnitProfiles = {
        {QStringLiteral("n-scratch"), QStringLiteral("prof-1")},
        {QStringLiteral("n-acme"), QStringLiteral("prof-1")},
        {QStringLiteral("n-build"), QStringLiteral("prof-1")},
        {QStringLiteral("n-coder"), QStringLiteral("prof-2")},
        {QStringLiteral("n-worker"), QStringLiteral("prof-2")},
        {QStringLiteral("n-review"), QStringLiteral("prof-3")},
        {QStringLiteral("n-deep"), QStringLiteral("prof-3")},
    };
    n.profile = ProfileRef(kUnitProfiles.value(n.id.toString(), QStringLiteral("prof-1")));
}

InMemorySessionStore::InMemorySessionStore(QObject* parent) : InMemorySessionStore(parent, true) {}

InMemorySessionStore::InMemorySessionStore(QObject* parent, bool seed) : ISessionStore(parent) {
    if (seed) {
        seedSampleData();
    }
}

InMemorySessionStore::InMemorySessionStore(daemonnet::IDaemonNet* net, QObject* parent)
    : ISessionStore(parent) {
    seedFromDaemonNet(net);
}

void InMemorySessionStore::seedFromDaemonNet(daemonnet::IDaemonNet* net) {
    if (net == nullptr) {
        return;
    }
    const daemonnet::SeedBundle bundle = net->seed();
    m_units = bundle.units;
    m_tags = bundle.tags;
    m_participants = bundle.participants;
    // Copy the sessions, assigning each a stable local int handle (deterministic order) while
    // keeping its authoritative string sessionId. The int handle is transitional (the pipeline
    // migrates to SessionId in P3).
    m_sessions.clear();
    m_sessions.reserve(bundle.sessions.size());
    int nextId = 1;
    for (Session s : bundle.sessions) {
        s.id = nextId++;
        m_sessions.push_back(s);
    }
    m_nextId = nextId;
    int maxTag = 0;
    for (const Tag& t : m_tags) {
        maxTag = std::max(maxTag, t.id);
    }
    m_nextTagId = maxTag + 1;
}

void InMemorySessionStore::seedSampleData() {
    // A fleet-of-fleets that deliberately exercises the recursion:
    //  - "scratchpad" is a LONE root unit (a tree of one, no children).
    //  - "Acme Platform" is a deep root: Orchestrator -> Orchestrator ("Build
    //    Fleet") -> Orchestrator ("Deep Fleet") -> Engine, i.e. orchestrators
    //    nested inside orchestrators (no fixed "fleet/agent/subagent" shape).
    //  - sessions hang off units at multiple depths, including off intermediate
    //    orchestrator units (their own reasoning transcript).
    const auto mkUnit = [](const char* id, const char* parent, const char* name, UnitKind kind,
                           UnitState state, const char* work) {
        UnitNode n;
        n.id = UnitId(QString::fromLatin1(id));
        const QString p = QString::fromLatin1(parent);
        n.parentId = p.isEmpty() ? UnitId() : UnitId(p);
        n.name = QString::fromUtf8(name);
        n.kind = kind;
        n.state = state;
        n.work = QString::fromUtf8(work);
        return n;
    };
    m_units = {
        mkUnit("n-scratch", "", "scratchpad", UnitKind::Engine, UnitState::Running, ""),
        mkUnit("n-acme", "", "Acme Platform", UnitKind::Orchestrator, UnitState::Running,
               "Coordinating release"),
        mkUnit("n-build", "n-acme", "Build Fleet", UnitKind::Orchestrator, UnitState::Running,
               "Dispatching work"),
        mkUnit("n-coder", "n-build", "Coder", UnitKind::Engine, UnitState::Running,
               "Implementing API"),
        mkUnit("n-review", "n-build", "Reviewer", UnitKind::Engine, UnitState::Finished, ""),
        mkUnit("n-deep", "n-build", "Deep Fleet", UnitKind::Orchestrator, UnitState::Running,
               "Verifying outputs"),
        mkUnit("n-worker", "n-deep", "Worker A", UnitKind::Engine, UnitState::Running,
               "Running checks"),
        mkUnit("n-ops", "n-acme", "Ops Host", UnitKind::Host, UnitState::Running, ""),
    };
    // Stamp daemon-parity metadata (profile/session/role) on every seeded unit so
    // each unit carries the agent identity the Memory/Profile surfaces key off.
    for (UnitNode& n : m_units) {
        applyUnitMeta(n);
    }

    m_tags = {
        {1, QStringLiteral("ideas"), QStringLiteral("#2383e2")},
        {2, QStringLiteral("todo"), QStringLiteral("#e2a423")},
    };
    m_nextTagId = 3; // next id after the seeded tags

    const QDateTime now = QDateTime::currentDateTime();
    auto make = [&](const QString& unitId, const QList<int>& tagIds, bool archived,
                    const QString& title, const QString& content) {
        Session c;
        c.id = m_nextId++;
        c.sessionId = SessionId(QStringLiteral("s-seed-%1").arg(c.id));
        c.unitId = UnitId(unitId);
        c.tagIds = tagIds;
        c.isArchived = archived;
        c.title = title;
        c.content = content;
        c.created = now;
        c.modified = now;
        m_sessions.push_back(c);
    };

    make(QStringLiteral("n-scratch"), {1}, false, QStringLiteral("Scratch ideas"),
         QStringLiteral("# Scratchpad\n\nA lone unit with **no fleet** behind it:\n\n"
                        "- One root, one session\n- Still the same surface\n"));
    // An orchestrator's own reasoning transcript (a parent unit owns a session too).
    make(QStringLiteral("n-acme"), {}, false, QStringLiteral("Release planning"),
         QStringLiteral("## Release planning\n\nClassify, gate, and route the incoming work.\n"));
    make(QStringLiteral("n-build"), {2}, false, QStringLiteral("Dispatch log"),
         QStringLiteral("Spawned Coder and Reviewer; admitted within budget.\n"));
    make(QStringLiteral("n-coder"), {2}, false, QStringLiteral("Implement endpoint"),
         QStringLiteral("Working the `/tree` endpoint.\n\n> Stream UnitNode children.\n"));
    make(QStringLiteral("n-coder"), {}, false, QStringLiteral("Refactor pass"),
         QStringLiteral("Tidy the projection before review.\n"));
    make(QStringLiteral("n-review"), {1}, true, QStringLiteral("Old review notes"),
         QStringLiteral("Archived notes from an earlier review session.\n"));
    make(QStringLiteral("n-worker"), {}, false, QStringLiteral("Verification run"),
         QStringLiteral("Read-only checker over the worktree.\n"));

    // A demo transcript exercising every Phase 1 agent block (reasoning, tool
    // calls with each detail kind, a standalone content stream) for visual
    // inspection. Each block is its canonical fenced form.
    make(QStringLiteral("n-coder"), {1}, false, QStringLiteral("Agent blocks demo"),
         daemonnet::seed::agentBlocksMarkdown());

    // A demo transcript exercising the message/role layer.
    make(QStringLiteral("n-coder"), {1}, false, QStringLiteral("Message roles demo"),
         daemonnet::seed::roleLayerMarkdown());
}

bool InMemorySessionStore::isInSubtree(const UnitId& unitId, const UnitId& rootId) const {
    UnitId cur = unitId;
    // Walk up the parent chain; guard against cycles with a bounded loop.
    for (int guard = 0; guard < m_units.size() + 1 && !cur.isEmpty(); ++guard) {
        if (cur == rootId) {
            return true;
        }
        cur = unit(cur).parentId;
    }
    return false;
}

bool InMemorySessionStore::matchesScope(const Session& c, const ListScope& scope) const {
    switch (scope.type) {
    case NodeType::AllSessions:
        return !c.isArchived;
    case NodeType::Archived:
        return c.isArchived;
    case NodeType::Unit:
        return !c.isArchived && isInSubtree(c.unitId, scope.unitId);
    case NodeType::Tag:
        return !c.isArchived && c.tagIds.contains(scope.tagId);
    case NodeType::Agent:
        // Fleet membership (§0): an agent's sessions are those bound to its profile (`lensKey`).
        return !c.isArchived && !scope.lensKey.isEmpty() &&
               c.boundProfile.toString() == scope.lensKey;
    case NodeType::ByTransport:
    case NodeType::ByPeer:
    case NodeType::FleetSeparator:
    case NodeType::TagSeparator:
    case NodeType::TransportSeparator:
    case NodeType::Transport:
    case NodeType::FleetNode:
    case NodeType::AgentSession:
        // Transport/peer grouping needs the DaemonNet's edges (IDaemonNet::sessionsInScope), which
        // the flat store has no data for; separators + structural rows are never matchable.
        return false;
    }
    return false;
}

QList<UnitNode> InMemorySessionStore::unitChildren(const UnitId& parentId) const {
    QList<UnitNode> out;
    for (const UnitNode& n : m_units) {
        if (n.parentId == parentId) {
            out.push_back(n);
        }
    }
    return out;
}

UnitNode InMemorySessionStore::unit(const UnitId& id) const {
    for (const UnitNode& n : m_units) {
        if (n.id == id) {
            return n;
        }
    }
    return {};
}

QList<Tag> InMemorySessionStore::tags() const {
    return m_tags;
}

QList<domain::Participant> InMemorySessionStore::participants() const {
    return m_participants;
}

QList<Session> InMemorySessionStore::sessions(const ListScope& scope) const {
    QList<Session> out;
    for (const Session& c : m_sessions) {
        if (matchesScope(c, scope)) {
            out.push_back(c);
        }
    }
    // Pinned sessions float to the top; store order is otherwise preserved
    // (stable partition), so /title + moveSession stay predictable.
    std::ranges::stable_partition(out, [](const Session& c) { return c.isPinned; });
    return out;
}

int InMemorySessionStore::sessionCount(const ListScope& scope) const {
    int count = 0;
    for (const Session& c : m_sessions) {
        if (matchesScope(c, scope)) {
            ++count;
        }
    }
    return count;
}

SessionId InMemorySessionStore::mintSessionId() {
    return SessionId(QStringLiteral("s-") + QUuid::createUuid().toString(QUuid::WithoutBraces));
}

QString InMemorySessionStore::content(const SessionId& id) const {
    for (const Session& c : m_sessions) {
        if (c.sessionId == id) {
            return c.content;
        }
    }
    return {};
}

QString InMemorySessionStore::title(const SessionId& id) const {
    for (const Session& c : m_sessions) {
        if (c.sessionId == id) {
            return c.title;
        }
    }
    return {};
}

SessionId InMemorySessionStore::newSession(const UnitId& unitId) {
    Session c;
    c.id = m_nextId++;
    c.sessionId = mintSessionId();
    c.unitId = unitId;
    c.title = QStringLiteral("New session");
    c.created = QDateTime::currentDateTime();
    c.modified = c.created;
    m_sessions.push_back(c);
    emit changed();
    return c.sessionId;
}

UnitId InMemorySessionStore::createUnit(const UnitId& parentId, UnitKind kind) {
    UnitNode n;
    n.id = UnitId(QStringLiteral("n-new-%1").arg(m_nextUnitSeq++));
    n.parentId = parentId;
    n.name = parentId.isEmpty() ? QStringLiteral("New fleet") : QStringLiteral("New unit");
    n.kind = kind;
    n.state = UnitState::Unknown;
    applyUnitMeta(n);
    m_units.push_back(n);
    emit changed();
    return n.id;
}

int InMemorySessionStore::createTag(const QString& name, const QString& color) {
    Tag t;
    t.id = m_nextTagId++;
    t.name = name;
    t.color = color;
    m_tags.push_back(t);
    emit changed();
    return t.id;
}

// --- SessionId-keyed implementations (the canonical path) ---
void InMemorySessionStore::setContent(const SessionId& id, const QString& markdown) {
    if (id.isEmpty()) {
        return;
    }
    for (Session& c : m_sessions) {
        if (c.sessionId == id) {
            c.content = markdown;
            c.modified = QDateTime::currentDateTime();
            emit changed();
            return;
        }
    }
}

void InMemorySessionStore::renameSession(const SessionId& id, const QString& title) {
    if (id.isEmpty()) {
        return;
    }
    for (Session& c : m_sessions) {
        if (c.sessionId == id) {
            c.title = title;
            c.modified = QDateTime::currentDateTime();
            emit changed();
            return;
        }
    }
}

void InMemorySessionStore::deleteSession(const SessionId& id) {
    if (id.isEmpty()) {
        return;
    }
    for (int i = 0; i < m_sessions.size(); ++i) {
        if (m_sessions.at(i).sessionId == id) {
            m_sessions.removeAt(i);
            emit changed();
            return;
        }
    }
}

void InMemorySessionStore::setPinned(const SessionId& id, bool pinned) {
    if (id.isEmpty()) {
        return;
    }
    for (Session& c : m_sessions) {
        if (c.sessionId == id) {
            if (c.isPinned != pinned) {
                c.isPinned = pinned;
                emit changed();
            }
            return;
        }
    }
}

bool InMemorySessionStore::isPinned(const SessionId& id) const {
    for (const Session& c : m_sessions) {
        if (c.sessionId == id) {
            return c.isPinned;
        }
    }
    return false;
}

void InMemorySessionStore::moveSession(const SessionId& id, int delta) {
    if (delta == 0 || id.isEmpty()) {
        return;
    }
    for (int i = 0; i < m_sessions.size(); ++i) {
        if (m_sessions.at(i).sessionId != id) {
            continue;
        }
        const int target =
            qBound(0, i + (delta < 0 ? -1 : 1), static_cast<int>(m_sessions.size()) - 1);
        if (target != i) {
            m_sessions.move(i, target);
            emit changed();
        }
        return;
    }
}

void InMemorySessionStore::setArchived(const SessionId& id, bool archived) {
    if (id.isEmpty()) {
        return;
    }
    for (Session& c : m_sessions) {
        if (c.sessionId == id) {
            c.isArchived = archived;
            emit changed();
            return;
        }
    }
}

} // namespace persistence
