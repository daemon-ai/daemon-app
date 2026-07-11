// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemonnet/mock_fleet_source.h"

#include "daemonnet/seed_transcripts.h"
#include "uimodels/variant_list_model.h"

#include <functional>
#include <QDateTime>
#include <QHash>

namespace daemonnet {
namespace {

using domain::Lifecycle;
using domain::ProfileRef;
using domain::Session;
using domain::SessionRole;
using domain::SessionState;
using domain::Tag;
using domain::UnitId;
using domain::UnitKind;
using domain::UnitNode;
using domain::UnitState;

QString profileModel(const QString& profile) {
    static const QHash<QString, QString> kModels = {
        {QStringLiteral("prof-1"), QStringLiteral("llama-3.1-70b-instruct")},
        {QStringLiteral("prof-2"), QStringLiteral("qwen2.5-coder-32b")},
        {QStringLiteral("prof-3"), QStringLiteral("mixtral-8x7b")},
    };
    return kModels.value(profile, QString());
}

QString unitKindStr(UnitKind k) {
    switch (k) {
    case UnitKind::Orchestrator:
        return QStringLiteral("orchestrator");
    case UnitKind::Host:
        return QStringLiteral("host");
    case UnitKind::Engine:
        return QStringLiteral("worker");
    }
    return QStringLiteral("worker");
}

QString unitStatusStr(UnitState s) {
    switch (s) {
    case UnitState::Running:
        return QStringLiteral("running");
    case UnitState::Finished:
    case UnitState::Unknown:
        return QStringLiteral("idle");
    }
    return QStringLiteral("idle");
}

QString sessionStateStr(SessionState s) {
    switch (s) {
    case SessionState::Active:
        return QStringLiteral("active");
    case SessionState::Suspended:
        return QStringLiteral("suspended");
    case SessionState::Ready:
        return QStringLiteral("ready");
    case SessionState::Completed:
        return QStringLiteral("completed");
    case SessionState::Unknown:
        return QStringLiteral("idle");
    }
    return QStringLiteral("idle");
}

QString lifecycleStr(Lifecycle l) {
    return l == Lifecycle::Live ? QStringLiteral("live") : QStringLiteral("durable");
}

} // namespace

MockFleetSource::MockFleetSource(QObject* parent) : MockFleetSource(defaultSeed(), parent) {}

MockFleetSource::MockFleetSource(const SeedBundle& bundle, QObject* parent)
    : QObject(parent), m_units(bundle.units), m_sessions(bundle.sessions), m_tags(bundle.tags),
      m_participants(bundle.participants), m_fleetModel(new uimodels::VariantListModel(this)),
      m_sessionsModel(new uimodels::VariantListModel(this)) {
    computeProjections();
}

QObject* MockFleetSource::fleet() const {
    return m_fleetModel;
}
QObject* MockFleetSource::sessions() const {
    return m_sessionsModel;
}

SeedBundle MockFleetSource::seed() const {
    return SeedBundle{m_units, m_sessions, m_tags, m_participants};
}

SeedBundle MockFleetSource::defaultSeed() {
    SeedBundle out;
    out.tags = {
        {1, QStringLiteral("ideas"), QStringLiteral("#2383e2")},
        {2, QStringLiteral("todo"), QStringLiteral("#e2a423")},
    };
    out.participants = {
        {QStringLiteral("agent"), tr("Agent"), QStringLiteral("available"),
         QStringLiteral("#3fb950"), true},
        {QStringLiteral("user"), tr("User"), QStringLiteral("available"), QStringLiteral("#3fb950"),
         false},
    };

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
    const auto sess = [&](const char* sid, const char* unit, const QList<int>& tagIds,
                          bool archived, Lifecycle lifecycle, const QString& title,
                          const QString& content) {
        Session c;
        c.sessionId = domain::SessionId(QString::fromLatin1(sid));
        c.unitId = UnitId(QString::fromLatin1(unit));
        for (const UnitNode& u : out.units) {
            if (u.id == c.unitId) {
                c.boundProfile = u.profile;
                break;
            }
        }
        c.tagIds = tagIds;
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

    sess("s-scratch", "n-scratch", {1}, false, Lifecycle::Durable, QStringLiteral("Scratch ideas"),
         QStringLiteral("# Scratchpad\n\nA lone unit with **no fleet** behind it:\n\n"
                        "- One root, one session\n- Still the same surface\n"));
    sess("s-acme", "n-acme", {}, false, Lifecycle::Durable, QStringLiteral("Release planning"),
         QStringLiteral("## Release planning\n\nClassify, gate, and route the incoming work.\n"));
    sess("s-build", "n-build", {2}, false, Lifecycle::Live, QStringLiteral("Dispatch log"),
         QStringLiteral("Spawned Coder and Reviewer; admitted within budget.\n"));
    sess("s-coder-impl", "n-coder", {2}, false, Lifecycle::Live,
         QStringLiteral("Implement endpoint"),
         QStringLiteral("Working the `/tree` endpoint.\n\n> Stream UnitNode children.\n"));
    sess("s-coder-refactor", "n-coder", {}, false, Lifecycle::Live, QStringLiteral("Refactor pass"),
         QStringLiteral("Tidy the projection before review.\n"));
    sess("s-review-old", "n-review", {1}, true, Lifecycle::Durable,
         QStringLiteral("Old review notes"),
         QStringLiteral("Archived notes from an earlier review session.\n"));
    sess("s-worker-verify", "n-worker", {}, false, Lifecycle::Live,
         QStringLiteral("Verification run"),
         QStringLiteral("Read-only checker over the worktree.\n"));
    sess("s-demo-blocks", "n-coder", {1}, false, Lifecycle::Live,
         QStringLiteral("Agent blocks demo"), seed::agentBlocksMarkdown());
    sess("s-demo-roles", "n-coder", {1}, false, Lifecycle::Live,
         QStringLiteral("Message roles demo"), seed::roleLayerMarkdown());

    // Transport-bearing sessions (kept as plain sessions now the routing/transports seed is gone).
    sess("s-help", "n-coder", {}, false, Lifecycle::Live, QStringLiteral("Onboarding help"),
         QStringLiteral("Helping @alice through onboarding over Matrix.\n"));
    sess("s-design", "n-worker", {}, false, Lifecycle::Live, QStringLiteral("Design review room"),
         QStringLiteral("Multi-agent design review in the internal room.\n"));
    sess("s-secops", "n-build", {}, false, Lifecycle::Live, QStringLiteral("#secops triage"),
         QStringLiteral("Triaging #secops with @alice and @bob.\n"));
    sess("s-cron-backup", "n-ops", {}, false, Lifecycle::Live, QStringLiteral("Nightly backup"),
         QStringLiteral("Scheduled `nightly-backup` trigger, run #1842.\n"));
    sess("s-http-dashboard", "n-build", {}, false, Lifecycle::Live,
         QStringLiteral("Dashboard query"),
         QStringLiteral("Inbound `GET /status` from the `dashboard` API key.\n"));

    return out;
}

void MockFleetSource::computeProjections() {
    projectFleet();
    projectSessions();
}

void MockFleetSource::projectFleet() {
    const QString kId = QStringLiteral("id");
    QHash<QString, QStringList> children;
    for (const UnitNode& u : m_units) {
        if (!u.parentId.isEmpty()) {
            children[u.parentId.toString()].append(u.id.toString());
        }
    }
    QHash<QString, UnitNode> unitById;
    for (const UnitNode& u : m_units) {
        unitById.insert(u.id.toString(), u);
    }
    QList<QVariantMap> fleetRows;
    std::function<void(const QString&, int)> visit = [&](const QString& id, int depth) {
        const UnitNode u = unitById.value(id);
        QVariantMap row;
        row[kId] = id;
        row[QStringLiteral("depth")] = depth;
        row[QStringLiteral("name")] = u.name;
        row[QStringLiteral("kind")] = unitKindStr(u.kind);
        row[QStringLiteral("status")] = unitStatusStr(u.state);
        row[QStringLiteral("model")] = profileModel(u.profile.toString());
        fleetRows.append(row);
        for (const QString& c : children.value(id)) {
            visit(c, depth + 1);
        }
    };
    for (const UnitNode& u : m_units) {
        if (u.parentId.isEmpty()) {
            visit(u.id.toString(), 0);
        }
    }
    m_fleetModel->setRows(fleetRows);
}

void MockFleetSource::projectSessions() {
    static const QHash<QString, int> kTokens = {
        {QStringLiteral("s-scratch"), 320},        {QStringLiteral("s-acme"), 880},
        {QStringLiteral("s-build"), 540},          {QStringLiteral("s-coder-impl"), 1240},
        {QStringLiteral("s-coder-refactor"), 760}, {QStringLiteral("s-review-old"), 410},
        {QStringLiteral("s-worker-verify"), 600},  {QStringLiteral("s-demo-blocks"), 1500},
        {QStringLiteral("s-demo-roles"), 980},     {QStringLiteral("s-help"), 1840},
        {QStringLiteral("s-design"), 720},         {QStringLiteral("s-secops"), 1100},
    };
    QList<QVariantMap> sessionRows;
    for (const Session& c : m_sessions) {
        QVariantMap row;
        row[QStringLiteral("id")] = c.sessionId.toString();
        row[QStringLiteral("title")] = c.title;
        row[QStringLiteral("profile")] = c.boundProfile.toString();
        row[QStringLiteral("state")] = sessionStateStr(c.state);
        row[QStringLiteral("lifecycle")] = lifecycleStr(c.lifecycle);
        row[QStringLiteral("lastActivity")] = QStringLiteral("recently");
        row[QStringLiteral("tokens")] = kTokens.value(c.sessionId.toString(), 500);
        row[QStringLiteral("rewindable")] = true;
        sessionRows.append(row);
    }
    m_sessionsModel->setRows(sessionRows);
}

} // namespace daemonnet
