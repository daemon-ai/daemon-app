// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/mock_scenario.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QtGlobal>
#include <vector>

namespace daemonapp::daemon {
namespace {

Q_LOGGING_CATEGORY(lcScenario, "daemon.mirror.mock")

const QChar kSep = QChar(0x1f);

// The demo transport instance — the SAME id the (surviving) MockTransportRegistry advertises, so
// tree-activated chats open the scenario's seeded windows. When the tree/channels surfaces port
// onto the mirror, the registry demo folds into this scenario too.
QString mockTransport() {
    return QStringLiteral("matrix/@you:matrix.org");
}

// domain enum -> canonical mirror string (the vocabulary entities_map.cpp emits and
// MirrorSessionStore's stateFromString/... consume — one round-trip, no third vocabulary).
QString sessionStateString(domain::SessionState s) {
    switch (s) {
    case domain::SessionState::Active:
        return QStringLiteral("Active");
    case domain::SessionState::Suspended:
        return QStringLiteral("Suspended");
    case domain::SessionState::Ready:
        return QStringLiteral("Ready");
    case domain::SessionState::Completed:
        return QStringLiteral("Completed");
    case domain::SessionState::Unknown:
        break;
    }
    return QStringLiteral("Unknown");
}

QString lifecycleString(domain::Lifecycle l) {
    return l == domain::Lifecycle::Live ? QStringLiteral("Live") : QStringLiteral("Durable");
}

QString roleString(domain::SessionRole r) {
    switch (r) {
    case domain::SessionRole::ManagedChild:
        return QStringLiteral("ManagedChild");
    case domain::SessionRole::EphemeralSubagent:
        return QStringLiteral("EphemeralSubagent");
    case domain::SessionRole::Primary:
        break;
    }
    return QStringLiteral("Primary");
}

QString unitKindString(domain::UnitKind k) {
    switch (k) {
    case domain::UnitKind::Orchestrator:
        return QStringLiteral("Orchestrator");
    case domain::UnitKind::Host:
        return QStringLiteral("Host");
    case domain::UnitKind::Engine:
        break;
    }
    return QStringLiteral("Engine");
}

QString unitStateString(domain::UnitState s) {
    switch (s) {
    case domain::UnitState::Running:
        return QStringLiteral("Running");
    case domain::UnitState::Finished:
        return QStringLiteral("Finished");
    case domain::UnitState::Unknown:
        break;
    }
    return QStringLiteral("Unknown");
}

// Derive the mirror `sessions` rows from the ONE bundle (id agreement with the delegated
// transcript store is the invariant the content() join needs).
std::vector<mirror::Session> deriveSessions(const daemonnet::SeedBundle& bundle) {
    std::vector<mirror::Session> out;
    out.reserve(static_cast<std::size_t>(bundle.sessions.size()));
    for (const domain::Session& s : bundle.sessions) {
        mirror::Session m;
        m.session = s.sessionId.toString();
        m.state = sessionStateString(s.state);
        m.title = s.title;
        m.bound_profile = s.boundProfile.toString();
        m.last_activity_ms = static_cast<quint64>(s.lastActivityMs);
        m.lifecycle = lifecycleString(s.lifecycle);
        m.role = roleString(s.role);
        m.pinned = s.isPinned;
        m.archived = s.isArchived;
        out.push_back(m);
    }
    return out;
}

std::vector<mirror::FleetUnit> deriveFleetUnits(const daemonnet::SeedBundle& bundle) {
    // The ordered child edges (bundle declaration order), 0x1f-joined — the G2 entity shape the
    // MirrorFleetTree render reconstructs pre-order from (roots = units no child list names).
    QHash<QString, QStringList> childIds;
    for (const domain::UnitNode& u : bundle.units) {
        if (!u.parentId.isEmpty()) {
            childIds[u.parentId.toString()].append(u.id.toString());
        }
    }
    std::vector<mirror::FleetUnit> out;
    out.reserve(static_cast<std::size_t>(bundle.units.size()));
    for (const domain::UnitNode& u : bundle.units) {
        mirror::FleetUnit f;
        f.id = u.id.toString();
        f.kind = unitKindString(u.kind);
        f.state = unitStateString(u.state);
        f.work = u.work;
        f.profile = u.profile.toString();
        f.session = u.session.toString();
        f.title = u.name;
        f.role = roleString(u.role);
        const QStringList children = childIds.value(u.id.toString());
        f.child_count = static_cast<int>(children.size());
        f.child_ids = children.join(QChar(0x1f));
        out.push_back(f);
    }
    return out;
}

mirror::ChatMessage chatMsg(const QString& conv, quint64 cursor, const QString& author,
                            const QString& text) {
    mirror::ChatMessage m;
    m.transport = mockTransport();
    m.conv = conv;
    m.cursor = cursor;
    m.id = QStringLiteral("demo-%1-%2").arg(conv).arg(cursor);
    m.author = author;
    m.text = text;
    m.timestamp = 1'700'000'000'000ULL + (cursor * 60'000ULL); // deterministic, monotonic
    return m;
}

mirror::Conversation convRow(const QString& id, const QString& title, int members) {
    mirror::Conversation c;
    c.transport = mockTransport();
    c.id = id;
    c.kind = QStringLiteral("channel");
    c.title = title;
    c.member_count = members;
    return c;
}

mirror::Contact contactRow(const QString& id, const QString& name) {
    mirror::Contact c;
    c.transport = mockTransport();
    c.id = id;
    c.display_name = name;
    c.permission = QStringLiteral("Full");
    c.presence_primitive = QStringLiteral("Available");
    return c;
}

// The demo mirror seed: the chat/persons/contacts/routing/channels demo content that used to be
// scattered across the per-seam data mocks, collapsed into the ONE declarative seed (§9). The
// (transport, conv) ids match the surviving MockTransportRegistry demo rooms so tree activation
// opens these windows.
void fillDefaultMirrorSeed(mirror::SeedSet& seed, const daemonnet::SeedBundle& bundle) {
    const QString t = mockTransport();
    const QString dev = QStringLiteral("!daemon-dev:matrix.org");
    const QString design = QStringLiteral("!design:matrix.org");

    seed.sessions = deriveSessions(bundle);
    seed.fleetUnits = deriveFleetUnits(bundle);

    seed.conversations = {convRow(dev, QStringLiteral("daemon-dev"), 3),
                          convRow(design, QStringLiteral("design"), 2)};
    seed.contacts = {contactRow(QStringLiteral("@bob:matrix.org"), QStringLiteral("Bob")),
                     contactRow(QStringLiteral("@carol:matrix.org"), QStringLiteral("Carol"))};

    mirror::Person alice;
    alice.id = QStringLiteral("p-alice");
    alice.alias = QStringLiteral("Alice");
    alice.endpoint_count = 1;
    mirror::Person bob;
    bob.id = QStringLiteral("p-bob");
    bob.alias = QStringLiteral("Bob");
    bob.endpoint_count = 1;
    seed.persons = {alice, bob};

    seed.chat.insert(t + kSep + dev,
                     {chatMsg(dev, 1, QStringLiteral("Bob"),
                              QStringLiteral("Morning! The mirror cutover branch is up.")),
                      chatMsg(dev, 2, QStringLiteral("Carol"),
                              QStringLiteral("Nice — running the parity suite now.")),
                      chatMsg(dev, 3, QStringLiteral("Me"),
                              QStringLiteral("Ping me if the seeder path diverges.")),
                      chatMsg(dev, 4, QStringLiteral("Bob"), QStringLiteral("Will do."))});
    seed.chat.insert(t + kSep + design,
                     {chatMsg(design, 1, QStringLiteral("Carol"),
                              QStringLiteral("Sketches for the routing panel are in.")),
                      chatMsg(design, 2, QStringLiteral("Me"),
                              QStringLiteral("Looks good — pin this room to the design fleet?"))});

    // Lowercase state tokens — the SAME vocabulary map_transport_account / the TransportChanged
    // patch write, so mock rows are byte-identical to daemon rows (§9).
    mirror::TransportAccount acct;
    acct.transport = t;
    acct.family = QStringLiteral("matrix");
    acct.display_name = QStringLiteral("@you:matrix.org");
    acct.connection = QStringLiteral("connected");
    acct.presence = QStringLiteral("available");
    acct.bound_profile = QStringLiteral("prof-1");
    acct.enabled = true;
    seed.transportAccounts = {acct};

    // AD (1a): the adapter catalog — the canned families the deleted MockTransportRegistry
    // advertised (matrix = the full verb set + directory; the internal Rooms loopback =
    // deliberately narrower), as canonical mirror rows (ops_json in map_adapter's shape).
    mirror::Adapter matrix;
    matrix.family = QStringLiteral("matrix");
    matrix.display_name = QStringLiteral("Matrix");
    matrix.directory = true;
    matrix.cap_rooms = true;
    matrix.cap_direct_messages = true;
    matrix.ops_json = QString::fromUtf8(
        QJsonDocument(
            QJsonObject{
                {QStringLiteral("conversationOps"),
                 QJsonObject{{QStringLiteral("create"), true},
                             {QStringLiteral("joinChannel"), true},
                             {QStringLiteral("leave"), true},
                             {QStringLiteral("delete"), true},
                             {QStringLiteral("send"), true},
                             {QStringLiteral("setTopic"), false},
                             {QStringLiteral("setTitle"), false},
                             {QStringLiteral("setDescription"), false}}},
                {QStringLiteral("membershipOps"), QJsonObject{{QStringLiteral("invite"), true},
                                                              {QStringLiteral("remove"), true},
                                                              {QStringLiteral("ban"), true},
                                                              {QStringLiteral("setRole"), true}}},
                {QStringLiteral("contactsOps"), QJsonObject{{QStringLiteral("getProfile"), true},
                                                            {QStringLiteral("actionMenu"), true},
                                                            {QStringLiteral("setAlias"), true}}},
                {QStringLiteral("rosterOps"), QJsonObject{{QStringLiteral("list"), true},
                                                          {QStringLiteral("add"), true},
                                                          {QStringLiteral("update"), true},
                                                          {QStringLiteral("remove"), true}}}})
            .toJson(QJsonDocument::Compact));
    mirror::Adapter rooms;
    rooms.family = QStringLiteral("room");
    rooms.display_name = QStringLiteral("Rooms (internal)");
    rooms.directory = false;
    rooms.cap_rooms = true;
    rooms.cap_direct_messages = true;
    rooms.ops_json = QString::fromUtf8(
        QJsonDocument(
            QJsonObject{
                {QStringLiteral("conversationOps"),
                 QJsonObject{{QStringLiteral("create"), true},
                             {QStringLiteral("joinChannel"), true},
                             {QStringLiteral("leave"), true},
                             {QStringLiteral("delete"), false},
                             {QStringLiteral("send"), true},
                             {QStringLiteral("setTopic"), false},
                             {QStringLiteral("setTitle"), false},
                             {QStringLiteral("setDescription"), false}}},
                {QStringLiteral("membershipOps"), QJsonObject{{QStringLiteral("invite"), true},
                                                              {QStringLiteral("remove"), true},
                                                              {QStringLiteral("ban"), false},
                                                              {QStringLiteral("setRole"), false}}},
                {QStringLiteral("contactsOps"), QJsonObject{{QStringLiteral("getProfile"), false},
                                                            {QStringLiteral("actionMenu"), false},
                                                            {QStringLiteral("setAlias"), false}}},
                {QStringLiteral("rosterOps"), QJsonObject{{QStringLiteral("list"), true},
                                                          {QStringLiteral("add"), true},
                                                          {QStringLiteral("update"), false},
                                                          {QStringLiteral("remove"), true}}}})
            .toJson(QJsonDocument::Compact));
    seed.adapters = {matrix, rooms};

    // AD (1a): per-transport person endpoints (the tree's Persons section joins persons ×
    // endpoints on this account) — Alice + Bob reachable on the demo Matrix account.
    mirror::PersonEndpoint aliceEp;
    aliceEp.person = QStringLiteral("p-alice");
    aliceEp.transport = t;
    aliceEp.contact = QStringLiteral("@alice:matrix.org");
    aliceEp.display_name = QStringLiteral("Alice");
    aliceEp.presence_primitive = QStringLiteral("available");
    mirror::PersonEndpoint bobEp;
    bobEp.person = QStringLiteral("p-bob");
    bobEp.transport = t;
    bobEp.contact = QStringLiteral("@bob:matrix.org");
    bobEp.display_name = QStringLiteral("Bob");
    bobEp.presence_primitive = QStringLiteral("available");
    seed.personEndpoints = {aliceEp, bobEp};

    mirror::Room devRoom;
    devRoom.transport = t;
    devRoom.room = dev;
    devRoom.name = QStringLiteral("daemon-dev");
    mirror::Room designRoom;
    designRoom.transport = t;
    designRoom.room = design;
    designRoom.name = QStringLiteral("design");
    designRoom.session = QStringLiteral("s-design");
    seed.rooms = {devRoom, designRoom};

    // One demo pin: the design room routes to the design-review session. origin_key follows the
    // canonical §3.5 form (entities_map originKeyOf: transport|group|chat|thread).
    mirror::RoutePin pin;
    pin.origin_key = t + kSep + QStringLiteral("group") + kSep + design + kSep;
    pin.transport = t;
    pin.session = QStringLiteral("s-design");
    pin.profile = QStringLiteral("prof-3");
    seed.routePins = {pin};
}

// Transcript CONTENT is mirror-served (the G2 flip: MirrorSessionStore::content() projects
// `w_transcript_blocks`). The ONE bundle carries the transcript as canonical BLOCKS: one seeded
// block per session into the mirror window. Each seed session's prose is modeled as one
// assistant `Message` block (seq 1); the rich block-showcase demos (`s-demo-*`) ride the same
// path, so their leading fence is an assistant turn — faithfully re-decomposing them into typed
// reasoning/tool blocks is blocked on the lossy mirror TranscriptBlock entity (no tone/duration/
// stdout fields), a post-M5 entity enrichment. (AD: the derived legacy-baseline leg died with
// the InMemory delegate — the mirror is the only content source.)
void seedTranscriptsFromBundle(MockScenario& s) {
    for (const domain::Session& session : s.bundle.sessions) {
        if (session.content.isEmpty()) {
            continue;
        }
        mirror::TranscriptBlock block;
        block.session = session.sessionId.toString();
        block.seq = 1;
        block.kind = QStringLiteral("Message");
        block.role = QStringLiteral("assistant");
        block.text = session.content;
        s.mirror.seed.transcriptBlocks.push_back(block);
    }
}

// The scripted timeline (§9): deterministic liveness — message arrivals + a presence flip. Each
// step is one Seeder batch (origin = seeder) through the same apply pipeline.
std::vector<mirror::ScenarioStep> defaultTimeline() {
    const QString dev = QStringLiteral("!daemon-dev:matrix.org");
    const QString design = QStringLiteral("!design:matrix.org");
    std::vector<mirror::ScenarioStep> steps;
    steps.push_back({8'000, [dev](mirror::Seeder& s) {
                         s.appendMessage(chatMsg(dev, 5, QStringLiteral("Bob"),
                                                 QStringLiteral("Parity suite is green here.")));
                     }});
    steps.push_back({21'000, [design](mirror::Seeder& s) {
                         s.appendMessage(
                             chatMsg(design, 3, QStringLiteral("Carol"),
                                     QStringLiteral("Updated the sketches — take two.")));
                     }});
    steps.push_back({34'000, [](mirror::Seeder& s) {
                         mirror::TransportAccount acct;
                         acct.transport = mockTransport();
                         acct.family = QStringLiteral("matrix");
                         acct.display_name = QStringLiteral("@you:matrix.org");
                         acct.connection = QStringLiteral("connected");
                         acct.presence = QStringLiteral("away");
                         acct.bound_profile = QStringLiteral("prof-1");
                         acct.enabled = true;
                         s.upsertTransportAccount(acct);
                     }});
    return steps;
}

