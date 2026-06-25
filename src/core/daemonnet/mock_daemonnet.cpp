#include "daemonnet/mock_daemonnet.h"

#include "daemonnet/seed_transcripts.h"
#include "uimodels/variant_list_model.h"

#include <QDateTime>
#include <QHash>
#include <QSet>
#include <QStringList>

#include <functional>

namespace daemonnet {
namespace {

using domain::Lifecycle;
using domain::ProfileRef;
using domain::Session;
using domain::SessionId;
using domain::SessionRole;
using domain::SessionState;
using domain::Tag;
using domain::UnitId;
using domain::UnitKind;
using domain::UnitNode;
using domain::UnitState;

QString profileModel(const QString& profile)
{
    static const QHash<QString, QString> kModels = {
        { QStringLiteral("prof-1"), QStringLiteral("llama-3.1-70b-instruct") },
        { QStringLiteral("prof-2"), QStringLiteral("qwen2.5-coder-32b") },
        { QStringLiteral("prof-3"), QStringLiteral("mixtral-8x7b") },
    };
    return kModels.value(profile, QString());
}

QString unitKindStr(UnitKind k)
{
    switch (k) {
    case UnitKind::Orchestrator: return QStringLiteral("orchestrator");
    case UnitKind::Host: return QStringLiteral("host");
    case UnitKind::Engine: return QStringLiteral("worker");
    }
    return QStringLiteral("worker");
}

QString unitStatusStr(UnitState s)
{
    switch (s) {
    case UnitState::Running: return QStringLiteral("running");
    case UnitState::Finished: return QStringLiteral("idle");
    case UnitState::Unknown: return QStringLiteral("idle");
    }
    return QStringLiteral("idle");
}

QString sessionStateStr(SessionState s)
{
    switch (s) {
    case SessionState::Active: return QStringLiteral("active");
    case SessionState::Suspended: return QStringLiteral("suspended");
    case SessionState::Ready: return QStringLiteral("ready");
    case SessionState::Completed: return QStringLiteral("completed");
    case SessionState::Unknown: return QStringLiteral("idle");
    }
    return QStringLiteral("idle");
}

QString lifecycleStr(Lifecycle l)
{
    return l == Lifecycle::Live ? QStringLiteral("live") : QStringLiteral("durable");
}

} // namespace

MockDaemonNet::MockDaemonNet(QObject* parent)
    : IDaemonNet(parent)
    , m_fleetModel(new uimodels::VariantListModel(this))
    , m_sessionsModel(new uimodels::VariantListModel(this))
    , m_channelsModel(new uimodels::VariantListModel(this))
    , m_byPeerModel(new uimodels::VariantListModel(this))
{
    buildSeed();
    computeProjections();
}

QObject* MockDaemonNet::fleet() const { return m_fleetModel; }
QObject* MockDaemonNet::sessions() const { return m_sessionsModel; }
QObject* MockDaemonNet::channels() const { return m_channelsModel; }
QObject* MockDaemonNet::byPeer() const { return m_byPeerModel; }

QVariantList MockDaemonNet::nodes() const
{
    QVariantList out;
    out.reserve(static_cast<int>(m_nodes.size()));
    for (const QVariantMap& n : m_nodes) {
        out.append(n);
    }
    return out;
}

QVariantList MockDaemonNet::edges() const
{
    QVariantList out;
    out.reserve(static_cast<int>(m_edges.size()));
    for (const QVariantMap& e : m_edges) {
        out.append(e);
    }
    return out;
}

SeedBundle MockDaemonNet::seed() const
{
    return SeedBundle{ m_units, m_sessions, m_tags };
}

QList<domain::UnitNode> MockDaemonNet::unitChildren(const domain::UnitId& parent) const
{
    QList<domain::UnitNode> out;
    for (const UnitNode& u : m_units) {
        if (u.parentId == parent) {
            out.push_back(u);
        }
    }
    return out;
}

domain::UnitNode MockDaemonNet::unit(const domain::UnitId& id) const
{
    for (const UnitNode& u : m_units) {
        if (u.id == id) {
            return u;
        }
    }
    return {};
}

QList<domain::Tag> MockDaemonNet::tags() const { return m_tags; }

bool MockDaemonNet::isInSubtree(const domain::UnitId& unitId, const domain::UnitId& rootId) const
{
    domain::UnitId cur = unitId;
    while (!cur.isEmpty()) {
        if (cur == rootId) {
            return true;
        }
        cur = unit(cur).parentId;
    }
    return false;
}

QSet<QString> MockDaemonNet::sessionsBoundBy(const QString& edgeKind, const QString& lensKey) const
{
    QSet<QString> ids;
    for (const QVariantMap& e : m_edges) {
        if (e.value(QStringLiteral("edgeKind")).toString() == edgeKind
            && e.value(QStringLiteral("target")).toString() == lensKey) {
            ids.insert(e.value(QStringLiteral("source")).toString());
        }
    }
    return ids;
}

QList<domain::Session> MockDaemonNet::sessionsInScope(const domain::ListScope& scope) const
{
    using domain::NodeType;
    QList<domain::Session> out;
    QSet<QString> lensIds;
    if (scope.type == NodeType::ByTransport) {
        lensIds = sessionsBoundBy(QStringLiteral("over"), scope.lensKey);
    } else if (scope.type == NodeType::ByPeer) {
        lensIds = sessionsBoundBy(QStringLiteral("participant"), scope.lensKey);
    }
    for (const Session& s : m_sessions) {
        bool keep = false;
        switch (scope.type) {
        case NodeType::AllSessions:
            keep = !s.isArchived;
            break;
        case NodeType::Archived:
            keep = s.isArchived;
            break;
        case NodeType::Unit:
            keep = !s.isArchived && isInSubtree(s.unitId, scope.unitId);
            break;
        case NodeType::Tag:
            keep = !s.isArchived && s.tagIds.contains(scope.tagId);
            break;
        case NodeType::ByTransport:
        case NodeType::ByPeer:
            keep = lensIds.contains(s.sessionId.toString());
            break;
        case NodeType::FleetSeparator:
        case NodeType::TagSeparator:
            keep = false; // headers carry no session list
            break;
        }
        if (keep) {
            out.push_back(s);
        }
    }
    return out;
}

domain::Session MockDaemonNet::sessionDetail(const domain::SessionId& id) const
{
    for (const Session& s : m_sessions) {
        if (s.sessionId == id) {
            return s;
        }
    }
    return {};
}

QString MockDaemonNet::content(const domain::SessionId& id) const
{
    return sessionDetail(id).content;
}

void MockDaemonNet::buildSeed()
{
    const QDateTime now = QDateTime::currentDateTime();

    // --- Tags (client-local cross-cutting labels) ---
    m_tags = {
        { 1, QStringLiteral("ideas"), QStringLiteral("#2383e2") },
        { 2, QStringLiteral("todo"), QStringLiteral("#e2a423") },
    };

    // --- Units: the fleet-of-fleets (a lone Engine root, nested orchestrators, a Host) ---
    static const QHash<QString, QString> kUnitProfiles = {
        { QStringLiteral("n-scratch"), QStringLiteral("prof-1") },
        { QStringLiteral("n-acme"), QStringLiteral("prof-1") },
        { QStringLiteral("n-build"), QStringLiteral("prof-1") },
        { QStringLiteral("n-coder"), QStringLiteral("prof-2") },
        { QStringLiteral("n-worker"), QStringLiteral("prof-2") },
        { QStringLiteral("n-review"), QStringLiteral("prof-3") },
        { QStringLiteral("n-deep"), QStringLiteral("prof-3") },
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
        n.session = SessionId(n.id.toString());
        n.role = n.parentId.isEmpty() ? SessionRole::Primary
            : (n.id == UnitId(QStringLiteral("n-worker")) ? SessionRole::EphemeralSubagent
                                                          : SessionRole::ManagedChild);
        n.profile = (kind == UnitKind::Host)
            ? ProfileRef()
            : ProfileRef(kUnitProfiles.value(n.id.toString(), QStringLiteral("prof-1")));
        m_units.push_back(n);
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
    mkUnit("n-worker", "n-deep", "Worker A", UnitKind::Engine, UnitState::Running, "Running checks");
    mkUnit("n-ops", "n-acme", "Ops Host", UnitKind::Host, UnitState::Running, "");

    // --- Sessions: hung off units at multiple depths (+ the renderer-demo transcripts) ---
    const auto sess = [&](const char* sid, const char* unit, const QList<int>& tagIds, bool archived,
                          Lifecycle lifecycle, const QString& title, const QString& content) {
        Session c;
        c.sessionId = SessionId(QString::fromLatin1(sid));
        c.unitId = UnitId(QString::fromLatin1(unit));
        for (const UnitNode& u : m_units) {
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
        m_sessions.push_back(c);
    };

    sess("s-scratch", "n-scratch", { 1 }, false, Lifecycle::Durable, QStringLiteral("Scratch ideas"),
         QStringLiteral("# Scratchpad\n\nA lone unit with **no fleet** behind it:\n\n"
                        "- One root, one session\n- Still the same surface\n"));
    sess("s-acme", "n-acme", {}, false, Lifecycle::Durable, QStringLiteral("Release planning"),
         QStringLiteral("## Release planning\n\nClassify, gate, and route the incoming work.\n"));
    sess("s-build", "n-build", { 2 }, false, Lifecycle::Live, QStringLiteral("Dispatch log"),
         QStringLiteral("Spawned Coder and Reviewer; admitted within budget.\n"));
    sess("s-coder-impl", "n-coder", { 2 }, false, Lifecycle::Live,
         QStringLiteral("Implement endpoint"),
         QStringLiteral("Working the `/tree` endpoint.\n\n> Stream UnitNode children.\n"));
    sess("s-coder-refactor", "n-coder", {}, false, Lifecycle::Live, QStringLiteral("Refactor pass"),
         QStringLiteral("Tidy the projection before review.\n"));
    sess("s-review-old", "n-review", { 1 }, true, Lifecycle::Durable,
         QStringLiteral("Old review notes"),
         QStringLiteral("Archived notes from an earlier review session.\n"));
    sess("s-worker-verify", "n-worker", {}, false, Lifecycle::Live,
         QStringLiteral("Verification run"),
         QStringLiteral("Read-only checker over the worktree.\n"));
    sess("s-demo-blocks", "n-coder", { 1 }, false, Lifecycle::Live,
         QStringLiteral("Agent blocks demo"), seed::agentBlocksMarkdown());
    sess("s-demo-roles", "n-coder", { 1 }, false, Lifecycle::Live,
         QStringLiteral("Message roles demo"), seed::roleLayerMarkdown());

    // Transport-bearing sessions (the combos): a matrix DM, an internal Room, a Matrix channel.
    sess("s-help", "n-coder", {}, false, Lifecycle::Live, QStringLiteral("Onboarding help"),
         QStringLiteral("Helping @alice through onboarding over Matrix.\n"));
    sess("s-design", "n-worker", {}, false, Lifecycle::Live, QStringLiteral("Design review room"),
         QStringLiteral("Multi-agent design review in the internal room.\n"));
    sess("s-secops", "n-build", {}, false, Lifecycle::Live, QStringLiteral("#secops triage"),
         QStringLiteral("Triaging #secops with @alice and @bob.\n"));

    // --- Raw graph (agents + sessions + transports/peers/rooms + edges) for channels/byPeer/patch-bay ---
    const auto node = [&](const QString& id, const QString& kind, const QString& label,
                          QVariantMap extra = {}) {
        QVariantMap n = std::move(extra);
        n[QStringLiteral("id")] = id;
        n[QStringLiteral("kind")] = kind;
        n[QStringLiteral("label")] = label;
        m_nodes.append(n);
    };
    const auto edge = [&](const QString& id, const QString& src, const QString& dst,
                          const QString& kind, const QString& role = {}) {
        QVariantMap e;
        e[QStringLiteral("id")] = id;
        e[QStringLiteral("source")] = src;
        e[QStringLiteral("target")] = dst;
        e[QStringLiteral("edgeKind")] = kind;
        if (!role.isEmpty()) {
            e[QStringLiteral("role")] = role;
        }
        m_edges.append(e);
    };

    for (const UnitNode& u : m_units) {
        QVariantMap x;
        x[QStringLiteral("unitKind")] = unitKindStr(u.kind);
        x[QStringLiteral("status")] = unitStatusStr(u.state);
        x[QStringLiteral("model")] = profileModel(u.profile.toString());
        node(u.id.toString(), QStringLiteral("agent"), u.name, x);
        if (!u.parentId.isEmpty()) {
            edge(QStringLiteral("d-%1").arg(u.id.toString()), u.parentId.toString(),
                 u.id.toString(), QStringLiteral("delegation"));
        }
    }
    for (const Session& c : m_sessions) {
        QVariantMap x;
        x[QStringLiteral("title")] = c.title;
        node(c.sessionId.toString(), QStringLiteral("session"), c.title, x);
        edge(QStringLiteral("r-%1").arg(c.sessionId.toString()), c.unitId.toString(),
             c.sessionId.toString(), QStringLiteral("runs"));
    }

    // Transport + peer + place + user nodes (mixed connection/presence).
    const auto transport = [&](const QString& id, const QString& label, const QString& connection,
                               const QString& presence) {
        QVariantMap x;
        x[QStringLiteral("connection")] = connection;
        x[QStringLiteral("presence")] = presence;
        node(id, QStringLiteral("transport"), label, x);
    };
    transport(QStringLiteral("matrix/@bot:hs.org"), QStringLiteral("matrix/@bot:hs.org"),
              QStringLiteral("connected"), QStringLiteral("available"));
    transport(QStringLiteral("internal"), QStringLiteral("internal (loopback)"),
              QStringLiteral("connected"), QStringLiteral("available"));
    transport(QStringLiteral("slack/@bot"), QStringLiteral("slack/@bot"),
              QStringLiteral("connecting"), QStringLiteral("unknown"));
    transport(QStringLiteral("matrix/@ops:hs.org"), QStringLiteral("matrix/@ops:hs.org"),
              QStringLiteral("error"), QStringLiteral("unknown"));

    QVariantMap aliceX;
    aliceX[QStringLiteral("nature")] = QStringLiteral("human");
    node(QStringLiteral("@alice:hs.org"), QStringLiteral("peer"), QStringLiteral("@alice:hs.org"),
         aliceX);
    QVariantMap bobX;
    bobX[QStringLiteral("nature")] = QStringLiteral("human");
    node(QStringLiteral("@bob:hs.org"), QStringLiteral("peer"), QStringLiteral("@bob:hs.org"), bobX);

    QVariantMap viewerX;
    viewerX[QStringLiteral("role")] = QStringLiteral("viewer");
    node(QStringLiteral("operator-bob"), QStringLiteral("user"), QStringLiteral("operator-bob"),
         viewerX);

    node(QStringLiteral("#help"), QStringLiteral("channel"), QStringLiteral("#help"), {});
    node(QStringLiteral("#secops"), QStringLiteral("channel"), QStringLiteral("#secops"), {});
    node(QStringLiteral("design-review"), QStringLiteral("room"), QStringLiteral("design-review"),
         {});

    // Transport bindings for the three transport sessions.
    edge(QStringLiteral("o-help"), QStringLiteral("s-help"), QStringLiteral("matrix/@bot:hs.org"),
         QStringLiteral("over"));
    edge(QStringLiteral("ip-help"), QStringLiteral("s-help"), QStringLiteral("#help"),
         QStringLiteral("inPlace"));
    edge(QStringLiteral("p-help-a"), QStringLiteral("s-help"), QStringLiteral("@alice:hs.org"),
         QStringLiteral("participant"), QStringLiteral("author"));

    edge(QStringLiteral("o-design"), QStringLiteral("s-design"), QStringLiteral("internal"),
         QStringLiteral("over"));
    edge(QStringLiteral("ip-design"), QStringLiteral("s-design"), QStringLiteral("design-review"),
         QStringLiteral("inPlace"));
    edge(QStringLiteral("p-design-op"), QStringLiteral("s-design"), QStringLiteral("operator-bob"),
         QStringLiteral("participant"), QStringLiteral("spectator"));

    edge(QStringLiteral("o-secops"), QStringLiteral("s-secops"), QStringLiteral("matrix/@bot:hs.org"),
         QStringLiteral("over"));
    edge(QStringLiteral("ip-secops"), QStringLiteral("s-secops"), QStringLiteral("#secops"),
         QStringLiteral("inPlace"));
    edge(QStringLiteral("p-secops-a"), QStringLiteral("s-secops"), QStringLiteral("@alice:hs.org"),
         QStringLiteral("participant"), QStringLiteral("author"));
    edge(QStringLiteral("p-secops-b"), QStringLiteral("s-secops"), QStringLiteral("@bob:hs.org"),
         QStringLiteral("participant"), QStringLiteral("author"));
}

void MockDaemonNet::computeProjections()
{
    const QString kEdgeKind = QStringLiteral("edgeKind");
    const QString kSource = QStringLiteral("source");
    const QString kTarget = QStringLiteral("target");
    const QString kId = QStringLiteral("id");
    const QString kLabel = QStringLiteral("label");
    const QString kKind = QStringLiteral("kind");

    QHash<QString, QVariantMap> byId;
    byId.reserve(m_nodes.size());
    for (const QVariantMap& n : m_nodes) {
        byId.insert(n.value(kId).toString(), n);
    }

    // --- fleet: delegation spanning tree (pre-order DFS over typed units) ---
    QHash<QString, QStringList> children;
    QSet<QString> delegated;
    for (const UnitNode& u : m_units) {
        if (!u.parentId.isEmpty()) {
            children[u.parentId.toString()].append(u.id.toString());
            delegated.insert(u.id.toString());
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

    // --- sessions roster (from typed sessions) ---
    // Per-session token usage (deterministic; drives the dashboard's live "tokens today" total). Kept
    // explicit rather than content-derived so the figures read like real usage and closing any session
    // visibly moves the total.
    static const QHash<QString, int> kTokens = {
        { QStringLiteral("s-scratch"), 320 },         { QStringLiteral("s-acme"), 880 },
        { QStringLiteral("s-build"), 540 },           { QStringLiteral("s-coder-impl"), 1240 },
        { QStringLiteral("s-coder-refactor"), 760 },  { QStringLiteral("s-review-old"), 410 },
        { QStringLiteral("s-worker-verify"), 600 },   { QStringLiteral("s-demo-blocks"), 1500 },
        { QStringLiteral("s-demo-roles"), 980 },      { QStringLiteral("s-help"), 1840 },
        { QStringLiteral("s-design"), 720 },          { QStringLiteral("s-secops"), 1100 },
    };
    QList<QVariantMap> sessionRows;
    for (const Session& c : m_sessions) {
        QVariantMap row;
        row[kId] = c.sessionId.toString();
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

    // --- channels (transport sessions grouped via Over/InPlace/Participant edges) ---
    QHash<QString, QString> overOf;
    QHash<QString, QString> placeOf;
    QHash<QString, QStringList> partsOf;
    QHash<QString, int> placeMembers;
    for (const QVariantMap& e : m_edges) {
        const QString kind = e.value(kEdgeKind).toString();
        const QString src = e.value(kSource).toString();
        const QString dst = e.value(kTarget).toString();
        if (kind == QStringLiteral("over")) {
            overOf.insert(src, dst);
        } else if (kind == QStringLiteral("inPlace")) {
            placeOf.insert(src, dst);
            placeMembers[dst] += 1;
        } else if (kind == QStringLiteral("participant")) {
            partsOf[src].append(dst);
        }
    }
    QList<QVariantMap> channelRows;
    for (auto it = overOf.constBegin(); it != overOf.constEnd(); ++it) {
        const QString sid = it.key();
        const QString transportId = it.value();
        const QString placeId = placeOf.value(sid);
        const QStringList parts = partsOf.value(sid);
        QString peerLabel = QStringLiteral("(group)");
        if (!parts.isEmpty()) {
            peerLabel = byId.value(parts.first()).value(kLabel).toString();
        } else if (!placeId.isEmpty()) {
            peerLabel = byId.value(placeId).value(kLabel).toString();
        }
        QString scope = QStringLiteral("dm");
        if (!placeId.isEmpty()) {
            const QString placeKind = byId.value(placeId).value(kKind).toString();
            scope = (placeKind == QStringLiteral("channel")) ? QStringLiteral("group")
                                                             : QStringLiteral("dm");
        }
        QVariantMap row;
        row[kId] = sid;
        row[QStringLiteral("transport")] = byId.value(transportId).value(kLabel);
        row[QStringLiteral("peer")] = peerLabel;
        row[QStringLiteral("scope")] = scope;
        row[QStringLiteral("presence")]
            = byId.value(transportId).value(QStringLiteral("presence"), QStringLiteral("unknown"));
        row[QStringLiteral("session")] = sid;
        channelRows.append(row);
    }
    m_channelsModel->setRows(channelRows);

    // --- byPeer (conversations grouped by remote Peer participant) ---
    QHash<QString, int> peerCounts;
    for (const QVariantMap& e : m_edges) {
        if (e.value(kEdgeKind).toString() != QStringLiteral("participant")) {
            continue;
        }
        const QString tgt = e.value(kTarget).toString();
        if (byId.value(tgt).value(kKind).toString() == QStringLiteral("peer")) {
            peerCounts[tgt] += 1;
        }
    }
    QList<QVariantMap> peerRows;
    for (auto it = peerCounts.constBegin(); it != peerCounts.constEnd(); ++it) {
        const QVariantMap p = byId.value(it.key());
        QVariantMap row;
        row[kId] = it.key();
        row[QStringLiteral("peer")] = p.value(kLabel);
        row[QStringLiteral("kind")] = p.value(QStringLiteral("nature"), QStringLiteral("unknown"));
        row[QStringLiteral("count")] = it.value();
        peerRows.append(row);
    }
    m_byPeerModel->setRows(peerRows);
}

} // namespace daemonnet
