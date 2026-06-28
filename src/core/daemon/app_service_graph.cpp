#include "daemon/app_service_graph.h"

#include "accounts/mock_accounts_service.h"
#include "automation/mock_cron_store.h"
#include "automation/mock_routing_store.h"
#include "config/mock_daemon_config.h"
#include "connection/iconnection_service.h"
#include "connection/mock_connection_service.h"
#include "daemon/cached_session_store.h"
#include "daemon/daemon_accounts_service.h"
#include "daemon/daemon_approvals_inbox.h"
#include "daemon/daemon_cache_store.h"
#include "daemon/daemon_connection_service.h"
#include "daemon/daemon_dashboard.h"
#include "daemon/daemon_fleet_tree.h"
#include "daemon/daemon_fs_service.h"
#include "daemon/daemon_model_catalog.h"
#include "daemon/daemon_profile_store.h"
#include "daemon/daemon_session_roster.h"
#include "daemon/daemon_session_settings.h"
#include "daemon/daemon_transport.h"
#include "daemon/node_api_client.h"
#include "daemon/repositories.h"
#include "daemon/subscription_manager.h"
#include "daemonnet/mock_daemonnet.h"
#include "firstrun/first_run_model.h"
#include "fleet/mock_approvals_inbox.h"
#include "fleet/mock_dashboard.h"
#include "fleet/mock_fleet_tree.h"
#include "fleet/mock_session_roster.h"
#include "fs/local_disk_fs_service.h"
#include "memory/mock_memory_service.h"
#include "models/mock_model_catalog.h"
#include "nav/nav_controller.h"
#include "persistence/in_memory_session_store.h"
#include "profiles/mock_profile_store.h"
#include "session/mock_checkpoint_timeline.h"
#include "session/mock_session_settings.h"
#include "settings/qt_settings_store.h"
#include "transports/mock_presence_service.h"
#include "transports/mock_transport_registry.h"

