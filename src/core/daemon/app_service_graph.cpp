#include "daemon/app_service_graph.h"

#include "accounts/mock_accounts_service.h"
#include "automation/mock_cron_store.h"
#include "automation/mock_routing_store.h"
#include "config/mock_daemon_config.h"
#include "connection/iconnection_service.h"
#include "connection/mock_connection_service.h"
#include "daemon/cached_session_store.h"
#include "daemon/daemon_accounts_service.h"
#include "daemon/daemon_cache_store.h"
#include "daemon/daemon_connection_service.h"
#include "daemon/daemon_model_catalog.h"
#include "daemon/daemon_transport.h"
#include "daemon/node_api_client.h"
#include "daemon/repositories.h"
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
    // firstRun + accounts + modelCatalog are constructed per-mode below (after the repositories the
    // daemon-backed services need); firstRun binds the resulting modelCatalog for inference
    // readiness.
    graph.profiles = new profiles::MockProfileStore(owner);
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
    graph.sessionSettings = new session::MockSessionSettings(owner);
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
    graph.files = new FsRepository(graph.nodeApi, graph.cache, owner);
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
        // On connect-ready, populate sessions + profiles + credentials + models so the onboarding
        // provider/model step and the shell reflect the daemon end-to-end.
        QObject::connect(
            graph.connection, &connection::IConnectionService::stateChanged, graph.sessions,
            [conn = graph.connection, sessions = graph.sessions, profiles = graph.profileRepository,
             credentials = graph.credentialRepository, models = graph.models] {
                if (conn->ready()) {
                    sessions->refreshSessions();
                    profiles->refreshProfiles();
                    credentials->refreshList();
                    models->refreshModels();
                    models->refreshCurrent();
                }
            });
        // Daemon mode is partially live: be explicit so testers/operators don't read it as a fully
        // daemon-backed app. Live now: connection (Health), sessions (SessionsQuery), accounts
        // (CredentialSet/List), modelCatalog (Models/ModelCurrent). Still mock below.
        qInfo().noquote() << "AppServiceGraph: ServiceMode::Daemon - live seams: connection, "
                             "sessions(cache), accounts(credentials), modelCatalog(models); still "
                             "mock: fs, daemonConfig, memory, daemonNet, "
                             "roster/fleetTree/approvals/dashboard, routing/cron, "
                             "transports/presence, sessionSettings, checkpoints.";
    } else {
        // Mock mode: a non-persisted in-memory store re-seeded fresh from the unified DaemonNet
        // each run (deterministic, always matches the current seed - no stale-db drift). The
        // sidebar/list/ transcript and the Fleet/Sessions pages now all reflect the one DaemonNet
        // source.
        graph.store = new persistence::InMemorySessionStore(graph.daemonNet, owner);
        graph.modelCatalog = new models::MockModelCatalog(owner);
        graph.accounts = new accounts::MockAccountsService(owner);
    }
    // Built last so it binds the resolved modelCatalog (daemon-backed or mock) for the inference
    // readiness gate (CON-7).
    graph.firstRun =
        new firstrun::FirstRunModel(graph.settings, graph.connection, graph.modelCatalog, owner);
    return graph;
}

} // namespace daemonapp::daemon
