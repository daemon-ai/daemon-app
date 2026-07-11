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
class IChatService;
class IContactsService;
class IPersonsService;
class IPresenceService;
class ITransportRegistry;
} // namespace transports
namespace update {
class UpdateManager;
}
// mirror A5 (spec 09 wave M2): the live mirror substrate wired beside the legacy paths
// (dual-write). Forward-declared so this widely-included header stays free of the mirror headers;
// the pointers are null when the immer substrate is not built.
namespace mirror {
class MirrorService;
class LocalDatabase;
class Outbox;
} // namespace mirror

namespace daemonapp::daemon {

class DaemonFetchExecutor; // mirror A5: FetchExecutor over NodeApiClient (Daemon mode)
class MockScenarioHost;    // mirror A8: the mock scenario driver (Mock mode; §9)

class AgentRepository;
class ContactsRepository; // [acct-mgmt] transport contacts / roster (wire v34)
class ChatRepository;     // [integrations wire v38] native chat (ConvHistory / ConvSend)
class PersonsRepository;  // [integrations wire v38] person registry (PersonList)
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
class ITranscriptMirrorSink; // A7T: engine transcript stream -> mirror window (substrate only)

enum class ServiceMode {
    Mock,
    Daemon,
};

// The shared GUI/TUI service bundle.
//
// createAppServiceGraph() logs the daemon mode's live-seam census at startup so a partially-live
// state is never mistaken for fully live.
struct AppServiceGraph {
    // mirror A7 (M4) → AD: THE session store — a MirrorSessionStore projecting the mirror
    // `sessions` table + transcript windows in BOTH modes (daemon: ingestor-fed; mock:
    // scenario-seeded). Session-meta mutations ride the outbox lane; create rides the direct
    // SessionCreate seam. The legacy stores (CachedSessionStore / InMemorySessionStore) are
    // deleted. NEVER null once the graph is built.
    persistence::ISessionStore* storeMirror = nullptr;
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
    // [integrations wire v38] Person registry + native chat seams. Mock-backed by default; the
    // Daemon* services replace them in Daemon mode (projecting PersonsRepository / ChatRepository).
    transports::IPersonsService* persons = nullptr;
    transports::IChatService* chat = nullptr;
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
    // [integrations wire v38] Person registry (PersonList) + native chat (ConvHistory / ConvSend);
    // Daemon mode only (the DaemonPersonsService / DaemonChatService project them). The
    // SubscriptionManager routes PersonsChanged / MessagesChanged into them.
    PersonsRepository* personsRepository = nullptr;
    ChatRepository* chatRepository = nullptr;
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

    // --- mirror A5 (spec 09 wave M2): the live mirror substrate, wired beside the legacy paths
    // (dual-write, §13). The ingestor is the single writer; it is fed the node event stream through
    // the daemon bridge and drives fetches over the DaemonFetchExecutor (Daemon mode). The M2 read
    // lenses (ChatWindowModel / PersonListModel / ContactListModel / ConversationListModel) read
    // this store; the outbox owns the ConvSend + turn-prompt lanes (§6). Null when the immer
    // substrate is not built into this stack. ---
    mirror::MirrorService* mirrorService = nullptr;
    DaemonFetchExecutor* mirrorExecutor = nullptr; // Daemon mode only
    mirror::LocalDatabase* localDb = nullptr; // precious local-<id>.db (outbox + sidecar, §4.5)
    mirror::Outbox* outbox = nullptr;         // durable write queue (§6): ConvSend + turn-prompt
    // A7T (spec 09 §13 wave M4 sub-step 6): the turn engine dual-writes its coalesced transcript
    // blocks here so w_transcript_blocks tracks the live/rebaselined transcript. Non-null only in
    // Daemon mode with the substrate; null (no-op) otherwise. The read facade
    // (MirrorSessionStore::content) still delegates to the legacy cache — the flip is withheld on
    // the entity-field gap (LEDGER-a7t).
    ITranscriptMirrorSink* transcriptMirrorSink = nullptr;
    // mirror A8 (spec 09 §9, wave M5): the mock scenario driver — the Seeder (the mock single
    // Writer) + scripted timeline + scripted verb outcomes + the api/<N> mock Hello. Mock mode
    // with the substrate only; null everywhere else.
    MockScenarioHost* mockHost = nullptr;
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
