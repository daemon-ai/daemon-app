#include "persistence/in_memory_session_store.h"

#include <QDateTime>

#include <algorithm>

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

void InMemorySessionStore::applyUnitMeta(UnitNode& n)
{
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
        { QStringLiteral("n-scratch"), QStringLiteral("prof-1") },
        { QStringLiteral("n-acme"), QStringLiteral("prof-1") },
        { QStringLiteral("n-build"), QStringLiteral("prof-1") },
        { QStringLiteral("n-coder"), QStringLiteral("prof-2") },
        { QStringLiteral("n-worker"), QStringLiteral("prof-2") },
        { QStringLiteral("n-review"), QStringLiteral("prof-3") },
        { QStringLiteral("n-deep"), QStringLiteral("prof-3") },
    };
    n.profile = ProfileRef(kUnitProfiles.value(n.id.toString(), QStringLiteral("prof-1")));
}

InMemorySessionStore::InMemorySessionStore(QObject* parent)
    : InMemorySessionStore(parent, true)
{
}

InMemorySessionStore::InMemorySessionStore(QObject* parent, bool seed)
    : ISessionStore(parent)
{
    if (seed) {
        seedSampleData();
    }
}

void InMemorySessionStore::seedSampleData()
{
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
        { 1, QStringLiteral("ideas"), QStringLiteral("#2383e2") },
        { 2, QStringLiteral("todo"), QStringLiteral("#e2a423") },
    };
    m_nextTagId = 3; // next id after the seeded tags

    const QDateTime now = QDateTime::currentDateTime();
    auto make = [&](const QString& unitId, const QList<int>& tagIds, bool archived,
                    const QString& title, const QString& content) {
        Session c;
        c.id = m_nextId++;
        c.sessionId = SessionId(QStringLiteral("local-%1").arg(c.id));
        c.unitId = UnitId(unitId);
        c.tagIds = tagIds;
        c.isArchived = archived;
        c.title = title;
        c.content = content;
        c.created = now;
        c.modified = now;
        m_sessions.push_back(c);
    };

    make(QStringLiteral("n-scratch"), { 1 }, false, QStringLiteral("Scratch ideas"),
         QStringLiteral("# Scratchpad\n\nA lone unit with **no fleet** behind it:\n\n"
                        "- One root, one session\n- Still the same surface\n"));
    // An orchestrator's own reasoning transcript (a parent unit owns a session too).
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
    // inspection. Each block is its canonical fenced form.
    make(QStringLiteral("n-coder"), { 1 }, false, QStringLiteral("Agent blocks demo"),
         agentBlocksSampleMarkdown());

    // A demo transcript exercising the message/role layer.
    make(QStringLiteral("n-coder"), { 1 }, false, QStringLiteral("Message roles demo"),
         roleLayerSampleMarkdown());
}

QString InMemorySessionStore::agentBlocksSampleMarkdown()
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

QString InMemorySessionStore::roleLayerSampleMarkdown()
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

