// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/app_service_graph.h"

#include "accounts/mock_accounts_service.h"
#include "auth/auth_flow_controller.h"
#include "auth/mock_auth_flow_service.h"
#include "automation/mock_cron_store.h"
#include "config/mock_daemon_config.h"
#include "connection/iconnection_service.h"
#include "connection/mock_connection_service.h"
#include "daemon/cached_session_store.h"
#include "daemon/daemon_accounts_service.h"
#include "daemon/daemon_approvals_inbox.h"
#include "daemon/daemon_auth_flow_service.h"
#include "daemon/daemon_cache_store.h"
#include "daemon/daemon_chat_service.h"
#include "daemon/daemon_checkpoint_timeline.h"
#include "daemon/daemon_connection_service.h"
#include "daemon/daemon_contacts_service.h"
#include "daemon/daemon_dashboard.h"
#include "daemon/daemon_fleet_tree.h"
#include "daemon/daemon_fs_service.h"
#include "daemon/daemon_model_catalog.h"
#include "daemon/daemon_persons_service.h"
#include "daemon/daemon_presence_service.h"
#include "daemon/daemon_profile_store.h"
#include "daemon/daemon_provider_catalog.h"
#include "daemon/daemon_session_roster.h"
#include "daemon/daemon_session_settings.h"
#include "daemon/daemon_tool_inventory.h" // [wave2:app-approvals-safety] D2
#include "daemon/daemon_transport.h"
#include "daemon/daemon_transport_registry.h"
#include "daemon/engine_identity.h"
#include "daemon/feedback_repository.h"
#include "daemon/node_api_client.h"
#include "daemon/principal_model.h"
#include "daemon/repositories.h"
#include "daemon/subscription_manager.h"
#include "daemonnet/mock_fleet_source.h"
#include "feedback/mock_feedback.h"
#include "firstrun/first_run_model.h"
#include "fleet/mock_approvals_inbox.h"
#include "fleet/mock_dashboard.h"
#include "fleet/mock_fleet_tree.h"
#include "fs/local_disk_fs_service.h"
#include "memory/mock_memory_service.h"
#include "models/mock_model_catalog.h"
#include "models/mock_provider_catalog.h"
#include "nav/nav_controller.h"
#include "persistence/in_memory_session_store.h"
#include "profiles/mock_profile_store.h"
#include "session/mock_checkpoint_timeline.h"
#include "session/mock_session_settings.h"
#include "settings/qt_settings_store.h"
#include "setup/agent_setup_model.h"
#include "tools/mock_tool_inventory.h" // [wave2:app-approvals-safety] D2
#include "transports/mock_contacts_service.h"
#include "transports/mock_persons_service.h"
#include "transports/mock_presence_service.h"
#include "transports/mock_transport_registry.h"
#include "update/update_manager.h"

#include <memory>
#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QString>
#include <QtGlobal>

#ifdef DAEMON_APP_HAVE_MIRROR_SUBSTRATE
#include "daemon/daemon_fetch_executor.h"
#include "daemon/ingestor_bridge.h" // translateNodeEvent (DecodedNodeEvent -> mirror::NodeEvent)
#include "daemon/mirror_session_store.h" // mirror A7 (M4): the 6->1 session store projection
#include "daemon/mock_scenario.h"        // mirror A8 (M5): the mock seed-scenario catalog
#include "daemon/mock_scenario_host.h"   // mirror A8 (M5): the mock scenario driver (Seeder)
#include "local_database.h"
#include "mirror/mirror_service.h"
#include "outbox.h"
#include "verb_class.h"

#include <QCryptographicHash>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#endif

namespace daemonapp::daemon {

#ifdef DAEMON_APP_HAVE_MIRROR_SUBSTRATE
namespace {

// Per-identity mirror-db paths (spec 09 §4.5), matching the DaemonCacheStore / LocalDatabase
// convention EXACTLY (one namespace-hash convention across all three per-identity files):
// SHA-256(userKey) hex truncated to 16, suffixed to the base name, in AppDataLocation beside
// daemon_cache.db. An empty key = the shared/default file (pre-auth / local-trust).
class MirrorDbPaths final : public mirror::DbPathProvider {
public:
    explicit MirrorDbPaths(QString userKey) : user_key_(std::move(userKey)) {}

    [[nodiscard]] QString mirrorDbPath() const override {
        QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (dir.isEmpty()) {
            dir = QDir::tempPath();
        }
        QDir().mkpath(dir);
        if (user_key_.isEmpty()) {
            return QDir(dir).filePath(QStringLiteral("mirror.db"));
        }
        const QByteArray h =
            QCryptographicHash::hash(user_key_.toUtf8(), QCryptographicHash::Sha256)
                .toHex()
                .left(16);
        return QDir(dir).filePath(QStringLiteral("mirror-%1.db").arg(QString::fromLatin1(h)));
    }

