#include "daemon/app_service_graph.h"

#include "accounts/mock_accounts_service.h"
#include "automation/mock_cron_store.h"
#include "automation/mock_routing_store.h"
#include "connection/iconnection_service.h"
#include "daemon/cached_session_store.h"
#include "daemon/daemon_cache_store.h"
#include "config/mock_daemon_config.h"
#include "daemon/daemon_connection_service.h"
#include "daemon/daemon_transport.h"
#include "daemon/node_api_client.h"
#include "daemon/repositories.h"
#include "connection/mock_connection_service.h"
#include "firstrun/first_run_model.h"
#include "fleet/mock_approvals_inbox.h"
#include "fleet/mock_dashboard.h"
#include "fleet/mock_fleet_tree.h"
#include "fleet/mock_session_roster.h"
#include "fs/local_disk_fs_service.h"
#include "memory/mock_memory_service.h"
#include "models/mock_model_catalog.h"
#include "nav/nav_controller.h"
#include "persistence/sqlite_session_store.h"
#include "profiles/mock_profile_store.h"
#include "session/mock_checkpoint_timeline.h"
#include "session/mock_session_settings.h"
#include "settings/qt_settings_store.h"

#include <QByteArray>
#include <QDebug>
#include <QString>
#include <QtGlobal>

namespace daemonapp::daemon {

ServiceMode serviceModeFromEnvironment(ServiceMode fallback)
{
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

AppServiceGraph createAppServiceGraph(ServiceMode mode, QObject* owner)
{
    AppServiceGraph graph;
    graph.settings = new settings::QtSettingsStore(owner);
    graph.connection = mode == ServiceMode::Daemon
        ? static_cast<connection::IConnectionService*>(new DaemonConnectionService(owner))
        : static_cast<connection::IConnectionService*>(new connection::MockConnectionService(owner));
    graph.daemonConfig = new config::MockDaemonConfig(owner);
    graph.fs = new fs::LocalDiskFsService(
        graph.daemonConfig->value(QStringLiteral("workspace/root")).toString(), QString(), owner);
    graph.memory = new memory::MockMemoryService(owner);
    graph.nav = new nav::NavController(owner);
    graph.firstRun = new firstrun::FirstRunModel(graph.settings, graph.connection, owner);
    graph.modelCatalog = new models::MockModelCatalog(owner);
    graph.accounts = new accounts::MockAccountsService(owner);
    graph.profiles = new profiles::MockProfileStore(owner);
    graph.roster = new fleet::MockSessionRoster(owner);
    graph.fleetTree = new fleet::MockFleetTree(owner);
    graph.approvals = new fleet::MockApprovalsInbox(owner);
    graph.dashboard = new fleet::MockDashboard(graph.roster, graph.fleetTree, graph.approvals, owner);
    graph.routing = new automation::MockRoutingStore(owner);
    graph.cron = new automation::MockCronStore(owner);
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

    if (daemonConnection != nullptr) {
        // Daemon mode reads sessions through the cache projection rather than the local sqlite
        // store, and kicks a SessionsQuery once the Health probe reports the daemon is ready so
        // the cache (and therefore the UI) populates end-to-end.
        graph.store = new CachedSessionStore(graph.cache, graph.sessions, owner);
        QObject::connect(graph.connection, &connection::IConnectionService::stateChanged,
                         graph.sessions, [conn = graph.connection, sessions = graph.sessions] {
                             if (conn->ready()) {
                                 sessions->refreshSessions();
                             }
                         });
        // Daemon mode is only partially live: be explicit so testers/operators don't read it as a
        // fully daemon-backed app. Live now: connection (real Health probe) and sessions (NodeApi
        // SessionsQuery projected through DaemonCacheStore). Everything else is still mock-backed
        // until its own daemon adapter lands.
        qInfo().noquote() << "AppServiceGraph: ServiceMode::Daemon - live seams: connection, "
                             "sessions(cache); still mock: fs, daemonConfig, memory, modelCatalog, "
                             "accounts, roster/fleetTree/approvals/dashboard, routing/cron, "
                             "sessionSettings, checkpoints.";
    } else {
        graph.store = new persistence::SqliteSessionStore(QString(), owner);
    }
    return graph;
}

} // namespace daemonapp::daemon