// The scripted verb outcomes (§9). Rejection rules are MANDATORY fixtures: "/reject ..." drives
// the §6.5 lane-pause path, "/timeout ..." the §6.3 transient/backoff path; everything else acks
// and (for ConvSend) echoes a provenance-stamped mirror row (§6.6, origin_op = op_id).
mirror::VerbScript defaultVerbScript() {
    mirror::VerbScript script;

    mirror::VerbRule reject = mirror::VerbScript::reject(
        QStringLiteral("ConvSend"), QStringLiteral("Forbidden"),
        QStringLiteral("Rejected by the mock scenario script (\"/reject\")"));
    reject.matcher = [](const QJsonObject& args) {
        return args.value(QStringLiteral("message"))
            .toString()
            .startsWith(QStringLiteral("/reject"));
    };
    script.add(reject);

    mirror::VerbRule timeout = mirror::VerbScript::timeout(QStringLiteral("ConvSend"), 1'200);
    timeout.matcher = [](const QJsonObject& args) {
        return args.value(QStringLiteral("message"))
            .toString()
            .startsWith(QStringLiteral("/timeout"));
    };
    script.add(timeout);

    script.add(mirror::VerbScript::ok(QStringLiteral("ConvSend"), /*echoDelayMs=*/400));

    // AD (§6.4 session-meta lane): the MANDATORY rejection fixture — a rename whose title starts
    // "/reject" is Forbidden (drives the §6.5 lane pause + the metaUpdateFailed toast in mock);
    // every other SessionUpdateMeta acks and echoes the patched row (§6.6 landing, roster
    // re-projects event-driven).
    mirror::VerbRule metaReject = mirror::VerbScript::reject(
        QStringLiteral("SessionUpdateMeta"), QStringLiteral("Forbidden"),
        QStringLiteral("Rejected by the mock scenario script (\"/reject\")"));
    metaReject.matcher = [](const QJsonObject& args) {
        return args.value(QStringLiteral("title")).toString().startsWith(QStringLiteral("/reject"));
    };
    script.add(metaReject);
    script.add(mirror::VerbScript::ok(QStringLiteral("SessionUpdateMeta"), /*echoDelayMs=*/120));
    return script;
}

MockScenario buildDefault() {
    MockScenario s;
    s.name = QStringLiteral("default");
    s.bundle = daemonnet::defaultSeedBundle();
    s.mirror.apiVersion = 39;
    fillDefaultMirrorSeed(s.mirror.seed, s.bundle);
    seedTranscriptsFromBundle(s); // canonical transcript blocks + derived legacy content
    s.mirror.timeline = defaultTimeline();
    s.mirror.verbScript = defaultVerbScript();
    return s;
}

MockScenario buildApi38() {
    MockScenario s = buildDefault();
    s.name = QStringLiteral("api38");
    s.mirror.apiVersion = 38; // degraded reconnect + auto-replay HOLD (§5.6/§6.8)
    return s;
}

MockScenario buildEmpty() {
    MockScenario s;
    s.name = QStringLiteral("empty");
    s.mirror.apiVersion = 39;
    s.mirror.verbScript = defaultVerbScript(); // outcomes still scripted; just nothing seeded
    return s;
}

} // namespace

QStringList mockScenarioNames() {
    return {QStringLiteral("default"), QStringLiteral("api38"), QStringLiteral("empty")};
}

MockScenario mockScenarioByName(const QString& name) {
    if (name.isEmpty() || name == QStringLiteral("default")) {
        return buildDefault();
    }
    if (name == QStringLiteral("api38")) {
        return buildApi38();
    }
    if (name == QStringLiteral("empty")) {
        return buildEmpty();
    }
    qCWarning(lcScenario).noquote()
        << "unknown DAEMON_APP_MOCK_SCENARIO" << name
        << "- falling back to 'default' (known:" << mockScenarioNames().join(QStringLiteral(", "))
        << ")";
    return buildDefault();
}

MockScenario mockScenarioFromEnvironment() {
    return mockScenarioByName(QString::fromUtf8(qgetenv("DAEMON_APP_MOCK_SCENARIO")).trimmed());
}

} // namespace daemonapp::daemon
