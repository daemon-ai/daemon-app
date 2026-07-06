// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>

namespace accounts {
class IAccountsService;
}
namespace auth {
class AuthFlowController;
class IAuthFlowService;
} // namespace auth
namespace automation {
class ICronStore;
class IRoutingStore;
} // namespace automation
namespace config {
class IDaemonConfig;
}
namespace connection {
class IConnectionService;
}
namespace daemonnet {
class IDaemonNet;
}
namespace firstrun {
class FirstRunModel;
}
namespace fleet {
class IApprovalsInbox;
class IDashboard;
class IFleetTree;
class ISessionRoster;
} // namespace fleet
namespace fs {
class IFsService;
}
namespace memory {
class IMemoryService;
}
namespace models {
class IModelCatalog;
class IProviderCatalog;
} // namespace models
namespace nav {
class NavController;
}
namespace persistence {
class ISessionStore;
}
namespace profiles {
class IProfileStore;
}
namespace session {
class ICheckpointTimeline;
class ISessionSettings;
} // namespace session
namespace settings {
class ISettingsStore;
}
namespace transports {
class IPresenceService;
class ITransportRegistry;
} // namespace transports
namespace update {
class UpdateManager;
}

namespace daemonapp::daemon {

class AcpRepository;
class ApprovalRepository;
class AuthRepository;
class FleetRepository;
class TransportRepository;
class CheckpointRepository;
class CredentialRepository;
class DaemonCacheStore;
class ModelRepository;
class ProviderRepository;
class NodeApiClient;
class PrincipalModel;
class ProfileRepository;
class SessionRepository;
class SubscriptionManager;

enum class ServiceMode {
    Mock,
    Daemon,
};

// The shared GUI/TUI service bundle.
//
// ServiceMode::Daemon is only partially live today: `connection` (real Health probe) and `store`
// (sessions projected from DaemonCacheStore via NodeApi SessionsQuery) are daemon-backed, while
// `fs`, `daemonConfig`, `memory`, `modelCatalog`, `accounts`, the fleet group
// (`roster`/`fleetTree`/`approvals`/`dashboard`), `routing`/`cron`, `sessionSettings`, and
// `checkpoints` remain mock-backed until each grows its own daemon adapter. createAppServiceGraph()
// logs this at startup in daemon mode so the partial state is not mistaken for fully live.
struct AppServiceGraph {
    persistence::ISessionStore* store = nullptr;
    settings::ISettingsStore* settings = nullptr;
    connection::IConnectionService* connection = nullptr;
    config::IDaemonConfig* daemonConfig = nullptr;
    fs::IFsService* fs = nullptr;
    memory::IMemoryService* memory = nullptr;
    nav::NavController* nav = nullptr;
    firstrun::FirstRunModel* firstRun = nullptr;
    models::IModelCatalog* modelCatalog = nullptr;
    models::IProviderCatalog* providerCatalog = nullptr;
    accounts::IAccountsService* accounts = nullptr;
    profiles::IProfileStore* profiles = nullptr;
    fleet::ISessionRoster* roster = nullptr;
    fleet::IFleetTree* fleetTree = nullptr;
    fleet::IApprovalsInbox* approvals = nullptr;
    fleet::IDashboard* dashboard = nullptr;
    automation::IRoutingStore* routing = nullptr;
    automation::ICronStore* cron = nullptr;
    transports::ITransportRegistry* transportRegistry = nullptr;
    transports::IPresenceService* presence = nullptr;
    daemonnet::IDaemonNet* daemonNet = nullptr;
    session::ISessionSettings* sessionSettings = nullptr;
    session::ICheckpointTimeline* checkpoints = nullptr;
    // Release-feed / auto-update surface (packaging/UPDATES.md). Inert unless the
    // package job compiled in a capability dial; both front ends bind it.
    update::UpdateManager* update = nullptr;
    DaemonCacheStore* cache = nullptr;
    NodeApiClient* nodeApi = nullptr;
    // The authenticated principal (WhoAmI) for advisory capability gating. Always constructed
    // (unauthenticated by default); populated from AuthOk in Daemon mode.
    PrincipalModel* principal = nullptr;
    SessionRepository* sessions = nullptr;
    ProfileRepository* profileRepository = nullptr;
    ModelRepository* models = nullptr;
    ProviderRepository* providerRepository = nullptr;
    CredentialRepository* credentialRepository = nullptr;
    ApprovalRepository* approvalRepository = nullptr;
    CheckpointRepository* checkpointRepository = nullptr;
    FleetRepository* fleetRepository = nullptr;         // PRO-9/10; non-null only in Daemon mode
    TransportRepository* transportRepository = nullptr; // Channels (EIO-1/3/8/9); Daemon mode only
    // The ACP agent catalog (foreign engines; wire v23): backs the new-agent dialog's engine
    // picker. Constructed with the other repositories (inert without a connection).
    AcpRepository* acp = nullptr;
    // Interactive auth (app-wizard-auth stream): the AuthBegin/AuthComplete wire seam (Daemon
    // mode only), the seam interface (daemon adapter or mock), and the shared flow view-model
    // both front ends bind (the GUI AuthFlowSheet / TUI auth panel).
    AuthRepository* authRepository = nullptr;
    auth::IAuthFlowService* authFlow = nullptr;
    auth::AuthFlowController* authFlowController = nullptr;
    // The node-wide event feed consumer (L3): owns the single EventsSince stream + routes
    // notifications. Non-null only in Daemon mode.
    SubscriptionManager* subscriptions = nullptr;
};

// Resolve the service mode from the DAEMON_APP_SERVICE_MODE environment variable
// ("daemon" / "mock", case-insensitive). Anything else (including unset) returns the fallback,
// which is now Daemon: a plain launch is daemon-backed (live agent enablement #1).
//
// DEFAULT (live agent enablement #1): the code default is `Daemon`. `DAEMON_APP_SERVICE_MODE=mock`
// is the documented offline/test escape hatch (mock connection + canned turn simulator, no live
// daemon); `=daemon` is honored but redundant. Mock is never a default and never a GUI-selectable
// provider - it is reachable only under this flag. The offscreen ctests are unaffected: no test
// resolves the mode from the environment, and the ones that build the graph pass ServiceMode
// explicitly (tst_app_service_graph).
ServiceMode serviceModeFromEnvironment(ServiceMode fallback = ServiceMode::Daemon);

// Shared service factory for GUI and TUI. The current Mock mode preserves existing behavior; Daemon
// mode swaps in daemon-backed services as they land, starting with the connection seam.
AppServiceGraph createAppServiceGraph(ServiceMode mode, QObject* owner);

} // namespace daemonapp::daemon
