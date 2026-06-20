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
    m_nextTagId = 3; // next id after the seeded tags

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

    // A demo transcript exercising every Phase 1 agent block (reasoning, tool
    // calls with each detail kind, a standalone content stream) for visual
    // inspection. Each block is its canonical fenced form: a ```tool/```reasoning/
    // ```content fence whose body is the compact-JSON metadata (the same form the
    // runtime inject path serializes and the parser round-trips).
    make(QStringLiteral("n-coder"), { 1 }, false, QStringLiteral("Agent blocks demo"),
         agentBlocksSampleMarkdown());

    // A demo transcript exercising the message/role layer: user bubbles (with
    // directive chips), assistant turns (reasoning + tool + text, with a footer),
    // a system steer/slash notice, and a process notification. Roles are carried
    // by ```msg boundary markers the parser consumes into per-block role/messageId.
    make(QStringLiteral("n-coder"), { 1 }, false, QStringLiteral("Message roles demo"),
         roleLayerSampleMarkdown());
}

QString InMemoryConversationStore::agentBlocksSampleMarkdown()
{
    return QStringLiteral(R"SAMPLE(# Agent transcript blocks

A demo turn exercising every Phase 1 block. Activate (click) any card to reveal
its raw fenced markdown.

```reasoning
{"status":"complete","durationMs":4200,"body":"First I'll inspect the repo, then run the build and the tests, and finally summarize any failures. The `/tree` endpoint is the likely culprit."}
```

Running a shell command:

```tool
{"callId":"c1","name":"terminal","tone":"terminal","status":"ok","durationMs":1200,"argsSummary":"ninja -C build-test be_core_tests","detailKind":"ansi-stream","stdout":"\u001b[32mPASS\u001b[0m  80 tests\n\u001b[33mWARN\u001b[0m  1 deprecation\n\u001b[1mBuild OK\u001b[0m\n"}
```

Searching the web:

```tool
{"callId":"c2","name":"web_search","tone":"web","status":"ok","durationMs":820,"argsSummary":"qml block editor","detailKind":"search-results","hits":[{"title":"Qt QML Applications","url":"https://doc.qt.io/qt-6/qmlapplications.html","snippet":"Build native, fluid UIs with QML backed by C++."},{"title":"md4qt - C++ Markdown parser","url":"https://github.com/igormironchik/md4qt","snippet":"A header-only C++17/20 CommonMark parser."}]}
```

Applying a patch:

```tool
{"callId":"c3","name":"apply_patch","tone":"edit","status":"ok","durationMs":260,"argsSummary":"src/main.cpp","detailKind":"diff","diff":"--- a/src/main.cpp\n+++ b/src/main.cpp\n@@ -1,4 +1,4 @@\n int main(int argc, char** argv) {\n-    return 0;\n+    return App(argc, argv).run();\n }\n"}
```

A tool still in flight:

```tool
{"callId":"c4","name":"compile","tone":"tool","status":"running","argsSummary":"cmake --build build-test"}
```

A failing tool:

```tool
{"callId":"c5","name":"terminal","tone":"terminal","status":"error","durationMs":90,"detailKind":"ansi-stream","stderr":"\u001b[31merror:\u001b[0m expected ';' before '}' token\n"}
```

A standalone content stream:

```content
{"kind":"ansi-stream","body":"\u001b[36mi\u001b[0m tailing logs\n\u001b[2mwaiting for events...\u001b[0m\n"}
```

A generated image (image_generate):

```tool
{"callId":"c6","name":"image_generate","status":"ok","durationMs":5200,"aspectRatio":"1:1","imageUrl":"https://doc.qt.io/qt-6/images/qt-logo.png"}
```

The agent needs a decision (clarify - a multi-question form: single-select,
multi-select, and a freeform reply, submitted together):

```tool
{"callId":"c7","name":"clarify","requestId":"q1","questions":[{"id":"db","prompt":"Which database should I target for the migration?","choices":["PostgreSQL","SQLite","MySQL"]},{"id":"scope","prompt":"Which parts should I migrate? (select all that apply)","choices":["Schema","Data","Indexes","Triggers"],"multiSelect":true},{"id":"notes","prompt":"Any extra constraints I should know about?","allowFreeform":true}]}
```

A dangerous command awaiting approval:

```tool
{"callId":"c8","name":"terminal","tone":"terminal","status":"running","needsApproval":true,"allowPermanent":true,"approvalCommand":"rm -rf build-test && cmake --preset test","argsSummary":"rm -rf build-test"}
```

That wraps the demo turn.
)SAMPLE");
}

QString InMemoryConversationStore::roleLayerSampleMarkdown()
{
    return QStringLiteral(R"ROLES(```msg
{"id":"u1","role":"user"}
```

Can you migrate the database layer? See @file:src/db/schema.sql and @url:https://example.com/migrations for context.

```msg
{"id":"m1","role":"assistant"}
```

```reasoning
{"status":"complete","durationMs":3100,"body":"The user wants a DB migration. I'll inspect the schema, then propose a plan and apply it."}
```

```tool
{"callId":"r1","name":"read_file","tone":"edit","status":"ok","durationMs":120,"argsSummary":"src/db/schema.sql","detailKind":"diff","diff":"--- a/src/db/schema.sql\n+++ b/src/db/schema.sql\n@@\n-CREATE TABLE t(id INT);\n+CREATE TABLE t(id BIGINT);\n"}
```

Here's the migration plan: add a versioned `schema_migrations` table and widen every `id` column to `BIGINT` across the schema.

```msg
{"id":"s1","role":"system"}
```

steer:Prefer PostgreSQL syntax for all migrations.

```msg
{"id":"u2","role":"user"}
```

Sounds good — go ahead and run it.

```msg
{"id":"m2","role":"assistant"}
```

Applying the migration now.

```tool
{"callId":"r2","name":"terminal","tone":"terminal","status":"ok","durationMs":2400,"argsSummary":"psql -f migrate.sql","detailKind":"ansi-stream","stdout":"\u001b[32mOK\u001b[0m  3 tables migrated\n"}
```

Migration complete. All `id` columns are now `BIGINT` and a `schema_migrations` table tracks versions.

```msg
{"id":"s2","role":"system"}
```

slash:/model gpt-5
Switched model to gpt-5 for this thread.

```msg
{"id":"s3","role":"system"}
```

process:Background indexing finished
Reindexed 3 tables in 4.2s with no errors.
)ROLES");
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

QString InMemoryConversationStore::createNode(const QString& parentId, AgentNodeKind kind)
{
    AgentNode n;
    n.id = QStringLiteral("n-new-%1").arg(m_nextNodeSeq++);
    n.parentId = parentId;
    n.name = parentId.isEmpty() ? QStringLiteral("New fleet") : QStringLiteral("New node");
    n.kind = kind;
    n.state = AgentState::Unknown;
    m_nodes.push_back(n);
    emit changed();
    return n.id;
}

int InMemoryConversationStore::createTag(const QString& name, const QString& color)
{
    Tag t;
    t.id = m_nextTagId++;
    t.name = name;
    t.color = color;
    m_tags.push_back(t);
    emit changed();
    return t.id;
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