bool InMemorySessionStore::isInSubtree(const UnitId& unitId, const UnitId& rootId) const
{
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

bool InMemorySessionStore::matchesScope(const Session& c, const ListScope& scope) const
{
    switch (scope.type) {
    case NodeType::AllSessions:
        return !c.isArchived;
    case NodeType::Archived:
        return c.isArchived;
    case NodeType::Unit:
        return !c.isArchived && isInSubtree(c.unitId, scope.unitId);
    case NodeType::Tag:
        return !c.isArchived && c.tagIds.contains(scope.tagId);
    case NodeType::FleetSeparator:
    case NodeType::TagSeparator:
        return false;
    }
    return false;
}

QList<UnitNode> InMemorySessionStore::unitChildren(const UnitId& parentId) const
{
    QList<UnitNode> out;
    for (const UnitNode& n : m_units) {
        if (n.parentId == parentId) {
            out.push_back(n);
        }
    }
    return out;
}

UnitNode InMemorySessionStore::unit(const UnitId& id) const
{
    for (const UnitNode& n : m_units) {
        if (n.id == id) {
            return n;
        }
    }
    return {};
}

QList<Tag> InMemorySessionStore::tags() const
{
    return m_tags;
}

QList<Session> InMemorySessionStore::sessions(const ListScope& scope) const
{
    QList<Session> out;
    for (const Session& c : m_sessions) {
        if (matchesScope(c, scope)) {
            out.push_back(c);
        }
    }
    // Pinned sessions float to the top; store order is otherwise preserved
    // (stable partition), so /title + moveSession stay predictable.
    std::stable_partition(out.begin(), out.end(), [](const Session& c) { return c.isPinned; });
    return out;
}

int InMemorySessionStore::sessionCount(const ListScope& scope) const
{
    int count = 0;
    for (const Session& c : m_sessions) {
        if (matchesScope(c, scope)) {
            ++count;
        }
    }
    return count;
}

QString InMemorySessionStore::content(int sessionId) const
{
    for (const Session& c : m_sessions) {
        if (c.id == sessionId) {
            return c.content;
        }
    }
    return {};
}

QString InMemorySessionStore::title(int sessionId) const
{
    for (const Session& c : m_sessions) {
        if (c.id == sessionId) {
            return c.title;
        }
    }
    return {};
}

int InMemorySessionStore::createSession(const UnitId& unitId)
{
    Session c;
    c.id = m_nextId++;
    c.sessionId = SessionId(QStringLiteral("local-%1").arg(c.id));
    c.unitId = unitId;
    c.title = QStringLiteral("New session");
    c.created = QDateTime::currentDateTime();
    c.modified = c.created;
    m_sessions.push_back(c);
    emit changed();
    return c.id;
}

UnitId InMemorySessionStore::createUnit(const UnitId& parentId, UnitKind kind)
{
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

int InMemorySessionStore::createTag(const QString& name, const QString& color)
{
    Tag t;
    t.id = m_nextTagId++;
    t.name = name;
    t.color = color;
    m_tags.push_back(t);
    emit changed();
    return t.id;
}

void InMemorySessionStore::setContent(int sessionId, const QString& markdown)
{
    for (Session& c : m_sessions) {
        if (c.id == sessionId) {
            c.content = markdown;
            c.modified = QDateTime::currentDateTime();
            emit changed();
            return;
        }
    }
}

void InMemorySessionStore::renameSession(int sessionId, const QString& title)
{
    for (Session& c : m_sessions) {
        if (c.id == sessionId) {
            c.title = title;
            c.modified = QDateTime::currentDateTime();
            emit changed();
            return;
        }
    }
}

void InMemorySessionStore::deleteSession(int sessionId)
{
    for (int i = 0; i < m_sessions.size(); ++i) {
        if (m_sessions.at(i).id == sessionId) {
            m_sessions.removeAt(i);
            emit changed();
            return;
        }
    }
}

void InMemorySessionStore::setPinned(int sessionId, bool pinned)
{
    for (Session& c : m_sessions) {
        if (c.id == sessionId) {
            if (c.isPinned != pinned) {
                c.isPinned = pinned;
                emit changed();
            }
            return;
        }
    }
}

bool InMemorySessionStore::isPinned(int sessionId) const
{
    for (const Session& c : m_sessions) {
        if (c.id == sessionId) {
            return c.isPinned;
        }
    }
    return false;
}

void InMemorySessionStore::moveSession(int sessionId, int delta)
{
    if (delta == 0) {
        return;
    }
    for (int i = 0; i < m_sessions.size(); ++i) {
        if (m_sessions.at(i).id != sessionId) {
            continue;
        }
        const int target = qBound(0, i + (delta < 0 ? -1 : 1),
                                  static_cast<int>(m_sessions.size()) - 1);
        if (target != i) {
            m_sessions.move(i, target);
            emit changed();
        }
        return;
    }
}

void InMemorySessionStore::setArchived(int sessionId, bool archived)
{
    for (Session& c : m_sessions) {
        if (c.id == sessionId) {
            c.isArchived = archived;
            emit changed();
            return;
        }
    }
}

} // namespace persistence