    // The precious local-<id>.db manages its own namespaced path (LocalDatabase, same hash);
    // Persistence never consumes this member (declared for the DbPathProvider contract).
    [[nodiscard]] QString localDbPath() const override { return {}; }

private:
    QString user_key_;
};

} // namespace
#endif

ServiceMode serviceModeFromEnvironment(ServiceMode fallback) {
    const QByteArray raw = qgetenv("DAEMON_APP_SERVICE_MODE");
    if (raw.isEmpty()) {
        return fallback;
    }
    const QString value = QString::fromUtf8(raw).trimmed().toLower();
    if (value == QStringLiteral("daemon")) {
        return ServiceMode::Daemon;
    }
    if (value == QStringLiteral("mock")) {
        return ServiceMode::Mock;
    }
    return fallback;
}

AppServiceGraph createAppServiceGraph(ServiceMode mode, QObject* owner) {
    AppServiceGraph graph;
    graph.settings = new settings::QtSettingsStore(owner);
    graph.connection = mode == ServiceMode::Daemon
                           ? static_cast<connection::IConnectionService*>(
                                 new DaemonConnectionService(owner, graph.settings))
                           : static_cast<connection::IConnectionService*>(
                                 new connection::MockConnectionService(owner));
    graph.daemonConfig = new config::MockDaemonConfig(owner);
    // User-feedback + telemetry-consent seam. Mock-first: recorded in memory and
    // consent persisted locally; the daemon adapter (FeedbackSubmit /
    // TelemetryConsentGet/Set, wire v31) replaces it in a later integration phase.
    graph.feedback = new feedback::MockFeedback(owner);
    graph.fs = new fs::LocalDiskFsService(
        graph.daemonConfig->value(QStringLiteral("workspace/root")).toString(), QString(), owner);
    graph.memory = new memory::MockMemoryService(owner);
    graph.nav = new nav::NavController(owner);
    // firstRun + accounts + modelCatalog + profiles are constructed per-mode below (after the
    // repositories the daemon-backed services need); firstRun binds the resulting modelCatalog for
    // inference readiness.
    // The mock fleet/roster/session seed (MockFleetSource): the source the fleet/roster mocks + the
    // mock session store project from (M3 — routing/integrations now live on the mirror; the
    // deleted mock net's routing + transports-tree + patch-bay are gone). In daemon mode the
    // fleet/roster/store below are deleted + rebuilt daemon-backed, so this seed is used only in
    // mock mode; building it unconditionally keeps the pre-mode-branch surfaces simple.
#ifdef DAEMON_APP_HAVE_MIRROR_SUBSTRATE
    // mirror A8 (spec 09 §9, M5): the mock SCENARIO is the only mock seed source — resolved once,
    // feeding BOTH the legacy delegate stores (its SeedBundle, here) and the mock mirror (its
    // canonical seed, in the mirror block below), so the two sides' session ids provably agree
    // (the delegated content() join depends on it). Daemon mode gets an empty placeholder (the
    // pre-branch mock services it feeds are deleted + rebuilt daemon-backed below).
    const MockScenario mockScenario =
        mode == ServiceMode::Daemon ? MockScenario{} : mockScenarioFromEnvironment();
    auto* fleetSource = new daemonnet::MockFleetSource(mockScenario.bundle, owner);
#else
    auto* fleetSource = new daemonnet::MockFleetSource(owner);
#endif
    // mirror A8 (M5): roster + dashboard are built at the END of this factory in BOTH modes — the
    // roster projects the final storeMirror (the 6→1 read everywhere; MockSessionRoster died with
    // the cutover) and the dashboard observes the FINAL seam pointers.
    graph.fleetTree = new fleet::MockFleetTree(fleetSource, owner);
    graph.approvals = new fleet::MockApprovalsInbox(owner);
    // [wave2:app-approvals-safety] D2: tool inventory starts as the canned mock; the daemon branch
    // below REPLACES it with the repo-backed DaemonToolInventory (ToolList).
    graph.tools = new tools::MockToolInventory(owner);
    // Cron has NO node wire op yet (the daemon-api codec subset carries none), so in Daemon mode
    // it must render EMPTY rather than pass its illustrative demo rows off as node-backed data
    // (render honesty; see seam_migration.h). Mock mode keeps the demo seed. (Routing moved to the
    // mirror store's pin table in M3; the legacy intent->model surfaces are retired.)
    const bool seedMockDemo = mode != ServiceMode::Daemon;
    graph.cron = new automation::MockCronStore(owner, seedMockDemo);
    // Transport-adapter seams (daemon-transport-adapter-spec.md): inert mocks until a daemon
    // adapter decodes transport_adapters / transport_instances. The registry advertises the
    // existing adapter families for the "Add channel" picker; presence reports offline/unknown.
    graph.transportRegistry = new transports::MockTransportRegistry(owner);
    graph.presence = new transports::MockPresenceService(owner);
    // [acct-mgmt] Transport contacts / roster (Phase D): inert canned mock until a daemon adapter
    // decodes the node's roster ops (replaced in the Daemon branch below).
    graph.contacts = new transports::MockContactsService(owner);
    // [integrations wire v38] Person registry: inert canned mock until the integrations tree's
    // persons sections port onto the mirror (post-M5; the tree composes directly from
    // IPersonsService). The Daemon branch below replaces it with the DaemonPersonsService.
    graph.persons = new transports::MockPersonsService(owner);
    // mirror A8 (M5): NO mock chat seam. The chat surfaces read the seeded mock MIRROR (window
    // lens) and send through the ConvSend outbox lane + scripted outcomes — MockChatService died
    // with the M5 cutover. `chat` stays null in mock; daemon mode builds DaemonChatService below
    // (the dual-write legacy path until A9 retires the seam).
    // sessionSettings is constructed per-mode below (the daemon variant needs nodeApi to send
    // SetSessionMode); checkpoints starts as the mock and is REPLACED in the daemon branch with
    // the repo-backed DaemonCheckpointTimeline (CheckpointList/CheckpointRewind; E4/TOOL-9).
    graph.checkpoints = new session::MockCheckpointTimeline(owner, seedMockDemo);
    graph.cache = new DaemonCacheStore(QString(), owner);
    // The WhoAmI / principal model (advisory capability gating). Always present; populated from
    // AuthOk in daemon mode and cleared on disconnect.
    graph.principal = new PrincipalModel(owner);
    auto* daemonConnection = qobject_cast<DaemonConnectionService*>(graph.connection);
    if (daemonConnection != nullptr) {
        // Reuse the connection seam's transport + client so repositories share the single
        // serialized request pipeline (and the Health probe) rather than a parallel one.
        graph.nodeApi = daemonConnection->client();
    } else {
        auto* transport = new DaemonTransport(owner);
        graph.nodeApi = new NodeApiClient(transport, owner);
    }
    graph.sessions = new SessionRepository(graph.nodeApi, graph.cache, owner);
    graph.profileRepository = new ProfileRepository(graph.nodeApi, graph.cache, owner);
    graph.models = new ModelRepository(graph.nodeApi, graph.cache, owner);
    graph.providerRepository = new ProviderRepository(graph.nodeApi, graph.cache, owner);
    graph.approvalRepository = new ApprovalRepository(graph.nodeApi, graph.cache, owner);
    // [wave2:app-approvals-safety] D2/D4: tool inventory + per-session fingerprint repositories
    // (inert without a connection; only projected into facades in the daemon branch below).
    graph.toolRepository = new ToolRepository(graph.nodeApi, graph.cache, owner);
    graph.fingerprintRepository = new FingerprintRepository(graph.nodeApi, graph.cache, owner);
    graph.checkpointRepository = new CheckpointRepository(graph.nodeApi, graph.cache, owner);
    if (daemonConnection != nullptr) {
        // Durable checkpoints (E4/TOOL-9): replace the (empty-seeded) mock timeline with the
        // repo-backed one over CheckpointList/CheckpointRewind. The mock built above is parented
        // to `owner`; drop it for the daemon one.
        delete graph.checkpoints;
        graph.checkpoints = new DaemonCheckpointTimeline(graph.checkpointRepository, owner);
    }
    graph.credentialRepository = new CredentialRepository(graph.nodeApi, graph.cache, owner);
    // The foreign-agent catalog (foreign engines): backs the engine picker + Agents settings.
    graph.agents = new AgentRepository(graph.nodeApi, graph.cache, owner);
    // Interactive auth (AuthBegin/AuthComplete): the wire seam behind the AuthFlowSheet.
    graph.authRepository = new AuthRepository(graph.nodeApi, graph.cache, owner);

    if (daemonConnection != nullptr) {
        // Daemon mode reads sessions through the cache projection rather than the local sqlite
        // store, and kicks a SessionsQuery once the Health probe reports the daemon is ready so
        // the cache (and therefore the UI) populates end-to-end.
        graph.store = new CachedSessionStore(graph.cache, graph.sessions, owner);
        // User feedback + telemetry consent (wire v32): replace the mock seam with the
        // daemon-backed adapter over FeedbackSubmit / TelemetryConsentGet/Set. The mock built above
        // is parented to `owner`; drop it for the daemon one. Consent is seeded on connect-ready
        // below.
        delete graph.feedback;
        auto* daemonFeedback = new DaemonFeedback(graph.nodeApi, owner);
        graph.feedback = daemonFeedback;
        // Provider credentials + model discovery are daemon-backed in this mode (CON-4 / CON-6).
        graph.accounts = new accounts::DaemonAccountsService(graph.credentialRepository,
                                                             graph.profileRepository, owner);
        // Interactive auth over the real AuthProviders/AuthBegin/AuthComplete ops.
        graph.authFlow = new auth::DaemonAuthFlowService(graph.authRepository, owner);
        // Provider + credential + profile repositories back the Providers tab with the node's
        // real ProviderCatalog and credential presence (no hardcoded provider fiction).
        graph.modelCatalog = new models::DaemonModelCatalog(
            graph.models, graph.providerRepository, graph.credentialRepository,
            graph.profileRepository, graph.sessions, owner);
        // Node-driven provider/model discovery (ProviderCatalog/ProviderModels), sharing the model
        // catalog for the local [installed]+Discover compose.
        graph.providerCatalog =
            new DaemonProviderCatalog(graph.providerRepository, graph.modelCatalog, owner);
        graph.profiles = new profiles::DaemonProfileStore(graph.profileRepository, owner);
        // Per-session approval mode (CHA-4): the setter sends SetSessionMode to the node.
        // [wave2:app-approvals-safety] D4: also backs the per-session remembered-fingerprint list
        // (FingerprintList/Revoke) via the FingerprintRepository.
        graph.sessionSettings =
            new DaemonSessionSettings(graph.nodeApi, graph.fingerprintRepository, owner);
        // PRO-11: pending-approvals inbox backed by the ApprovalRepository (ApprovalsPending poll
        // on ready + ApprovalDecide), replacing the mock fleet inbox.
        graph.approvals = new DaemonApprovalsInbox(graph.approvalRepository, owner);
        // [wave2:app-approvals-safety] D2: replace the canned tool inventory with the repo-backed
        // one over ToolList. The mock built above is parented to `owner`; drop it for the daemon
        // one.
        delete graph.tools;
        graph.tools = new DaemonToolInventory(graph.toolRepository, owner);
        // Daemon-backed, offline-first fleet/subagent tree (PRO-9/10): replace the mock fleet tree
        // with one projected from the cached Tree query. The mock built above is parented to
        // `owner`; drop it for the daemon one. Built before the subscription feed so the feed can
        // re-query the tree on a roster delta (a fleet unit IS a durable session - FIX 2).
        graph.fleetRepository = new FleetRepository(graph.nodeApi, graph.cache, owner);
        // [wave2:app-delegation] F7/DEL-7: read-only delegation guardrail ceilings (node policy).
        graph.capsRepository = new CapsRepository(graph.nodeApi, owner);
        // Phase F: the node OpenAI-gateway control seam (GatewayGet/GatewaySet). Ops-only; it never
        // polls Health (that is the connection service's single cache). Adjacent to capsRepository,
        // both being read/light node-policy seams built with the daemon connection.
        graph.gatewayRepository = new GatewayRepository(graph.nodeApi, owner);
        delete graph.fleetTree;
        // [wave2:app-delegation] F3: fleet rows carry engine/lifetime decoded straight off the wire
        // UnitNode (v29) — no client-side profile-join (the node peer is v29, so the wire answers
        // directly, per-child, including foreign-engine children under unknown profiles).
        graph.fleetTree = new fleet::DaemonFleetTree(graph.fleetRepository, owner);
        // L3 node-wide event feed (daemon-sync-protocol §5): one EventsSince stream that routes
        // out-of-focus changes (roster/meta -> debounced roster refetch + tree re-query, approvals
        // -> badge, downloads -> models, session-advanced -> focused-engine nudge, resync ->
        // baseline) so the client stops polling and stops full-refetching on every change.
        graph.subscriptions = new SubscriptionManager(
            graph.nodeApi, graph.sessions, graph.approvalRepository, graph.models, graph.cache,
            graph.fleetRepository, graph.profileRepository, owner);
        // Daemon-backed filesystem: replace the dev local-disk seam over the NodeApi fs_* ops - the
        // only path that reaches a remote/embedded host's workspace. The common LocalDiskFsService
        // built above is parented to `owner`; drop it for the daemon one.
        delete graph.fs;
        graph.fs = new fs::DaemonFsService(graph.nodeApi, graph.cache, owner);
        // Daemon-backed, offline-first Channels read surface (story 04: EIO-1/3/8/9): replace the
        // inert mock transport registry + presence with ones projected from the node's
        // TransportAdapters / TransportInstances (+ ConvList per account). The mocks above are
        // parented to `owner`; drop them for the daemon ones.
        graph.transportRepository = new TransportRepository(graph.nodeApi, graph.cache, owner);
        // [wave2:app-channels-liveness] B5: let the event feed patch live transport presence in
        // place (the repo is built after the SubscriptionManager, so wire it via the setter).
        if (graph.subscriptions != nullptr) {
            graph.subscriptions->setTransportRepository(graph.transportRepository);
        }
        delete graph.transportRegistry;
        graph.transportRegistry =
            new transports::DaemonTransportRegistry(graph.transportRepository, owner);
        delete graph.presence;
        graph.presence = new transports::DaemonPresenceService(graph.transportRepository, owner);
        // [acct-mgmt] Daemon-backed transport contacts / roster (Phase D, wire v34): replace the
        // inert mock with the DaemonContactsService projecting the ContactsRepository. The feed's
        // ContactsChanged event refetches a transport's roster in place (wired via the setter
        // because the repo is built after the SubscriptionManager).
        graph.contactsRepository = new ContactsRepository(graph.nodeApi, graph.cache, owner);
        if (graph.subscriptions != nullptr) {
            graph.subscriptions->setContactsRepository(graph.contactsRepository);
        }
        delete graph.contacts;
        graph.contacts = new transports::DaemonContactsService(graph.contactsRepository, owner);
        // [integrations wire v38] Daemon-backed person registry (PersonList) + native chat
        // (ConvHistory / ConvSend): replace the inert mocks with services projecting the new
        // repositories. The feed's PersonsChanged refetches PersonList and MessagesChanged
        // refetches the affected conversation's ConvHistory — wired via the setters (the repos are
        // built after the SubscriptionManager).
        graph.personsRepository = new PersonsRepository(graph.nodeApi, graph.cache, owner);
        graph.chatRepository = new ChatRepository(graph.nodeApi, graph.cache, owner);
        if (graph.subscriptions != nullptr) {
            graph.subscriptions->setPersonsRepository(graph.personsRepository);
            graph.subscriptions->setChatRepository(graph.chatRepository);
        }
        delete graph.persons;
        graph.persons = new transports::DaemonPersonsService(graph.personsRepository, owner);
        // (chat is null outside daemon mode since M5 — no mock to delete.)
        graph.chat = new transports::DaemonChatService(graph.chatRepository, owner);
        // Daemon-backed, offline-first session roster + dashboard (replaces the mock pair). The
        // roster projects the (offline-first) CachedSessionStore; the dashboard derives its
        // counters from the FINAL seam pointers (DaemonFleetTree + DaemonApprovalsInbox) and its
        // health from the connection. Both are constructed at the END of this factory (M5: in
        // both modes), so nothing here observes a swapped-out seam pointer.
        // The real routing manager (M3): RoutingRepository is the node-authoritative DIRECT
        // mutation seam (RoutingBindChat/Unbind — it IS-A daemonnet::IRoutingActions the routing
        // manager view-model drives). Reads no longer flow through it: the routing pin table lives
        // on the mirror store (RoutingListChats → route_pins), fetched by the ingestor and
        // re-listed on the routing refresh triggers wired in the mirror block below. Its own
        // in-memory cache is now dead (residual debt; see LEDGER-a6).
        graph.routingRepository = new RoutingRepository(graph.nodeApi, graph.cache, owner);
        // [waveB:app-v30] D2: the feed's MembershipChanged (self removal) re-lists the routing pins
        // (the node reconciled them); wired now that the routing repo exists.
        if (graph.subscriptions != nullptr) {
            graph.subscriptions->setRoutingRepository(graph.routingRepository);
        }
        // On connect-ready, populate sessions + profiles + credentials + models so the onboarding
        // provider/model step and the shell reflect the daemon end-to-end. Fire only on the
        // transition INTO ready: stateChanged also fires for statusMessage churn (e.g. the
        // "Reconnecting..." status) and the heartbeat re-affirming ready, which must not trigger a
        // re-refresh storm; a genuine reconnect (connecting -> ready) does re-sync once, as
        // desired.
        // Bind the WhoAmI principal from AuthOk and namespace the cache per user BEFORE the ready
        // refresh storm below, so a user switch never surfaces the prior user's cached rows. AuthOk
        // precedes "ready" (Health is answered only after auth), so the ordering holds.
        QObject::connect(
            graph.nodeApi, &NodeApiClient::authenticated, graph.principal,
            [principal = graph.principal, cache = graph.cache](const DecodedPrincipalView& view) {
                principal->setPrincipal(view);
                cache->setUserNamespace(view.userId);
            });
        // Clear the principal when the connection drops to offline (logout / unreachable), so
        // capability-gated surfaces re-gate closed.
        QObject::connect(graph.connection, &connection::IConnectionService::stateChanged,
                         graph.principal, [principal = graph.principal, conn = graph.connection] {
                             if (conn->state() == QStringLiteral("offline")) {
                                 principal->clear();
                             }
                         });

        auto wasReady = std::make_shared<bool>(false);
        QObject::connect(
            graph.connection, &connection::IConnectionService::stateChanged, graph.sessions,
            [conn = graph.connection, sessions = graph.sessions, profiles = graph.profileRepository,
             credentials = graph.credentialRepository, models = graph.models,
             providers = graph.providerRepository, approvals = graph.approvalRepository,
             subscriptions = graph.subscriptions, fs = graph.fs, fleet = graph.fleetRepository,
             transports = graph.transportRepository, routing = graph.routingRepository,
             agents = graph.agents, authRepo = graph.authRepository,
             toolRepo = graph.toolRepository,   // [wave2:app-approvals-safety] D2
             gateway = graph.gatewayRepository, // Phase F: node OpenAI-gateway status
             feedback = daemonFeedback,         // wire v32: seed telemetry consent
             persons = graph.personsRepository, // [integrations wire v38] person registry
             wasReady] {
                const bool nowReady = conn->ready();
                if (nowReady && !*wasReady) {
                    // Initial baseline once per (re)connect; the EventsSince feed then keeps the
                    // surfaces fresh incrementally instead of re-running this storm on every
                    // change.
                    sessions->refreshSessions();
                    profiles->refreshProfiles();
                    credentials->refreshList();
                    models->refreshModels();
                    models->refreshCurrent();
                    providers->refreshProviders(); // node-driven provider/model discovery
                    approvals->refreshPending();
                    toolRepo->refreshTools(); // [wave2:app-approvals-safety] D2 tool inventory
                    fs->listRoots();          // populate the daemon-backed file roots
                    fleet->refreshTree(QStringLiteral("baseline")); // PRO-9: baseline the tree
                    // Channels: the adapter picker + configured accounts/status dots (EIO-1/3/9).
                    transports->refreshAdapters();
                    transports->refreshInstances();
                    // [waveB:app-v30] D2: seed each account's ConvList once per connect (the
                    // baseline that replaces the retired per-tab-enter / per-expand polling; the
                    // feed's ConversationsChanged / MembershipChanged keeps them fresh after).
                    transports->refreshAllConversations();
                    // [integrations wire v38] The node's person/metacontact registry (the Persons
                    // tree section); refetched incrementally on the PersonsChanged feed event.
                    persons->refresh();
                    // Routing (M3): the reconnect staleness scan already fetches the pin table into
                    // the mirror (routing is a scanned collection); this repo re-list only keeps
                    // the dead cache warm + fires routesRefreshed → the mirror re-fetch wired
                    // below. Per-account rooms follow the instances refresh (wired in the mirror
                    // block).
                    routing->refreshChats();
                    // Foreign engines: the catalog for the engine picker + Agents settings.
                    agents->refreshCatalog();
                    // Interactive-auth family discovery (the AuthFlowSheet's provider list).
                    authRepo->refreshProviders();
                    // Phase F: read the node OpenAI-gateway status once per connect (there is no
                    // gateway node-event; the settings/status surfaces re-fetch on focus/toggle).
                    gateway->refresh();
                    // Seed the node-owned telemetry consent (wire v32) so the settings toggle
                    // reflects the node's stored state.
                    feedback->refreshConsent();
                    subscriptions->start();
                } else if (!nowReady && *wasReady) {
                    subscriptions->stop();
                }
                *wasReady = nowReady;
            });
        // Daemon mode is partially live: be explicit so testers/operators don't read it as a fully
        // daemon-backed app. Live now: connection (Health), sessions (SessionsQuery), accounts
        // (CredentialSet/List), modelCatalog (Models/ModelCurrent). Still mock below.
        qInfo().noquote()
            << "AppServiceGraph: ServiceMode::Daemon - live seams: connection, "
               "sessions(cache; pin/archive/title via SessionUpdateMeta), "
               "accounts(credentials), modelCatalog(models), profiles, "
               "approvals(ApprovalsPending/Decide), sessionSettings(SetSessionMode), fs(fs_*), "
               "fleetTree(Tree), roster/dashboard(offline-first over cache/fleet/approvals), "
               "transports/presence(TransportAdapters/Instances+ConvList), "
               "checkpoints(CheckpointList/Rewind), "
               "routing(mirror store pin table; RoutingBindChat/Unbind mutations); "
               "still mock: daemonConfig, memory; node-blocked (NO wire op yet - rendered EMPTY, "
               "not mock demo data): cron, routing delivery/handover panel.";
    } else {
        // Mock mode: a non-persisted in-memory store re-seeded fresh from the mock fleet seed each
        // run (deterministic, always matches the current seed - no stale-db drift). The sidebar /
        // list / transcript and the Fleet/Sessions pages all reflect the one MockFleetSource seed.
        graph.store = new persistence::InMemorySessionStore(fleetSource, owner);
        graph.modelCatalog = new models::MockModelCatalog(owner);
        graph.providerCatalog = new models::MockProviderCatalog(graph.modelCatalog, owner);
        graph.accounts = new accounts::MockAccountsService(owner);
        graph.profiles = new profiles::MockProfileStore(owner);
        graph.sessionSettings = new session::MockSessionSettings(owner);
        // Simulated interactive auth (no node): the sheet's state machine still exercises.
        graph.authFlow = new auth::MockAuthFlowService(owner);
    }
    // Shared engine-identity facade (C3/C4/C5): resolves session/profile -> engine chip label,
    // joining the (per-mode) profile store + session settings with the foreign-agent catalog
    // (null-tolerant: in mock mode the catalog is inert, so the label degrades to the agent name).
    graph.engineIdentity =
        new EngineIdentity(graph.profiles, graph.sessionSettings, graph.agents, owner);
    // Shared write-side setup view-model (Phase C): the one engine+backend+inference pipeline the
    // GUI/TUI setup surfaces bind and FirstRunModel delegates to. Built from the interface seams;
    // the foreign-agent catalog + EngineIdentity summariser are INJECTED here so the setup lib
    // stays free of the daemon layer.
    graph.agentSetup = new setup::AgentSetupModel(graph.profiles, graph.accounts,
                                                  graph.providerCatalog, graph.modelCatalog, owner);
    graph.agentSetup->setEngineIdentity(graph.engineIdentity);
    // Phase F: drive the setup form's NodeProvider gateway affordance from real gateway state. A
    // NodeProvider foreign setup reports gatewayRequired=true until the gateway is enabled AND
    // listening; mirror GatewayRepository::statusChanged into AgentSetupModel::setGatewayEnabled so
    // the inline "Gateway is disabled — [Enable]" warning clears once the node's gateway is live.
    // Daemon mode only (the repo is null in mock).
    if (graph.gatewayRepository != nullptr) {
        const auto pushGatewayEnabled = [agentSetup = graph.agentSetup,
                                         gateway = graph.gatewayRepository] {
            agentSetup->setGatewayEnabled(gateway->enabled() && gateway->listening());
        };
        QObject::connect(graph.gatewayRepository, &GatewayRepository::statusChanged,
                         graph.agentSetup, pushGatewayEnabled);
        pushGatewayEnabled(); // seed from whatever status is already known
    }
    // Feed the foreign-agent catalog rows into the setup model's engineChoices, refreshing on every
    // catalog reflection. `verification` is the node-derived verdict, rendered verbatim.
    const auto pushForeignAgents = [agentSetup = graph.agentSetup, agents = graph.agents] {
        QVariantList rows;
        for (const DecodedAgentEntry& e : agents->entries()) {
            QVariantMap row;
            row[QStringLiteral("name")] = e.name;
            row[QStringLiteral("protocol")] = e.protocol;
            row[QStringLiteral("installed")] = e.installed;
            QString verification = QStringLiteral("NotInstalled");
            switch (e.verification) {
            case AgentVerification::Verified:
                verification = QStringLiteral("Verified");
                break;
            case AgentVerification::Unverified:
                verification = QStringLiteral("Unverified");
                break;
            case AgentVerification::NotInstalled:
                verification = QStringLiteral("NotInstalled");
                break;
            }
            row[QStringLiteral("verification")] = verification;
            rows.append(row);
        }
        agentSetup->setForeignAgents(rows);
    };
    QObject::connect(graph.agents, &AgentRepository::catalogRefreshed, graph.agentSetup,
                     pushForeignAgents);
    pushForeignAgents(); // seed from whatever the catalog already holds (offline-first / cached)
    // The shared interactive-auth flow view-model over the per-mode seam (GUI + TUI bind it).
    graph.authFlowController = new auth::AuthFlowController(graph.authFlow, owner);
    // [wave2:app-channels-liveness] B1: land a freshly-connected account in the Channels surface
    // without a manual refresh. On a successful interactive sign-in, refetch the transport
    // instances (the new account appears with its live status) and that account's ConvList (its
    // rooms populate). Lives once in the shared graph so GUI + TUI both benefit through their
    // existing instancesChanged/conversationsChanged reactivity. Daemon-backed mode only.
    if (graph.transportRepository != nullptr) {
        QObject::connect(graph.authFlowController, &auth::AuthFlowController::succeeded,
                         graph.transportRepository,
                         [transports = graph.transportRepository](
                             const QString& /*credentialRef*/, const QString& /*accountLabel*/,
                             const QString& transportInstance, const QString& /*boundProfile*/) {
                             transports->refreshInstances();
                             if (!transportInstance.isEmpty()) {
                                 transports->refreshConversations(transportInstance);
                             }
                         });
    }
    // Built last so it binds the resolved modelCatalog (daemon-backed or mock) for the inference
    // readiness gate (CON-7).
    graph.firstRun = new firstrun::FirstRunModel(graph.settings, graph.connection,
                                                 graph.modelCatalog, graph.profiles, graph.accounts,
                                                 graph.providerCatalog, graph.agentSetup, owner);
    // CON-16: the wizard's agent-type step is offered iff the node's foreign-agent catalog
    // reflected any foreign agent. Wired here (not inside FirstRunModel) because firstrun must stay
    // free of the daemon layer; the connect-ready storm above refreshes the catalog, so the offer
    // is usually known by the time the gate decides.
    QObject::connect(graph.agents, &AgentRepository::catalogRefreshed, graph.firstRun,
                     [firstRun = graph.firstRun, agents = graph.agents] {
                         firstRun->setAgentTypeOffered(!agents->entries().isEmpty());
                     });

    // Release-feed / auto-update surface. Inert unless the package job compiled in a
    // capability dial; binding the settings store arms poll scheduling + staging
    // hygiene and persists the ETag / dismissed-version / auto-check bookkeeping.
    graph.update = new update::UpdateManager(owner);
    graph.update->setSettings(graph.settings);

    // mirror A7 (M4): the ported session/fleet consumers bind `storeMirror`. The composition-time
    // fallback IS the legacy store — mock mode (null mirror until A8's seeder) and substrate-less
    // stacks keep rendering with zero per-consumer conditionals (§9 "mock keeps working"). The
    // daemon+substrate branch below overrides it with the MirrorSessionStore projection.
    graph.storeMirror = graph.store;

#ifdef DAEMON_APP_HAVE_MIRROR_SUBSTRATE
    // ---- mirror A5 (spec 09 wave M2): live-wire the ingestor beside the legacy paths ----
    // Dual-write discipline (§13 M0–M2): the SubscriptionManager + repositories keep writing their
    // caches; the mirror is fed in parallel and the M2 read lenses (chat/persons/contacts/
    // conversation) read it. The ingestor is the single writer (§5.1). DAEMON MODE ONLY at M2:
    // mock mode leaves mirrorService/outbox null so the chat surfaces fall back to the legacy
    // mock seams (§9 "mock keeps working"); A8's seeder feeds the mirror in mock and removes the
    // fallback.
    if (daemonConnection != nullptr) {
        auto* svc = new mirror::MirrorService(owner);
        // Persistent per-identity boot (§4.5): mirror-<id>.db opens under the LAST authenticated
        // identity (persisted in settings) so a cold offline boot renders that identity's
        // last-known state (E1). The path convention matches the cache store / local-db exactly:
        // SHA-256(userKey) hex, first 16, suffixed to the base filename, beside daemon_cache.db.
        const QString lastIdentity =
            graph.settings->value(QStringLiteral("mirror/lastIdentity")).toString();
        {
            const MirrorDbPaths paths(lastIdentity);
            (void)svc->open(paths); // a failed open degrades to in-memory and logs
        }
        graph.mirrorService = svc;

        // The precious local-<id>.db + the durable outbox (§6): the ConvSend + turn-prompt lanes.
        // Same per-identity namespace at boot (LocalDatabase hashes the key identically).
        graph.localDb = new mirror::LocalDatabase(QString(), owner);
        if (!lastIdentity.isEmpty()) {
            graph.localDb->setIdentityNamespace(lastIdentity);
        }
        graph.outbox = new mirror::Outbox(graph.localDb, owner);
        // Drain gate (§6.3): mutation lanes require connected+authenticated; the turn-prompt
        // dispatch lane is gated on session-idle by its own consumer, not the wire.
        graph.outbox->setGate(
            [nodeApi = graph.nodeApi](const QString& /*lane*/, mirror::VerbClass cls) {
                return !mirror::requiresConnected(cls) || nodeApi->isAuthenticated();
            });
        graph.outbox->load();
        // Provenance-landing seam (§5.1/§6.6, deferred to A8 by BR/A7): the single-writer commit
        // path doubles as the outbox's confirmation feed. Every provenance-stamped apply
        // (journal record carrying origin_op) lands the matching outbox op. Wired IN THE SAME
        // CHANGE as setProvenanceCapable below — otherwise an `accepted` op (ack seen, awaiting the
        // provenance echo) would linger forever (BR's explicit warning). Read-side coupling: the
        // outbox never writes mirrored state.
        QObject::connect(svc, &mirror::MirrorService::provenanceStamped, graph.outbox,
                         [outbox = graph.outbox](const QString& originOp) {
                             outbox->onProvenanceStamped(originOp);
                         });
        // Two-phase-landing local cleanup (§6.6): a crash between "delta persisted" and "op row
        // deleted" leaves an op whose op-id already appears in the persisted mirror journal. Boot
        // reconciliation scans that retention tail (A1's Persistence surface) and idempotently
        // deletes such landed ops. Runs once at boot after both durable stores are open.
        graph.outbox->reconcileLandedOps(svc->persistence().persistedOriginOps());

        // Identity switch (AuthOk): persist the key, then re-namespace BOTH durable stores —
        // reopen only on an actual CHANGE (a plain reconnect of the same user must not rebase the
        // live store under attached lenses; mid-run lens re-prime after a switch is the A6 seam).
        {
            auto currentIdentity = std::make_shared<QString>(lastIdentity);
            QObject::connect(
                graph.nodeApi, &NodeApiClient::authenticated, svc,
                [svc, settings = graph.settings, localDb = graph.localDb, outbox = graph.outbox,
                 currentIdentity](const DecodedPrincipalView& view) {
                    settings->setValue(QStringLiteral("mirror/lastIdentity"), view.userId);
                    if (*currentIdentity == view.userId) {
                        return;
                    }
                    *currentIdentity = view.userId;
                    const MirrorDbPaths paths(view.userId);
                    (void)svc->open(paths);
                    localDb->setIdentityNamespace(view.userId);
                    outbox->load();
                });
        }

        // Reconnect wiring (§5.6/§6.8): on AuthOk drive the ingestor's per-connection reconnect and
        // — from api/39 — auto-replay the outbox. The mirror's live-wire seam (A4 shipped the
        // ingestor decoupled; nobody drove onConnected/onDisconnected until now).
        {
            QObject::connect(
                graph.nodeApi, &NodeApiClient::authenticated, svc,
                [svc, nodeApi = graph.nodeApi, outbox = graph.outbox](const DecodedPrincipalView&) {
                    const int api = static_cast<int>(nodeApi->daemonApiVersion());
                    // hasRevFields is compile-time TRUE: the v39 codec is vendored (the
                    // rev/removed/ bootstrap wire shapes exist), so mode selection keys purely off
                    // the node's advertised api/<N>. FULL (api/39) ⇒ onConnected enqueues the
                    // Bootstrap probe → onBootstrap → since_rev delta reads; degraded (api/38) ⇒
                    // the staleness scan.
                    svc->ingestor().onConnected(api, /*hasRevFields=*/true);
                    // Provenance capability (§6.6): from api/39 the node stamps origin_op on the
                    // read path, so an ack moves the op to `accepted` awaiting the provenance echo
                    // (landed via the provenanceStamped→onProvenanceStamped wiring above). Against
                    // api/38 an ack is terminal success (no provenance contract). Same per-
                    // connection gate as autoReplayEnabled.
                    outbox->setProvenanceCapable(mirror::Outbox::autoReplayEnabled(api));
                    // Auto-replay (§6.8): from api/39 the node dedups on op_id, so an unattended
                    // reconnect drain is safe. api/38 holds the lanes for a manual tap.
                    if (mirror::Outbox::autoReplayEnabled(api)) {
                        outbox->drain();
                    }
                });
            // Connection lost (§5.6): the ingestor retains its feed cursor for the next resume.
            QObject::connect(graph.nodeApi, &NodeApiClient::disconnected, svc,
                             [svc] { svc->ingestor().onDisconnected(); });
        }

        {
            // Fulfil the ingestor's fetch jobs over the SAME NodeApiClient (shared pipeline).
            graph.mirrorExecutor =
                new DaemonFetchExecutor(graph.nodeApi, svc->ingestor(), svc->scheduler(), owner);
            svc->setFetchExecutor(graph.mirrorExecutor);

            // Routing (M3): the pin table lives on the mirror store. A node-acked routing mutation
            // (or the connect-ready re-list) fires RoutingRepository::routesRefreshed → re-fetch
            // the pin table into the mirror (single writer: the ingestor, not the repo, writes
            // rows). Per-account bindable rooms follow the transport instance list
            // (TransportRooms).
            if (graph.routingRepository != nullptr) {
                QObject::connect(graph.routingRepository, &RoutingRepository::routesRefreshed, svc,
                                 [svc] { svc->ingestor().refetchRouting(); });
            }
            if (graph.transportRepository != nullptr) {
                QObject::connect(graph.transportRepository,
                                 &TransportRepository::instancesRefreshed, svc,
                                 [svc, transports = graph.transportRepository] {
                                     for (const CachedTransportInstanceRow& row :
                                          transports->cachedInstances()) {
                                         svc->ingestor().refetchRooms(row.transport);
                                     }
                                 });
            }

            // mirror A7 (M4): the mirror-backed session store the ported consumers bind. Session
            // ROWS project the mirror `sessions` table; mutations + transcript reads delegate to
            // the legacy CachedSessionStore (one wire mutation path; transcript re-homing is the
            // wave's LAST sub-gate). The legacy `store` stays live for unported consumers
            // (dual-write; parity asserts guard the two row-sets until deletion).
            graph.storeMirror =
                new MirrorSessionStore(&svc->store(), &svc->ingestor(), graph.store, owner);

            // Feed the node event stream into the ingestor through the bridge. A per-session
            // Subscribe item is not an events page (decode fails) so it is ignored — the mirror
            // follows only the node-wide feed. Read-only, parallel to the SubscriptionManager
            // (dual-write): neither owns the other's cursor.
            QObject::connect(graph.nodeApi, &NodeApiClient::streamItem, svc,
                             [svc](quint64 /*id*/, const QByteArray& cbor) {
                                 DecodedEventsPage page;
                                 if (!NodeApiCodec::decodeEventsPage(cbor, &page)) {
                                     return;
                                 }
                                 for (const DecodedNodeEvent& e : page.events) {
                                     svc->ingest(translateNodeEvent(e));
                                 }
                             });

            // The outbox drains ConvSend to the wire on a manual tap (always) and unattended on an
            // api/39 reconnect (§6.8). The send carries the op_id (§10.3) so a retry/replay is
            // dedup-safe: the node returns the original result for a duplicate (principal, op_id),
            // and stamps `origin_op` provenance for the eventual confirm (§6.6). The ack/rejection
            // below advances the lane state.
            QObject::connect(
                graph.outbox, &mirror::Outbox::sendRequested, graph.nodeApi,
                [nodeApi = graph.nodeApi](const mirror::PendingOp& op) {
                    if (op.verb != QStringLiteral("ConvSend")) {
                        return; // other lanes are wired by their owners; turn-prompt is dispatch
                    }
                    const QJsonObject args = QJsonDocument::fromJson(op.payload).object();
                    const QByteArray req = NodeApiCodec::encodeConvSendRequest(
                        args.value(QStringLiteral("transport")).toString(),
                        args.value(QStringLiteral("conv")).toString(),
                        args.value(QStringLiteral("message")).toString(), op.opId);
                    nodeApi->sendRequest(req, QStringLiteral("outbox/") + op.opId);
                });
            QObject::connect(graph.nodeApi, &NodeApiClient::responseReady, graph.outbox,
                             [outbox = graph.outbox](const QString& corr, const QByteArray& cbor) {
                                 if (!corr.startsWith(QStringLiteral("outbox/"))) {
                                     return;
                                 }
                                 const QString opId = corr.mid(7);
                                 // Ok → accepted/landed; a node rejection (api-error) pauses the
                                 // lane (§6.5).
                                 if (NodeApiCodec::responseKind(cbor) == ApiResponseKind::Error) {
                                     DecodedApiError err;
                                     NodeApiCodec::decodeError(cbor, &err);
                                     outbox->onRejection(opId, err.kind, err.message);
                                 } else {
                                     outbox->onAck(opId);
                                 }
                             });
            QObject::connect(graph.nodeApi, &NodeApiClient::failed, graph.outbox,
                             [outbox = graph.outbox](const QString& corr, const QString& msg) {
                                 if (corr.startsWith(QStringLiteral("outbox/"))) {
                                     // A transport-level failure is transient — backoff, keep
                                     // order.
                                     outbox->onTransientFailure(corr.mid(7), msg);
                                 }
                             });
        }
    } else {
        // ---- mirror A8 (spec 09 §9, wave M5): the MOCK mirror — same store, same journal, same
        // lenses, different feeder. The scenario's Seeder is the single Writer (§5.1); the A5/A7
        // null-mirror fallbacks die here: Mirror/Outbox are LIVE in mock, so the chat surfaces
        // read the window lens and route sends through the chat-send lane + scripted outcomes. ----
        auto* svc = new mirror::MirrorService(owner);
        // Mock state is throwaway by design (deterministic per run; the scenario is the only seed
        // source): in-memory mirror, in-memory local db — nothing persisted, no migration surface.
        svc->openInMemory();
        graph.mirrorService = svc;
        graph.localDb = new mirror::LocalDatabase(QStringLiteral(":memory:"), owner);
        graph.outbox = new mirror::Outbox(graph.localDb, owner);
        // Drain gate (§6.3): mutation lanes require the (mock) connection to be ready — the same
        // predicate shape as the daemon branch, over the mock liveness state machine.
        graph.outbox->setGate(
            [conn = graph.connection](const QString& /*lane*/, mirror::VerbClass cls) {
                return !mirror::requiresConnected(cls) || conn->ready();
            });
        graph.outbox->load();
        // Provenance landing (§5.1/§6.6): identical wiring to the daemon branch — the scripted
        // ok-outcome's echo lands ops through the SAME seam. (No boot reconciliation: the
        // in-memory pair starts empty by construction.)
        QObject::connect(svc, &mirror::MirrorService::provenanceStamped, graph.outbox,
                         [outbox = graph.outbox](const QString& originOp) {
                             outbox->onProvenanceStamped(originOp);
                         });

        // The scenario driver: seeds the mirror (one batch), plays the timeline, answers verbs
        // from the script, and drives the ingestor's per-connection mode from the scenario's
        // api/<N> at the mock connection's ready/lost transitions (§5.6/§6.8).
        graph.mockHost = new MockScenarioHost(svc, graph.outbox, mockScenario, owner);
        svc->setFetchExecutor(graph.mockHost->fetchExecutor());
        {
            auto wasReady = std::make_shared<bool>(false);
            QObject::connect(graph.connection, &connection::IConnectionService::stateChanged,
                             graph.mockHost,
                             [conn = graph.connection, host = graph.mockHost, wasReady] {
                                 const bool nowReady = conn->ready();
                                 if (nowReady && !*wasReady) {
                                     host->onConnectionReady();
                                 } else if (!nowReady && *wasReady) {
                                     host->onConnectionLost();
                                 }
                                 *wasReady = nowReady;
                             });
        }

        // mirror A8 (M5): mock storeMirror is the REAL MirrorSessionStore over the seeded mirror —
        // the A7 composition-time aliasing is deleted; the 6→1 projection now serves BOTH modes.
        // content() + mutations keep DELEGATING to the legacy in-memory store (A7T re-homes that
        // seam in parallel; the scenario derives the mirror ids from the same bundle so the
        // delegated content() join holds).
        graph.storeMirror =
            new MirrorSessionStore(&svc->store(), &svc->ingestor(), graph.store, owner);
    }
#endif

    // The ops-hub session roster projects storeMirror in BOTH modes (M4 daemon; M5 mock — the
    // seeded MirrorSessionStore, or the legacy alias on substrate-less stacks): the 6→1 session
    // read everywhere, MockSessionRoster deleted. The repository rides along for the operator
    // steer/startTurn/interrupt ops (F4; inert without a connection) and the archived-scope
    // refetch (F6). The dashboard observes the FINAL seam pointers; mock keeps the MockDashboard
    // presentation (seeded activity feed) over the same interfaces.
    graph.roster = new fleet::DaemonSessionRoster(graph.storeMirror, graph.sessions, owner);
    graph.dashboard =
        daemonConnection != nullptr
            ? static_cast<fleet::IDashboard*>(new fleet::DaemonDashboard(
                  graph.roster, graph.fleetTree, graph.approvals, graph.connection, owner))
            : static_cast<fleet::IDashboard*>(
                  new fleet::MockDashboard(graph.roster, graph.fleetTree, graph.approvals, owner));
    return graph;
}

} // namespace daemonapp::daemon
