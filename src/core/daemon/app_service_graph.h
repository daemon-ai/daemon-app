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
} // namespace automation
namespace config {
class IDaemonConfig;
}
namespace connection {
class IConnectionService;
}
namespace feedback {
class IFeedback;
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
namespace setup {
class AgentSetupModel;
}
namespace settings {
class ISettingsStore;
}
namespace tools {
class IToolInventory; // [wave2:app-approvals-safety] D2
}
namespace transports {
class IContactsService;
class IPresenceService;
class ITransportRegistry;
} // namespace transports
namespace update {
class UpdateManager;
}

namespace daemonapp::daemon {

class AgentRepository;
class ContactsRepository; // [acct-mgmt] transport contacts / roster (wire v34)
class EngineIdentity;
class ApprovalRepository;
class AuthRepository;
class CapsRepository;
class FleetRepository;
class GatewayRepository; // Phase F: node OpenAI-compatible gateway (GatewayGet/Set)
class TransportRepository;
class RoutingRepository;
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
class ToolRepository;        // [wave2:app-approvals-safety] D2
class FingerprintRepository; // [wave2:app-approvals-safety] D4

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
    // User-feedback seam (thumbs up/down + app feedback) and the node-owned
    // telemetry-consent source of truth. Mock-backed today (MockFeedback); a
    // daemon adapter (FeedbackSubmit / TelemetryConsentGet/Set, wire v31) lands
    // in a later integration phase.
    feedback::IFeedback* feedback = nullptr;
    fs::IFsService* fs = nullptr;
    memory::IMemoryService* memory = nullptr;
    nav::NavController* nav = nullptr;
    firstrun::FirstRunModel* firstRun = nullptr;
    // Shared write-side setup view-model (Phase C): the one "engine + backend + inference" pipeline
    // the GUI AgentSetupForm + TUI AgentSetupView bind and FirstRunModel delegates to. Built in
    // both modes; exposed to QML as the `AgentSetup` context property and reachable by the TUI via
    // this field. Its foreign-agent catalog + EngineIdentity summariser are injected from the
    // graph.
    setup::AgentSetupModel* agentSetup = nullptr;
    models::IModelCatalog* modelCatalog = nullptr;
    models::IProviderCatalog* providerCatalog = nullptr;
    accounts::IAccountsService* accounts = nullptr;
    profiles::IProfileStore* profiles = nullptr;
    fleet::ISessionRoster* roster = nullptr;
    fleet::IFleetTree* fleetTree = nullptr;
    fleet::IApprovalsInbox* approvals = nullptr;
    // [wave2:app-approvals-safety] D2: read-only node-wide tool inventory (Settings -> Tools).
    tools::IToolInventory* tools = nullptr;
    fleet::IDashboard* dashboard = nullptr;
    automation::ICronStore* cron = nullptr;
    transports::ITransportRegistry* transportRegistry = nullptr;
    transports::IPresenceService* presence = nullptr;
    // [acct-mgmt] Transport contacts / roster (Phase D). Mock-backed by default; a
    // DaemonContactsService replaces it in Daemon mode (projects ContactsRepository).
    transports::IContactsService* contacts = nullptr;
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
    // [wave2:app-approvals-safety] D2/D4: tool inventory + per-session remembered fingerprints
    // (Daemon mode only; the facades project these).
    ToolRepository* toolRepository = nullptr;
    FingerprintRepository* fingerprintRepository = nullptr;
    CheckpointRepository* checkpointRepository = nullptr;
    FleetRepository* fleetRepository = nullptr; // PRO-9/10; non-null only in Daemon mode
    // [wave2:app-delegation] F7/DEL-7: delegation guardrail ceilings (read-only). Daemon mode only.
    CapsRepository* capsRepository = nullptr;
    // Phase F: the node's resident OpenAI-compatible gateway (GatewayGet/GatewaySet ->
    // GatewayStatus) control seam. Read + toggle only; it never polls Health. Daemon mode only
    // (null in mock). Its statusChanged drives AgentSetupModel::setGatewayEnabled so the setup
    // form's NodeProvider gateway affordance reflects real gateway state.
    GatewayRepository* gatewayRepository = nullptr;
    TransportRepository* transportRepository = nullptr; // Channels (EIO-1/3/8/9); Daemon mode only
    // [acct-mgmt] Transport contacts / roster (RosterList/Add/Update/Remove + contact ops, wire
    // v34); Daemon mode only (the DaemonContactsService projects it).
    ContactsRepository* contactsRepository = nullptr;
    RoutingRepository* routingRepository = nullptr; // Routing pins (B6/ROU); Daemon mode only
    // The foreign-agent catalog (foreign engines; wire v29): backs the new-agent dialog's engine
    // picker and the Agents settings surface (register/remove). Constructed with the other
    // repositories (inert without a connection).
    AgentRepository* agents = nullptr;
    // Shared engine-identity presentation facade (C3/C4/C5): session/profile -> engine chip label,
    // joining the profile engine selector with the catalog protocol. Built in both modes.
    EngineIdentity* engineIdentity = nullptr;
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
