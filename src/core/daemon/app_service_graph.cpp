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
#include "daemon/daemon_checkpoint_timeline.h"
#include "daemon/daemon_connection_service.h"
#include "daemon/daemon_contacts_service.h"
#include "daemon/daemon_daemonnet.h"
#include "daemon/daemon_dashboard.h"
#include "daemon/daemon_fleet_tree.h"
#include "daemon/daemon_fs_service.h"
#include "daemon/daemon_model_catalog.h"
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
#include "daemonnet/mock_daemonnet.h"
#include "feedback/mock_feedback.h"
#include "firstrun/first_run_model.h"
#include "fleet/mock_approvals_inbox.h"
#include "fleet/mock_dashboard.h"
#include "fleet/mock_fleet_tree.h"
#include "fleet/mock_session_roster.h"
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
#include "tools/mock_tool_inventory.h" // [wave2:app-approvals-safety] D2
#include "transports/mock_contacts_service.h"
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

namespace daemonapp::daemon {

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
    // The unified mock DaemonNet (the single source the fleet/roster mocks now project from, plus
    // the future lenses + patch-bay); construct it before the surfaces that derive from it.
    //
    // Its transportsTree() is also the sidebar's INTEGRATIONS source, and that demo tree is
    // truthful only for the mock UI. Daemon mode has no live DaemonNet projection yet — the
    // planned replacement of this seam is a DaemonDaemonNet projected from TransportRepository
    // (TransportAdapters/Instances + ConvList) — so until it lands, Daemon mode seeds the tree
    // empty (no Integrations section) rather than passing mock channels off as live ones.
    // DAEMON_APP_MOCK_INTEGRATIONS opts back into the demo tree for development, mirroring the
    // DAEMON_APP_EDITOR_DEMO gate.
    const bool demoTransports =
        mode != ServiceMode::Daemon || qEnvironmentVariableIsSet("DAEMON_APP_MOCK_INTEGRATIONS");
    graph.daemonNet = new daemonnet::MockDaemonNet(
        demoTransports ? daemonnet::TransportsSeed::Demo : daemonnet::TransportsSeed::Empty, owner);
    graph.roster = new fleet::MockSessionRoster(graph.daemonNet, owner);
    graph.fleetTree = new fleet::MockFleetTree(graph.daemonNet, owner);
    graph.approvals = new fleet::MockApprovalsInbox(owner);
    // [wave2:app-approvals-safety] D2: tool inventory starts as the canned mock; the daemon branch
    // below REPLACES it with the repo-backed DaemonToolInventory (ToolList).
    graph.tools = new tools::MockToolInventory(owner);
    graph.dashboard =
        new fleet::MockDashboard(graph.roster, graph.fleetTree, graph.approvals, owner);
    // Cron has NO node wire op yet (the daemon-api codec subset carries none), so in Daemon mode
    // it must render EMPTY rather than pass its illustrative demo rows off as node-backed data
    // (render honesty; see seam_migration.h). Mock mode keeps the demo seed. (Routing left this
    // bucket at wire v28: the origin->session pin table is served by DaemonDaemonNet below; the
    // legacy intent->model IRoutingStore surface is retired.)
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
        // Daemon-backed, offline-first session roster + dashboard (replaces the mock pair). The
        // roster projects the (offline-first) CachedSessionStore; the dashboard derives its
        // counters from the FINAL seam pointers (DaemonFleetTree + DaemonApprovalsInbox) and its
        // health from the connection. Delete the mock dashboard FIRST (it observes the mock roster
        // + the now-swapped fleet tree), then the mock roster, then build the daemon pair over the
        // final pointers — otherwise the mock dashboard keeps a dangling fleet-tree pointer (a
        // confirmed use-after-free / SIGSEGV in MockDashboard::runningAgents()).
        delete graph.dashboard;
        delete graph.roster;
        // The real routing manager (B6/ROU, wire v28): RoutingRepository speaks
        // RoutingListChats/BindChat/UnbindChat/Set + TransportRooms; DaemonDaemonNet projects it
        // (plus the transport instances) through the IDaemonNet seam the RoutingPage/RouteDialog
        // already bind — replacing the always-demo MockDaemonNet. Swapped here, after every mock
        // consumer of the old pointer (roster/fleet/dashboard) is gone.
        // DAEMON_APP_MOCK_INTEGRATIONS keeps the demo MockDaemonNet for development (the same
        // escape hatch the Integrations tree seed honors above).
        graph.routingRepository = new RoutingRepository(graph.nodeApi, graph.cache, owner);
        // [waveB:app-v30] D2: the feed's MembershipChanged (self removal) re-lists the routing pins
        // (the node reconciled them); wired now that the routing repo exists.
        if (graph.subscriptions != nullptr) {
            graph.subscriptions->setRoutingRepository(graph.routingRepository);
        }
        if (!demoTransports) {
            delete graph.daemonNet;
            graph.daemonNet =
                new DaemonDaemonNet(graph.routingRepository, graph.transportRepository, graph.store,
                                    graph.sessions, owner);
        }
        // The repository rides along for the operator steer/startTurn/interrupt ops (F4) and the
        // archived-scope refetch (F6).
        graph.roster = new fleet::DaemonSessionRoster(graph.store, graph.sessions, owner);
        graph.dashboard = new fleet::DaemonDashboard(graph.roster, graph.fleetTree, graph.approvals,
                                                     graph.connection, owner);
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
             toolRepo = graph.toolRepository, // [wave2:app-approvals-safety] D2
             feedback = daemonFeedback,       // wire v32: seed telemetry consent
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
                    // Routing: the origin->session pin table (B6/ROU). Per-account rooms follow
                    // the instances refresh (DaemonDaemonNet chains them).
                    routing->refreshChats();
                    // Foreign engines: the catalog for the engine picker + Agents settings.
                    agents->refreshCatalog();
                    // Interactive-auth family discovery (the AuthFlowSheet's provider list).
                    authRepo->refreshProviders();
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
               "routing(RoutingListChats/BindChat/UnbindChat+TransportRooms via DaemonDaemonNet); "
               "still mock: daemonConfig, memory; node-blocked (NO wire op yet - rendered EMPTY, "
               "not mock demo data): cron, daemonNet delivery/handover panel.";
    } else {
        // Mock mode: a non-persisted in-memory store re-seeded fresh from the unified DaemonNet
        // each run (deterministic, always matches the current seed - no stale-db drift). The
        // sidebar/list/ transcript and the Fleet/Sessions pages now all reflect the one DaemonNet
        // source.
        graph.store = new persistence::InMemorySessionStore(graph.daemonNet, owner);
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
    graph.firstRun =
        new firstrun::FirstRunModel(graph.settings, graph.connection, graph.modelCatalog,
                                    graph.profiles, graph.accounts, graph.providerCatalog, owner);
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
    return graph;
}

} // namespace daemonapp::daemon