#include <memory>
#include <QByteArray>
#include <QDebug>
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
    graph.fs = new fs::LocalDiskFsService(
        graph.daemonConfig->value(QStringLiteral("workspace/root")).toString(), QString(), owner);
    graph.memory = new memory::MockMemoryService(owner);
    graph.nav = new nav::NavController(owner);
    // firstRun + accounts + modelCatalog + profiles are constructed per-mode below (after the
    // repositories the daemon-backed services need); firstRun binds the resulting modelCatalog for
    // inference readiness.
    // The unified mock DaemonNet (the single source the fleet/roster mocks now project from, plus
    // the future lenses + patch-bay); construct it before the surfaces that derive from it.
    graph.daemonNet = new daemonnet::MockDaemonNet(owner);
    graph.roster = new fleet::MockSessionRoster(graph.daemonNet, owner);
    graph.fleetTree = new fleet::MockFleetTree(graph.daemonNet, owner);
    graph.approvals = new fleet::MockApprovalsInbox(owner);
    graph.dashboard =
        new fleet::MockDashboard(graph.roster, graph.fleetTree, graph.approvals, owner);
    graph.routing = new automation::MockRoutingStore(owner);
    graph.cron = new automation::MockCronStore(owner);
    // Transport-adapter seams (daemon-transport-adapter-spec.md): inert mocks until a daemon
    // adapter decodes transport_adapters / transport_instances. The registry advertises the
    // existing adapter families for the "Add channel" picker; presence reports offline/unknown.
    graph.transportRegistry = new transports::MockTransportRegistry(owner);
    graph.presence = new transports::MockPresenceService(owner);
    // sessionSettings is constructed per-mode below (the daemon variant needs nodeApi to send
    // SetSessionMode); checkpoints is mock in both.
    graph.checkpoints = new session::MockCheckpointTimeline(owner);
    graph.cache = new DaemonCacheStore(QString(), owner);
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
    graph.approvalRepository = new ApprovalRepository(graph.nodeApi, graph.cache, owner);
    graph.checkpointRepository = new CheckpointRepository(graph.nodeApi, graph.cache, owner);
    graph.credentialRepository = new CredentialRepository(graph.nodeApi, graph.cache, owner);

    if (daemonConnection != nullptr) {
        // Daemon mode reads sessions through the cache projection rather than the local sqlite
        // store, and kicks a SessionsQuery once the Health probe reports the daemon is ready so
        // the cache (and therefore the UI) populates end-to-end.
        graph.store = new CachedSessionStore(graph.cache, graph.sessions, owner);
        // Provider credentials + model discovery are daemon-backed in this mode (CON-4 / CON-6).
        graph.accounts = new accounts::DaemonAccountsService(graph.credentialRepository,
                                                             graph.profileRepository, owner);
        graph.modelCatalog = new models::DaemonModelCatalog(graph.models, owner);
        graph.profiles = new profiles::DaemonProfileStore(graph.profileRepository, owner);
        // Per-session approval mode (CHA-4): the setter sends SetSessionMode to the node.
        graph.sessionSettings = new DaemonSessionSettings(graph.nodeApi, owner);
        // PRO-11: pending-approvals inbox backed by the ApprovalRepository (ApprovalsPending poll
        // on ready + ApprovalDecide), replacing the mock fleet inbox.
        graph.approvals = new DaemonApprovalsInbox(graph.approvalRepository, owner);
        // Daemon-backed, offline-first fleet/subagent tree (PRO-9/10): replace the mock fleet tree
        // with one projected from the cached Tree query. The mock built above is parented to
        // `owner`; drop it for the daemon one. Built before the SubscriptionManager so the feed can
        // route FleetChanged -> a live Tree refetch.
        graph.fleetRepository = new FleetRepository(graph.nodeApi, graph.cache, owner);
        delete graph.fleetTree;
        graph.fleetTree = new fleet::DaemonFleetTree(graph.fleetRepository, owner);
        // L3 node-wide event feed (daemon-sync-protocol §5): one EventsSince stream that routes
        // out-of-focus changes (roster/meta -> debounced roster refetch, fleet -> debounced Tree
        // refetch, approvals -> badge, downloads -> models, session-advanced -> focused-engine
        // nudge, resync -> baseline) so the client stops polling and stops full-refetching on every
        // change.
        graph.subscriptions =
            new SubscriptionManager(graph.nodeApi, graph.sessions, graph.approvalRepository,
                                    graph.models, graph.fleetRepository, graph.cache, owner);
        // Daemon-backed filesystem: replace the dev local-disk seam over the NodeApi fs_* ops - the
        // only path that reaches a remote/embedded host's workspace. The common LocalDiskFsService
        // built above is parented to `owner`; drop it for the daemon one.
        delete graph.fs;
        graph.fs = new fs::DaemonFsService(graph.nodeApi, graph.cache, owner);
        // Daemon-backed, offline-first roster + dashboard: project the live cache/fleet/approvals
        // seams (the SessionsPage roster + the dashboard counters were the last DaemonNet-backed
        // surfaces still on the mock). The mocks built above are parented to `owner`; drop them.
        // Order: roster first, then the dashboard which derives from roster + fleet + approvals.
        delete graph.roster;
        graph.roster = new fleet::DaemonSessionRoster(graph.store, owner);
        delete graph.dashboard;
        graph.dashboard = new fleet::DaemonDashboard(graph.roster, graph.fleetTree, graph.approvals,
                                                     graph.connection, owner);
        // On connect-ready, populate sessions + profiles + credentials + models so the onboarding
        // provider/model step and the shell reflect the daemon end-to-end. Fire only on the
        // transition INTO ready: stateChanged also fires for statusMessage churn (e.g. the
        // "Reconnecting..." status) and the heartbeat re-affirming ready, which must not trigger a
        // re-refresh storm; a genuine reconnect (connecting -> ready) does re-sync once, as
        // desired.
        auto wasReady = std::make_shared<bool>(false);
        QObject::connect(
            graph.connection, &connection::IConnectionService::stateChanged, graph.sessions,
            [conn = graph.connection, sessions = graph.sessions, profiles = graph.profileRepository,
             credentials = graph.credentialRepository, models = graph.models,
             approvals = graph.approvalRepository, subscriptions = graph.subscriptions,
             fs = graph.fs, fleet = graph.fleetRepository, wasReady] {
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
                    approvals->refreshPending();
                    fs->listRoots();      // populate the daemon-backed file roots
                    fleet->refreshTree(); // PRO-9: baseline the subagent tree
                    subscriptions->start();
                } else if (!nowReady && *wasReady) {
                    subscriptions->stop();
                }
                *wasReady = nowReady;
            });
        // Daemon mode is partially live: be explicit so testers/operators don't read it as a fully
        // daemon-backed app. Live now: connection (Health), sessions (SessionsQuery), accounts
        // (CredentialSet/List), modelCatalog (Models/ModelCurrent). Still mock below.
        qInfo().noquote() << "AppServiceGraph: ServiceMode::Daemon - live seams: connection, "
                             "sessions(cache), accounts(credentials), modelCatalog(models), "
                             "profiles, approvals(ApprovalsPending/Decide), fs(fs_*), "
                             "fleetTree(Tree), roster(cache), dashboard(derived), "
                             "sessionSettings(SetSessionMode); still mock: daemonConfig, "
                             "memory, daemonNet, routing/cron, "
                             "transports/presence, checkpoints.";
    } else {
        // Mock mode: a non-persisted in-memory store re-seeded fresh from the unified DaemonNet
        // each run (deterministic, always matches the current seed - no stale-db drift). The
        // sidebar/list/ transcript and the Fleet/Sessions pages now all reflect the one DaemonNet
        // source.
        graph.store = new persistence::InMemorySessionStore(graph.daemonNet, owner);
        graph.modelCatalog = new models::MockModelCatalog(owner);
        graph.accounts = new accounts::MockAccountsService(owner);
        graph.profiles = new profiles::MockProfileStore(owner);
        graph.sessionSettings = new session::MockSessionSettings(owner);
    }
    // Built last so it binds the resolved modelCatalog (daemon-backed or mock) for the inference
    // readiness gate (CON-7).
    graph.firstRun =
        new firstrun::FirstRunModel(graph.settings, graph.connection, graph.modelCatalog, owner);
    return graph;
}

} // namespace daemonapp::daemon
