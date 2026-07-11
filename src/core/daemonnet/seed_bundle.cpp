// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemonnet/seed_bundle.h"

#include "daemonnet/seed_transcripts.h"

#include <QDateTime>
#include <QHash>
#include <QString>

namespace daemonnet {
namespace {

using domain::Lifecycle;
using domain::ProfileRef;
using domain::Session;
using domain::SessionRole;
using domain::SessionState;
using domain::UnitId;
using domain::UnitKind;
using domain::UnitNode;
using domain::UnitState;

} // namespace

SeedBundle defaultSeedBundle() {
    SeedBundle out;

    static const QHash<QString, QString> kUnitProfiles = {
        {QStringLiteral("n-scratch"), QStringLiteral("prof-1")},
        {QStringLiteral("n-acme"), QStringLiteral("prof-1")},
        {QStringLiteral("n-build"), QStringLiteral("prof-1")},
        {QStringLiteral("n-coder"), QStringLiteral("prof-2")},
        {QStringLiteral("n-worker"), QStringLiteral("prof-2")},
        {QStringLiteral("n-review"), QStringLiteral("prof-3")},
        {QStringLiteral("n-deep"), QStringLiteral("prof-3")},
    };
    const auto mkUnit = [&](const char* id, const char* parent, const char* name, UnitKind kind,
                            UnitState state, const char* work) {
        UnitNode n;
        n.id = UnitId(QString::fromLatin1(id));
        const QString p = QString::fromLatin1(parent);
        n.parentId = p.isEmpty() ? UnitId() : UnitId(p);
        n.name = QString::fromUtf8(name);
        n.kind = kind;
        n.state = state;
        n.work = QString::fromUtf8(work);
        n.session = domain::SessionId(n.id.toString());
        n.role = n.parentId.isEmpty()
                     ? SessionRole::Primary
                     : (n.id == UnitId(QStringLiteral("n-worker")) ? SessionRole::EphemeralSubagent
                                                                   : SessionRole::ManagedChild);
        n.profile =
            (kind == UnitKind::Host)
                ? ProfileRef()
                : ProfileRef(kUnitProfiles.value(n.id.toString(), QStringLiteral("prof-1")));
        out.units.push_back(n);
    };
    mkUnit("n-scratch", "", "scratchpad", UnitKind::Engine, UnitState::Running, "");
    mkUnit("n-acme", "", "Acme Platform", UnitKind::Orchestrator, UnitState::Running,
           "Coordinating release");
    mkUnit("n-build", "n-acme", "Build Fleet", UnitKind::Orchestrator, UnitState::Running,
           "Dispatching work");
    mkUnit("n-coder", "n-build", "Coder", UnitKind::Engine, UnitState::Running, "Implementing API");
    mkUnit("n-review", "n-build", "Reviewer", UnitKind::Engine, UnitState::Finished, "");
    mkUnit("n-deep", "n-build", "Deep Fleet", UnitKind::Orchestrator, UnitState::Running,
           "Verifying outputs");
    mkUnit("n-worker", "n-deep", "Worker A", UnitKind::Engine, UnitState::Running,
           "Running checks");
    mkUnit("n-ops", "n-acme", "Ops Host", UnitKind::Host, UnitState::Running, "");

    const QDateTime now = QDateTime::currentDateTime();
    const auto sess = [&](const char* sid, const char* unit, bool archived, Lifecycle lifecycle,
                          const QString& title, const QString& content) {
        Session c;
        c.sessionId = domain::SessionId(QString::fromLatin1(sid));
        c.unitId = UnitId(QString::fromLatin1(unit));
        for (const UnitNode& u : out.units) {
            if (u.id == c.unitId) {
                c.boundProfile = u.profile;
                break;
            }
        }
        c.isArchived = archived;
        c.title = title;
        c.content = content;
        c.state = SessionState::Active;
        c.lifecycle = lifecycle;
        c.role = SessionRole::Primary;
        c.lastActivityMs = now.toMSecsSinceEpoch();
        c.created = now;
        c.modified = now;
        out.sessions.push_back(c);
    };

    sess("s-scratch", "n-scratch", false, Lifecycle::Durable, QStringLiteral("Scratch ideas"),
         QStringLiteral("# Scratchpad\n\nA lone unit with **no fleet** behind it:\n\n"
                        "- One root, one session\n- Still the same surface\n"));
    sess("s-acme", "n-acme", false, Lifecycle::Durable, QStringLiteral("Release planning"),
         QStringLiteral("## Release planning\n\nClassify, gate, and route the incoming work.\n"));
    sess("s-build", "n-build", false, Lifecycle::Live, QStringLiteral("Dispatch log"),
         QStringLiteral("Spawned Coder and Reviewer; admitted within budget.\n"));
    sess("s-coder-impl", "n-coder", false, Lifecycle::Live, QStringLiteral("Implement endpoint"),
         QStringLiteral("Working the `/tree` endpoint.\n\n> Stream UnitNode children.\n"));
    sess("s-coder-refactor", "n-coder", false, Lifecycle::Live, QStringLiteral("Refactor pass"),
         QStringLiteral("Tidy the projection before review.\n"));
    sess("s-review-old", "n-review", true, Lifecycle::Durable, QStringLiteral("Old review notes"),
         QStringLiteral("Archived notes from an earlier review session.\n"));
    sess("s-worker-verify", "n-worker", false, Lifecycle::Live, QStringLiteral("Verification run"),
         QStringLiteral("Read-only checker over the worktree.\n"));
    sess("s-demo-blocks", "n-coder", false, Lifecycle::Live, QStringLiteral("Agent blocks demo"),
         seed::agentBlocksMarkdown());
    sess("s-demo-roles", "n-coder", false, Lifecycle::Live, QStringLiteral("Message roles demo"),
         seed::roleLayerMarkdown());

    // Transport-bearing sessions (kept as plain sessions now the routing/transports seed is gone).
    sess("s-help", "n-coder", false, Lifecycle::Live, QStringLiteral("Onboarding help"),
         QStringLiteral("Helping @alice through onboarding over Matrix.\n"));
    sess("s-design", "n-worker", false, Lifecycle::Live, QStringLiteral("Design review room"),
         QStringLiteral("Multi-agent design review in the internal room.\n"));
    sess("s-secops", "n-build", false, Lifecycle::Live, QStringLiteral("#secops triage"),
         QStringLiteral("Triaging #secops with @alice and @bob.\n"));
    sess("s-cron-backup", "n-ops", false, Lifecycle::Live, QStringLiteral("Nightly backup"),
         QStringLiteral("Scheduled `nightly-backup` trigger, run #1842.\n"));
    sess("s-http-dashboard", "n-build", false, Lifecycle::Live, QStringLiteral("Dashboard query"),
         QStringLiteral("Inbound `GET /status` from the `dashboard` API key.\n"));

    return out;
}

} // namespace daemonnet
