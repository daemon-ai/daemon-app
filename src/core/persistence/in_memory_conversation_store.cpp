#include "persistence/in_memory_conversation_store.h"

#include <QDateTime>

namespace persistence {

using domain::AgentNode;
using domain::AgentNodeKind;
using domain::AgentState;
using domain::Conversation;
using domain::ListScope;
using domain::NodeType;
using domain::Tag;

InMemoryConversationStore::InMemoryConversationStore(QObject* parent)
    : IConversationStore(parent)
{
    seedSampleData();
}

void InMemoryConversationStore::seedSampleData()
{
    // A fleet-of-fleets that deliberately exercises the recursion:
    //  - "scratchpad" is a LONE root agent (a tree of one, no children).
    //  - "Acme Platform" is a deep root: Orchestrator -> Orchestrator ("Build
    //    Fleet") -> Orchestrator ("Deep Fleet") -> Engine, i.e. orchestrators
    //    nested inside orchestrators (no fixed "fleet/agent/subagent" shape).
    //  - conversations hang off nodes at multiple depths, including off
    //    intermediate orchestrator nodes (their own reasoning transcript).
    m_nodes = {
        { QStringLiteral("n-scratch"), {}, QStringLiteral("scratchpad"),
          AgentNodeKind::Engine, AgentState::Running, {} },

        { QStringLiteral("n-acme"), {}, QStringLiteral("Acme Platform"),
          AgentNodeKind::Orchestrator, AgentState::Running,
          QStringLiteral("Coordinating release") },
        { QStringLiteral("n-build"), QStringLiteral("n-acme"), QStringLiteral("Build Fleet"),
          AgentNodeKind::Orchestrator, AgentState::Running, QStringLiteral("Dispatching work") },
        { QStringLiteral("n-coder"), QStringLiteral("n-build"), QStringLiteral("Coder"),
          AgentNodeKind::Engine, AgentState::Running, QStringLiteral("Implementing API") },
        { QStringLiteral("n-review"), QStringLiteral("n-build"), QStringLiteral("Reviewer"),
          AgentNodeKind::Engine, AgentState::Finished, {} },
        { QStringLiteral("n-deep"), QStringLiteral("n-build"), QStringLiteral("Deep Fleet"),
          AgentNodeKind::Orchestrator, AgentState::Running, QStringLiteral("Verifying outputs") },
        { QStringLiteral("n-worker"), QStringLiteral("n-deep"), QStringLiteral("Worker A"),
          AgentNodeKind::Engine, AgentState::Running, QStringLiteral("Running checks") },
        { QStringLiteral("n-ops"), QStringLiteral("n-acme"), QStringLiteral("Ops Host"),
          AgentNodeKind::Host, AgentState::Running, {} },
    };

    m_tags = {
        { 1, QStringLiteral("ideas"), QStringLiteral("#2383e2") },
        { 2, QStringLiteral("todo"), QStringLiteral("#e2a423") },
    };

    const QDateTime now = QDateTime::currentDateTime();
    auto make = [&](const QString& agentId, const QList<int>& tagIds, bool archived,
                    const QString& title, const QString& content) {
        Conversation c;
        c.id = m_nextId++;
        c.agentId = agentId;
        c.tagIds = tagIds;
        c.isArchived = archived;
        c.title = title;
        c.content = content;
        c.created = now;
        c.modified = now;
        m_conversations.push_back(c);
    };

    make(QStringLiteral("n-scratch"), { 1 }, false, QStringLiteral("Scratch ideas"),
         QStringLiteral("# Scratchpad\n\nA lone agent with **no fleet** behind it:\n\n"
                        "- One root, one conversation\n- Still the same surface\n"));
    // An orchestrator's own reasoning transcript (a parent node owns a conversation too).
    make(QStringLiteral("n-acme"), {}, false, QStringLiteral("Release planning"),
         QStringLiteral("## Release planning\n\nClassify, gate, and route the incoming work.\n"));
    make(QStringLiteral("n-build"), { 2 }, false, QStringLiteral("Dispatch log"),
         QStringLiteral("Spawned Coder and Reviewer; admitted within budget.\n"));
    make(QStringLiteral("n-coder"), { 2 }, false, QStringLiteral("Implement endpoint"),
         QStringLiteral("Working the `/tree` endpoint.\n\n> Stream UnitNode children.\n"));
    make(QStringLiteral("n-coder"), {}, false, QStringLiteral("Refactor pass"),
         QStringLiteral("Tidy the projection before review.\n"));
    make(QStringLiteral("n-review"), { 1 }, true, QStringLiteral("Old review notes"),
         QStringLiteral("Archived notes from an earlier review session.\n"));
    make(QStringLiteral("n-worker"), {}, false, QStringLiteral("Verification run"),
         QStringLiteral("Read-only checker over the worktree.\n"));
}

bool InMemoryConversationStore::isInSubtree(const QString& nodeId, const QString& rootId) const
{
    QString cur = nodeId;
    // Walk up the parent chain; guard against cycles with a bounded loop.
    for (int guard = 0; guard < m_nodes.size() + 1 && !cur.isEmpty(); ++guard) {
        if (cur == rootId) {
            return true;
        }
        cur = agentNode(cur).parentId;
    }
    return false;
}

bool InMemoryConversationStore::matchesScope(const Conversation& c, const ListScope& scope) const
{
    switch (scope.type) {
    case NodeType::AllConversations:
        return !c.isArchived;
    case NodeType::Archived:
        return c.isArchived;
    case NodeType::Node:
        return !c.isArchived && isInSubtree(c.agentId, scope.nodeId);
    case NodeType::Tag:
        return !c.isArchived && c.tagIds.contains(scope.id);
    case NodeType::FleetSeparator:
    case NodeType::TagSeparator:
        return false;
    }
    return false;
}

QList<AgentNode> InMemoryConversationStore::agentChildren(const QString& parentId) const
{
    QList<AgentNode> out;
    for (const AgentNode& n : m_nodes) {
        if (n.parentId == parentId) {
            out.push_back(n);
        }
    }
    return out;
}

AgentNode InMemoryConversationStore::agentNode(const QString& id) const
{
    for (const AgentNode& n : m_nodes) {
        if (n.id == id) {
            return n;
        }
    }
    return {};
}

QList<Tag> InMemoryConversationStore::tags() const
{
    return m_tags;
}

QList<Conversation> InMemoryConversationStore::conversations(const ListScope& scope) const
{
    QList<Conversation> out;
    for (const Conversation& c : m_conversations) {
        if (matchesScope(c, scope)) {
            out.push_back(c);
        }
    }
    return out;
}

int InMemoryConversationStore::conversationCount(const ListScope& scope) const
{
    int count = 0;
    for (const Conversation& c : m_conversations) {
        if (matchesScope(c, scope)) {
            ++count;
        }
    }
    return count;
}

QString InMemoryConversationStore::content(int conversationId) const
{
    for (const Conversation& c : m_conversations) {
        if (c.id == conversationId) {
            return c.content;
        }
    }
    return {};
}

int InMemoryConversationStore::createConversation(const QString& agentId)
{
    Conversation c;
    c.id = m_nextId++;
    c.agentId = agentId;
    c.title = QStringLiteral("New conversation");
    c.created = QDateTime::currentDateTime();
    c.modified = c.created;
    m_conversations.push_back(c);
    emit changed();
    return c.id;
}

void InMemoryConversationStore::setContent(int conversationId, const QString& markdown)
{
    for (Conversation& c : m_conversations) {
        if (c.id == conversationId) {
            c.content = markdown;
            c.modified = QDateTime::currentDateTime();
            emit changed();
            return;
        }
    }
}

void InMemoryConversationStore::setArchived(int conversationId, bool archived)
{
    for (Conversation& c : m_conversations) {
        if (c.id == conversationId) {
            c.isArchived = archived;
            emit changed();
            return;
        }
    }
}

} // namespace persistence
