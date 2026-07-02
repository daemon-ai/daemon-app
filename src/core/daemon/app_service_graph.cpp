// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

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
#include "daemon/daemon_fleet_tree.h"
#include "daemon/daemon_fs_service.h"
#include "daemon/daemon_model_catalog.h"
#include "daemon/daemon_presence_service.h"
#include "daemon/daemon_profile_store.h"
#include "daemon/daemon_provider_catalog.h"
#include "daemon/daemon_session_settings.h"
#include "daemon/daemon_transport.h"
#include "daemon/daemon_transport_registry.h"
#include "daemon/node_api_client.h"
#include "daemon/principal_model.h"
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
#include "models/mock_provider_catalog.h"
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
        // Node-driven provider/model discovery (ProviderCatalog/ProviderModels), sharing the model
        // catalog for the local [installed]+Discover compose.
        graph.providerCatalog =
            new DaemonProviderCatalog(graph.providerRepository, graph.modelCatalog, owner);
        graph.profiles = new profiles::DaemonProfileStore(graph.profileRepository, owner);
        // Per-session approval mode (CHA-4): the setter sends SetSessionMode to the node.
        graph.sessionSettings = new DaemonSessionSettings(graph.nodeApi, owner);
        // PRO-11: pending-approvals inbox backed by the ApprovalRepository (ApprovalsPending poll
        // on ready + ApprovalDecide), replacing the mock fleet inbox.
        graph.approvals = new DaemonApprovalsInbox(graph.approvalRepository, owner);
        // Daemon-backed, offline-first fleet/subagent tree (PRO-9/10): replace the mock fleet tree
        // with one projected from the cached Tree query. The mock built above is parented to
        // `owner`; drop it for the daemon one. Built before the subscription feed so the feed can
        // re-query the tree on a roster delta (a fleet unit IS a durable session - FIX 2).
        graph.fleetRepository = new FleetRepository(graph.nodeApi, graph.cache, owner);
        delete graph.fleetTree;
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
        delete graph.transportRegistry;
        graph.transportRegistry =
            new transports::DaemonTransportRegistry(graph.transportRepository, owner);
        delete graph.presence;
        graph.presence = new transports::DaemonPresenceService(graph.transportRepository, owner);
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
             transports = graph.transportRepository, wasReady] {
                const bool nowReady = conn->ready();
                if (nowReady && !*wasReady) {
                    // Initial baseline once per (re)connect; the EventsSince feed then keeps the
                    // surfaces fresh incrementally instead of re-running this storm on every
                    // change.
                    sessions->refreshSessions();
                    profiles->refreshProfiles();
                    // #region agent log
                    {
                        QFile dbg(
                            QStringLiteral("/home/j/experiments/daemon/.cursor/debug-96b7ad.log"));
                        if (dbg.open(QIODevice::Append | QIODevice::Text))
                            dbg.write(
                                QStringLiteral(
                                    "{\"sessionId\":\"96b7ad\",\"hypothesisId\":\"PROFILE-"
                                    "REFLECT\","
                                    "\"location\":\"app_service_graph.cpp:connect-ready\","
                                    "\"message\":\"connect-ready refreshProfiles issued\",\"data\":"
                                    "{},\"timestamp\":%1}\n")
                                    .arg(QDateTime::currentMSecsSinceEpoch())
                                    .toUtf8());
                    }
                    // #endregion
                    credentials->refreshList();
                    models->refreshModels();
                    models->refreshCurrent();
                    providers->refreshProviders(); // node-driven provider/model discovery
                    approvals->refreshPending();
                    fs->listRoots(); // populate the daemon-backed file roots
                    fleet->refreshTree(QStringLiteral("baseline")); // PRO-9: baseline the tree
                    // Channels: the adapter picker + configured accounts/status dots (EIO-1/3/9).
                    transports->refreshAdapters();
                    transports->refreshInstances();
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
                             "profiles, approvals(ApprovalsPending/Decide), "
                             "sessionSettings(SetSessionMode), fs(fs_*), fleetTree(Tree), "
                             "transports/presence(TransportAdapters/Instances+ConvList); "
                             "still mock: daemonConfig, memory, daemonNet (Integrations seeded "
                             "empty unless DAEMON_APP_MOCK_INTEGRATIONS), roster/dashboard, "
                             "routing/cron, checkpoints.";
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
    }
    // Built last so it binds the resolved modelCatalog (daemon-backed or mock) for the inference
    // readiness gate (CON-7).
    graph.firstRun =
        new firstrun::FirstRunModel(graph.settings, graph.connection, graph.modelCatalog,
                                    graph.profiles, graph.accounts, graph.providerCatalog, owner);
    return graph;
}

} // namespace daemonapp::daemon
