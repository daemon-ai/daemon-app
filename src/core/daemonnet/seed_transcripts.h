#pragma once

#include <QString>

// Demo transcript markdown shared by the DaemonNet seed (the live mock UI source) and the
// test-fixture seed (`InMemorySessionStore::seedSampleData`). Kept in one place so the renderer
// showcase content does not drift between the two. These are pure presentation fixtures; a live
// daemon adapter sources real `SessionLogEntry` transcripts instead.
namespace daemonnet::seed {

// A demo turn exercising every Phase-1 agent block (reasoning, tool calls with each detail kind, a
// standalone content stream, clarify, approval). Each block is its canonical fenced form.
[[nodiscard]] inline QString agentBlocksMarkdown()
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

// A demo transcript exercising the message/role layer (user/assistant/system turns, steer, slash,
// process), interleaved with reasoning and tool blocks.
[[nodiscard]] inline QString roleLayerMarkdown()
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

} // namespace daemonnet::seed
