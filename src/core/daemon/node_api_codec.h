// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "daemon/daemon_cache_store.h"

#include <optional>
#include <QByteArray>
#include <QList>
#include <QMap>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace daemonapp::daemon {

struct DecodedServiceHealth {
    QString name;
    bool ok = false;
    quint32 restarts = 0;
    QString detail;
};

struct DecodedHealth {
    bool allOk = false;
    QList<DecodedServiceHealth> services;
};

// A redacted credential entry (CredentialList): never a secret, just whether one is stored + a
// hint.
struct DecodedCredentialInfo {
    QString profile;
    bool present = false;
    QString hint;
    // [acct-mgmt] wire v35: node-persisted display label (empty = none; set via
    // CredentialSetLabel).
    QString label;
};

// A discoverable model (Models / ModelCurrent). Optional pricing/context carry a `has*` flag since
// the wire models them as `(uint / null)`.
struct DecodedModelDescriptor {
    QString id;
    QString provider; // ProviderSelector wire string: "mock" | "genai" | "llama_cpp" | "mistral_rs"
    bool hasDisplayName = false;
    QString displayName; // optional human label; falls back to `id` when absent
    bool hasContextLength = false;
    quint32 contextLength = 0;
    bool hasInputPrice = false;
    quint64 inputPriceMicrosPerMtok = 0;
    bool hasOutputPrice = false;
    quint64 outputPriceMicrosPerMtok = 0;
    bool local = false;
};

// One provider the node offers (ProviderCatalog -> [ProviderDescriptor]). `id` is the discovery
// key (e.g. "anthropic", "openai", "daemon_cloud", "llama_cpp") - NOT a ProviderSelector: all cloud
// genai vendors share ProviderSelector::GenAi and are disambiguated by this id. `wireSelector` is
// the ProviderSelector the persisted profile uses; `defaultBaseUrl` (when present) pre-fills the
// profile base URL so the app never hardcodes an endpoint.
struct DecodedProviderDescriptor {
    QString id;
    QString displayName;
    QString kind; // "local" | "cloud" | "daemon_cloud"
    QString
        wireSelector; // ProviderSelector wire string: "genai" | "daemon_api" | "llama_cpp" | ...
    bool requiresKey = false;
    bool supportsModelDiscovery = false;
    bool hasDefaultBaseUrl = false;
    QString defaultBaseUrl;
    // [waveB:app-v30] CON-15: an optional interactive sign-in the node offers for this provider
    // (ProviderDescriptor.sign_in = ProviderSignIn{family, label}). `signInFamily` is the generic
    // auth family to begin (passed straight to auth_begin{family}); `signInLabel` is the node's
    // button label. Both empty when the node advertises no sign-in — the client shows nothing and
    // never fabricates a family or label (zero vendor knowledge).
    bool hasSignIn = false;
    QString signInFamily;
    QString signInLabel;
};

// A decoded error envelope (ApiResponse::Error): the variant kind + its human-readable message.
struct DecodedApiError {
    QString kind; // "UnknownSession" | "Unsupported" | "Conflict" | "Unauthenticated" |
                  // "Forbidden" | "Other"
    QString message;
};

// A profile listing entry (ProfileList -> Profiles). `isActive` marks the node's active default,
// which is the profile new sessions bind to (so onboarding stores the credential under it).
struct DecodedProfileInfo {
    QString id;
    QString provider;
    QString model;
    bool isActive = false;
    // Provenance (wire v31): mirrors profile-spec's created_by/owner so a client can badge +
    // subtree-filter the profile list. `createdBy` is the wire `author` ("operator" or an agent id,
    // disambiguated by `createdByIsAgent`); `owner` is the owning agent session id. Both
    // optional-null (a pre-provenance encoding omits both -> operator-authored).
    bool hasCreatedBy = false;
    bool createdByIsAgent = false;
    QString createdBy;
    bool hasOwner = false;
    QString owner;
};

// A transport-instance account bound to a profile (names only, never secrets).
struct DecodedBoundAccount {
    QString transportInstance;
    QString credentialRef;
};

// How a Foreign-engine profile sources its model backend (wire v30 foreign-backend; externally
// tagged union). "AgentNative" steers only the agent's own model over ACP (optional model hint);
// "NodeProvider" routes the agent through the node gateway to a node provider+model (the credential
// stays node-side, referenced by name). Optional on the profile-spec/session-detail: a pre-v30
// encoding omits it and the node treats it as AgentNative{model:null}.
struct DecodedForeignBackend {
    QString kind = QStringLiteral("AgentNative"); // "AgentNative" | "NodeProvider"
    // AgentNative arm: an optional model hint (Option::None on the wire).
    bool hasAgentNativeModel = false;
    QString agentNativeModel;
    // NodeProvider arm: the node provider selector + model, plus an optional credential ref.
    QString nodeProvider =
        QStringLiteral("genai"); // ProviderSelector wire string: "mock"|"genai"|...
    QString nodeModel;
    bool hasNodeCredentialRef = false;
    QString nodeCredentialRef;
};

// The node-derived trust status of a foreign-agent catalog entry (wire v32 agent-verification):
// "Verified" = installed ACP agent that reported a version at initialize; "Unverified" = installed
// but unconfirmed; "NotInstalled" = no candidate found (the serde default — a pre-v32 entry
// omitting the field decodes as NotInstalled until the node re-derives). Node-computed so every
// client renders the same verdict.
enum class AgentVerification {
    Verified,
    Unverified,
    NotInstalled,
};

// The full ProfileSpec (concrete wire profile-spec). Carries every field so a ProfileGet ->
// edit -> ProfileUpdate round-trip preserves the fields the editor does not touch. Nullable
// fields use a `has*` flag (Option::None on the wire); enum-ish fields are wire strings.
struct DecodedProfileSpec {
    QString id;
    QString provider = QStringLiteral("genai"); // "mock"|"genai"|"llama_cpp"|"mistral_rs"
    QString model;
    // Which execution engine the profile's sessions run on (wire v29 engine-selector): "Core"
    // (the native daemon-core engine, the default) or "Foreign" (a foreign agent referenced from
    // the node's catalog BY NAME via engineForeignAgent — recipes never travel in profiles). The
    // agent's protocol (ACP vs stream-json) is a catalog-entry property, not the engine kind.
    QString engineKind = QStringLiteral("Core"); // "Core" | "Foreign"
    QString engineForeignAgent;                  // the catalog name; only meaningful when "Foreign"
    bool hasBaseUrl = false;
    QString baseUrl;
    QString systemPrompt;
    bool hasToolAllowlist = false; // None = full node toolset; Some(list) = allowlist
    QStringList toolAllowlist;
    bool hasBudgetTokens = false;
    quint32 budgetTokens = 0;
    bool hasBudgetWallMs = false;
    quint32 budgetWallMs = 0;
    bool hasModelRetryAttempts = false;
    quint32 modelRetryAttempts = 0;
    bool hasContextBudgetTokens = false;
    quint32 contextBudgetTokens = 0;
    bool hasMaxIterations = false;
    quint32 maxIterations = 0;
    bool hasToolResultBudget = false;
    quint32 toolResultBudget = 0;
    QString contextEngine = QStringLiteral("lcm");        // "lcm"|"budgeted"
    QString memoryProvider = QStringLiteral("mnemosyne"); // "mnemosyne"|"file"|"none"
    bool hasCredentialRef = false;
    QString credentialRef;
    bool hasFallbackCredentialRef = false;
    QString fallbackCredentialRef;
    QList<DecodedBoundAccount> boundAccounts;
    // The Foreign-engine model backend (wire v30 foreign-backend). Only meaningful when
    // engineKind == "Foreign"; absent on a pre-v30 encoding (hasForeignBackend == false, the node
    // defaults it to AgentNative{model:null}).
    bool hasForeignBackend = false;
    DecodedForeignBackend foreignBackend;
    // Provenance (wire v31): who authored the profile + the owning agent session id. `createdBy` is
    // the wire `author`: "operator" (or absent -> operator-authored) or an agent id —
    // `createdByIsAgent` disambiguates an agent named literally "operator". `owner` is the owning
    // agent session id (set only for an agent-authored `agent/{session}/{name}` profile). Both
    // optional-null on the wire.
    bool hasCreatedBy = false;
    bool createdByIsAgent = false;
    QString createdBy;
    bool hasOwner = false;
    QString owner;
};

// --- Foreign-agent catalog (foreign engines; wire v29 dialog + settings surface)
// --------------------------------- One catalog row (AgentCatalog -> [AgentEntry]): a
// known/registered foreign agent. The new-agent dialog's engine picker and the Agents settings
// surface render these; a profile references an entry BY NAME ONLY (the launch recipe stays
// node-side, so it is deliberately NOT decoded here — no client surface needs it and nothing
// client-side may ever re-send one).
struct DecodedAgentEntry {
    QString name;
    QString source = QStringLiteral("Builtin"); // "Builtin" | "Manual" | "Endpoint"
    QString protocol = QStringLiteral("Acp");   // "Acp" | "StreamJson" (agent's wire protocol)
    bool installed = false;
    QString version; // ACP protocol version reported at initialize; empty = unprobed / stream-json
    // The node-derived trust verdict (wire v32 agent-verification). Optional on the wire; a pre-v32
    // entry omitting it decodes as NotInstalled (the serde default) — never re-derived client-side.
    AgentVerification verification = AgentVerification::NotInstalled;
};

// Register input (AgentRegister): the operator-supplied launch recipe for a manual foreign agent.
// The node forces source=Manual and RE-PROBES (a caller-supplied `installed` is never trusted), so
// this carries only what the operator types: identity + protocol + a stdio recipe
// (program/args/env) OR a `tcp://` endpoint. An empty program AND endpoint is invalid (the form
// blocks send).
struct DecodedAgentRecipeInput {
    QString name;
    QString protocol = QStringLiteral("Acp"); // "Acp" | "StreamJson"
    QString program;                          // stdio: the executable (PATH-resolved node-side)
    QStringList args;                         // stdio: program arguments
    QVariantMap env;                          // stdio: extra environment (string -> string)
    QString endpoint; // OR a tcp:// endpoint (mutually exclusive w/ program)
};

// --- Routing (B6/ROU: the origin->session pin table; wire v28) -------------------------------
// A wire `origin` (transport instance + scope), flattened to optional fields keyed by
// `scopeKind` (mirrors domain::Origin; the daemonnet adapter converts).
struct DecodedOrigin {
    QString transport;
    QString scopeKind = QStringLiteral("internal"); // "dm" | "group" | "api" | "internal"
    QString user;                                   // Dm
    QString chat;                                   // Group
    bool hasThread = false;                         // Group: thread present
    QString thread;                                 // Group (optional)
    QString apiKey;                                 // Api
};

// One explicit chat pin (RoutingListChats -> ChatRoutes page items / RoutingGet -> ChatRoute).
struct DecodedChatRoute {
    DecodedOrigin origin;
    QString session;
    QString profile;   // empty = no agent override
    QString isolation; // "PerUser" | "PerChat" | "PerThread" | "Shared" | "" (absent)
};

// One bindable room/chat on a transport instance (TransportRooms -> Rooms page items). The pin
// picker renders these; `session` (when set) is the room's pinned session.
struct DecodedRoomInfo {
    QString transport;
    QString room;    // the chat handle / room id (the Group scope key)
    QString name;    // display label (empty = use the room id)
    QString session; // pinned session id (empty = not pinned)
};

// --- Checkpoints (E4/TOOL-9) ----------------------------------------------------------------
// One durable checkpoint (CheckpointList -> Checkpoints page). Node-created on tool events;
// `turnOrdinal`/`cursorSeq` locate it on the session timeline (optional on the wire).
struct DecodedCheckpointInfo {
    QString id;
    QString session;
    QString tool; // the tool whose execution the checkpoint precedes
    quint64 createdUnix = 0;
    bool hasTurnOrdinal = false;
    quint64 turnOrdinal = 0;
    bool hasCursor = false;
    quint64 cursorSeq = 0;
};

// --- Profile distribution + version history (PRO-7 / PRO-8) --------------------------------------
// One bundled skill inside a profile distribution (SkillBundle). `files` is path -> UTF-8 content.
struct DecodedSkillBundle {
    QString name;
    QString category; // empty == none
    QMap<QString, QString> files;
};

// A portable profile distribution (ProfileExport -> Distribution; ProfileImport consumes one).
struct DecodedDistribution {
    quint32 wireVersion = 0;
    DecodedProfileSpec profile;
    QList<DecodedSkillBundle> skills;
    QString source; // empty == none
};

// One entry of a profile's content-addressed revision log (ProfileHistory -> Revisions).
struct DecodedRevision {
    quint64 seq = 0;
    bool hasParent = false;
    quint64 parent = 0;
    QString author; // "operator" or an agent id
    QString reason;
    quint64 tsMs = 0;
};

// --- Channels / Events-IO read surface (story 04: EIO-1/3/8/9)
// ------------------------------------ One self-describing transport adapter family
// (TransportAdapters -> Adapters). Backs the "Add channel" picker; the account-setup schema is
// omitted here (connect/EIO-2 is deferred).
struct DecodedAdapterInfo {
    QString family;           // "matrix" | "room" | "http" | "a2a"
    QString displayName;      // "Matrix" | "Rooms (internal)"
    QVariantMap capabilities; // rooms/directMessages/presence/roomEnumeration/fileTransfer/
                              // interactiveAuth -> bool
    // [waveB:app-v30] D3: node-labeled adapter policy rows (wire v30, optional). Each entry is a
    // map {key, label, value}: `label`/`value` are the node's display strings (rendered verbatim),
    // `key` is a stable identifier the client NEVER keys behavior off (display only).
    QList<QVariantMap> policies;
    // [acct-mgmt] Per-verb adapter ops (wire v33, optional-nullable on the wire). Each has*Ops is
    // true ONLY when the node reported a concrete ops map — absent (a v32 node / non-messaging
    // adapter) and null (an adapter without that feature) both decode false, meaning "no per-verb
    // info" (the UI then falls back to the legacy coarse capability heuristic). When true, the
    // map is AUTHORITATIVE: verb -> bool with the capabilities map's camelCase key convention.
    bool hasConversationOps = false;
    QVariantMap conversationOps; // create/joinChannel/leave/delete/send/setTopic/setTitle/
                                 // setDescription -> bool
    bool hasMembershipOps = false;
    QVariantMap membershipOps; // invite/remove/ban/setRole -> bool
    bool hasContactsOps = false;
    QVariantMap contactsOps; // getProfile/actionMenu/setAlias -> bool
    bool hasRosterOps = false;
    QVariantMap rosterOps; // list/add/update/remove -> bool (list, wire v34, gates the Contacts UI)
    bool hasDirectory = false;
    bool directory = false; // the adapter offers a people directory (DirectorySearch)
};

// One configured transport instance/account + its live status (TransportInstances ->
// TransportInstances). The Pidgin-style status dot binds to `connection`/`presence` (EIO-9).
struct DecodedTransportInstance {
    QString transport;                              // instance-qualified id (e.g. matrix/@bot:hs)
    QString family;                                 // adapter family
    QString displayName;                            // human label (e.g. @user:hs)
    QString connection = QStringLiteral("offline"); // offline|connecting|connected|disconnecting|
                                                    // error
    QString presence = QStringLiteral("unknown");   // unknown|offline|available|idle|away|busy
    QString boundProfile;                           // empty == unbound
    // [waveB:app-v30] D1: node-reported disconnect provenance (wire v30). `reason` is a coarse
    // lowercase token (disconnectReasonName; empty when absent); `message` is the node's human
    // string rendered verbatim; `fatal` gates the re-auth affordance (the ONLY thing it drives).
    QString reason;
    QString message;
    bool fatal = false;
    // [acct-mgmt] wire v35: node-persisted desired state + display label. `enabled` defaults true
    // when the wire field is absent; `label` (empty = none) overlays the display name wherever the
    // account renders (set via TransportSetLabel).
    bool enabled = true;
    QString label;
};

// [acct-mgmt] One field of an AccountSettingsSchema ({key,label,required}). Shared by the
// ChannelJoinDetails / CreateConversationDetails extras schemas (SettingsSchemaForm renders these).
struct DecodedSettingsField {
    QString key;
    QString label;
    bool required = false;
};

// [acct-mgmt] One transport contact (contact-info: RosterList/DirectorySearch page items,
// RosterAdd/Update/Remove + ContactGetProfile/SetAlias targets). The bare identity shape the wire's
// `contact-info` carries — `id` plus the node's optional display name / presence / permission,
// projected to display strings. A conversation member EMBEDS one of these (decodeMemberStruct
// reuses decodeContactInfoStruct); the roster surface uses it standalone. The node owns every
// field — the app renders, never derives.
struct DecodedContact {
    QString id;
    QString displayName; // empty = use id
    QString
        presence; // offline|available|idle|invisible|away|dnd|streaming|out_of_office (empty=none)
    QString permission; // unset|allow|deny (empty = absent)
};

// [acct-mgmt] One member of a conversation (ConversationInfo.members; ConvGet). Contact identity +
// membership facets, projected to display strings. `presence`/`permission`/`typing`/`role` are the
// wire enum names lowercased where the family uses them; `session` is the pinned session id (empty
// = none). The node owns all of these — the app renders, never derives.
struct DecodedConversationMember {
    QString contactId;
    QString displayName; // empty = use contactId
    QString
        presence; // offline|available|idle|invisible|away|dnd|streaming|out_of_office (empty=none)
    QString permission; // unset|allow|deny (empty = absent)
    QString alias;      // operator-set local alias (empty = none)
    QString nickname;   // in-room nickname (empty = none)
    QString typing;     // none|typing|paused (empty = absent)
    QString role;       // None|Voice|HalfOp|Op|Founder (empty = absent)
    QString session;    // pinned session id (empty = none)
};

// One live conversation/room within a transport (ConvList -> Conversations; EIO-8). [acct-mgmt]
// Extended with `description` and the optional `members` list (ConvGet -> ConversationInfo carries
// members; ConvList typically omits them — `hasMembers` marks presence).
struct DecodedConversation {
    QString transport;
    QString id;
    QString kind; // unset|dm|groupdm|channel|thread
    QString title;
    QString topic;
    QString description;
    bool hasMembers = false;
    QList<DecodedConversationMember> members;
};

// [acct-mgmt] The node-described join form (ConvJoinDetails -> ChannelJoinDetails). Honor the
// *_supported flags (hide unsupported fields) and *_max_length (input constraints) when rendering.
struct DecodedChannelJoinDetails {
    bool hasName = false;
    QString name; // pre-filled channel name (usually empty)
    bool hasNameMaxLength = false;
    quint32 nameMaxLength = 0;
    bool nicknameSupported = false;
    bool hasNicknameMaxLength = false;
    quint32 nicknameMaxLength = 0;
    bool passwordSupported = false;
    bool hasPasswordMaxLength = false;
    quint32 passwordMaxLength = 0;
    QList<DecodedSettingsField> extrasSchema;
};

// [acct-mgmt] The node-described create form (ConvCreateDetails -> CreateConversationDetails).
// `maxParticipants` is 0 = unlimited; participants are chosen client-side at create time (not part
// of the details schema).
struct DecodedCreateConversationDetails {
    bool hasMaxParticipants = false;
    quint32 maxParticipants = 0;
    QList<DecodedSettingsField> extrasSchema;
};

// [acct-mgmt] The filled join form the app sends back (ConvJoin -> ChannelJoinDetails). Optional
// nickname/password ride only when the schema advertised support; extras is the filled schema map.
struct ConvJoinForm {
    QString name;
    bool hasNickname = false;
    QString nickname;
    bool hasPassword = false;
    QString password;
    QMap<QString, QString> extras;
};

// [acct-mgmt] The filled create form (ConvCreate -> CreateConversationDetails). `participants` is
// a list of contact ids (create-time only); extras is the filled schema map.
struct ConvCreateForm {
    bool hasMaxParticipants = false;
    quint32 maxParticipants = 0;
    QStringList participants;
    QMap<QString, QString> extras;
};

// The agent-event arms a turn produces (daemon-protocol AgentEvent). Carried inside a
// session-payload (Subscribe -> LogPage) or an outbound (Poll -> Drained). The turn engine applies
// these to the transcript: TextDelta grows the assistant message; TurnFinished ends the turn.
enum class AgentEventKind {
    Other,
    TurnStarted,
    TextDelta,
    ReasoningDelta,
    ToolStarted,
    ToolFinished,
    Usage,
    Context,
    TurnFinished,
    Error,
};

struct DecodedAgentEvent {
    AgentEventKind kind = AgentEventKind::Other;
    quint64 seq = 0;
    QString text;               // TextDelta / ReasoningDelta text; Error failure message
    QString toolName;           // ToolStarted / ToolFinished tool name
    QString callId;             // ToolStarted / ToolFinished tool call id (CHA-3)
    QString toolArgs;           // ToolStarted args summary (CHA-3)
    bool toolOk = false;        // ToolFinished result ok (CHA-3)
    QString toolSummary;        // ToolFinished full result content (wire `summary`; D1)
    QString detailKind;         // ToolStarted / ToolFinished structured detail kind (e.g. "todo")
    QByteArray detailBody;      // ToolStarted / ToolFinished structured detail payload (JSON)
    quint32 inputTokens = 0;    // Usage delta (CHA-3)
    quint32 outputTokens = 0;   // Usage delta (CHA-3)
    quint32 costMicros = 0;     // Usage delta (CHA-3)
    quint32 contextUsed = 0;    // Context status used_tokens (CHA-3)
    bool hasContextMax = false; // Context status max_tokens present
    quint32 contextMax = 0;     // Context status max_tokens (CHA-3)
    QString endReason;          // TurnFinished: Completed | Failed | Interrupted | ...
    QString finalText;          // TurnFinished: the turn's final assistant text (if any)
    bool turnCompleted = false; // TurnFinished with end_reason == Completed
    // [waveB:app-v30] C6: TurnSummary.failure (wire v30) — a foreign-agent failure on a terminal
    // TurnFinished. `failureStage` is "Spawn"|"Handshake"|"Turn"|"Unknown"; `failureAgent` is the
    // agent name when the node reported one. hasFailure marks a present failure (drives the
    // stage-specific error copy; mid-turn death arrives as a normal terminal Failed carrying this).
    bool hasFailure = false;
    QString failureStage;
    QString failureAgent;
};

// One decoded session-log entry (Subscribe -> LogPage). The transcript engine consumes the
// `event` when payloadKind == Event and renders inbound Command echoes (the node records the
// user's StartTurn/Steer on the merged log before the engine replies) as user Message blocks;
// Meta/Response entries are surfaced so callers can advance the cursor without rendering them.
struct DecodedLogEntry {
    quint64 seq = 0;
    QString direction;   // Inbound | Outbound
    QString disposition; // Context | Transport
    QString originTransport;
    enum class PayloadKind {
        None,
        Event,
        Request,
        Command,
        Response,
        Meta
    } payloadKind = PayloadKind::None;
    DecodedAgentEvent event; // valid when payloadKind == Event
    // Valid when payloadKind == Command: the user-visible text of a StartTurn (input.text) or
    // Steer (text) command echo. Empty for the non-conversational arms (Interrupt/Snapshot/...).
    QString commandText;
    // Valid when payloadKind == Request (a parked HostRequest the turn is blocked on, HITL).
    quint32 hostRequestId = 0;
    QString hostKind; // "Approval" | "Input" | "Choice" | "Delegate" | "Spawn"
    QString hostPrompt;
    QStringList hostOptions; // Choice options
    // Approval only (wire v28): the node offered an "allow permanently" resolution for this gate.
    // Drives the inline "Allow permanently" affordance's visibility — the app never shows it unless
    // the node advertised it here.
    bool hostAllowPermanentOffered = false;
    // [wave2:app-delegation] F1/F2: delegation completion-notice provenance on a StartTurn Command
    // (wire v29 user_msg.notice). The node injects a detached child's completion as a fresh user
    // turn; when `hasNotice` is set, `noticeChild` is the finished child session id and
    // `noticeCallId` (optional) is the parent orchestrate tool call_id, so the client renders a
    // linked delegation chip instead of raw "[subagent … completed]" text. NOTE: user_msg.notice is
    // ALWAYS present on the wire (possibly null) — only a non-null CompletionNoticeRef sets
    // hasNotice.
    bool hasNotice = false;
    QString noticeChild;
    QString noticeCallId;
};

// A pending approval (ApprovalsPending -> Approvals). request_id is a STRING here (the aggregate
// inbox), distinct from the in-stream HostRequest.request_id (uint).
struct DecodedApprovalInfo {
    QString session;
    QString requestId;
    QString prompt;
    bool hasPath = false;
    QString path;
    // Durable "allow permanently" offer signal (wire v28): the node attaches a command fingerprint
    // to an approval it can remember permanently. The inbox offers "Allow permanently" only when
    // this is true, mirroring the node, which honors durable permanence solely for a verified
    // fingerprint (a permanent decision on a fingerprint-less approval degrades to a single allow).
    bool hasFingerprint = false;
    // [wave2:app-approvals-safety] D3: the fingerprint hex string itself (ApprovalInfo.fingerprint,
    // wire v28) — lowercase-hex sha256 of the resolved (exec-surface, abs-binary, argv, env-delta,
    // cwd) tuple. Display/correlation only (a chip in the structured prompt); empty when absent.
    QString fingerprint;
    // [waveB:app-v30] D5: optional structured detail (ApprovalInfo.detail = ToolDetail{kind,body}).
    // `detailKind` is the kind tag (e.g. "fs.diff"); `detailBody` is the raw JSON body (for
    // "fs.diff" it is {path, diff} — a unified diff). Empty when the node attached no detail.
    QString detailKind;
    QByteArray detailBody;
};

// [wave2:app-approvals-safety] D2: one entry of the node-wide tool inventory (ToolList -> Tools,
// wire v29). `enabled` is whether the tool is compiled/keyed and advertised; `requires` names the
// unmet prerequisite (e.g. a credential token, a build feature, an MCP server) when disabled, empty
// otherwise. Display/inventory only — the node owns tool gating; there is no client toggle op at
// v29 (ToolRegister is a register verb, Unsupported).
struct DecodedToolInfo {
    QString name;
    QString description;
    bool enabled = false;
    QString requires_; // trailing underscore: `requires` is a reserved identifier in some contexts
};

// [wave2:app-approvals-safety] D4: one remembered exec-approval fingerprint on a session's
// per-session allow-list (FingerprintList -> Fingerprints, wire v29). `fingerprint` is the hex
// digest; `label` is an optional human summary (always empty today — the node stores only the
// hash), reserved so future provenance needs no wire bump.
struct DecodedRememberedFingerprint {
    QString fingerprint;
    QString label;
    // [waveB:app-v30] D4-provenance: when the node remembered this fingerprint (Unix ms). 0 when
    // the node reported none. Rendered as a human-formatted timestamp beside the label.
    quint64 rememberedAtMs = 0;
};

// A slash command (CommandList -> Commands). CHA-7.
struct DecodedCommandSpec {
    QString name;
    QString summary;
    QString category;
    QString argsHint;
    bool sideEffecting = false;
};

// A session-search hit (SessionSearch -> SessionSearch). CHA-8.
struct DecodedSessionSearchHit {
    QString session;
    QString title;
    QString snippet;
};

// --- Local model track (Phase 2): search -> quant -> download -> install ----------------------
// The repo-centric discovery shapes. Sizes/counts decode as 32-bit (the codec's `uint` width) -
// fine for the targeted small GGUF quants; a >4 GiB artifact would need a codec bit-size bump.

// One Hugging Face repo hit (ModelSearch -> SearchPage). The Discover list renders these; clicking
// one drives a ModelFiles call for its quant picker.
struct DecodedSearchHit {
    QString repo;
    QString author; // empty when the Hub omits a distinct author
    quint64 downloads = 0;
    quint64 likes = 0;
    bool hasNumParameters = false;
    quint64 numParameters = 0;
    QString pipelineTag;
    QString lastModified;
    bool gated = false;
    bool isPrivate = false;
};

// One downloadable file in a repo (ModelFiles -> [ModelFile]). The quant picker groups by `quant`;
// `isSplit`/`isFirstShard` let the UI aggregate multi-part GGUF sets and name the shard to pull.
struct DecodedModelFile {
    QString path;
    quint64 sizeBytes = 0;
    QString quant; // parsed GGUF quant label (e.g. "Q4_K_M"); empty for non-quantized artifacts
    bool isSplit = false;
    bool isFirstShard = false;
    // Wire v27: a vision-projector (mmproj) companion — listed + downloadable, badged by the
    // picker, never a chat model (recommendation skips it node-side).
    bool isMmproj = false;
};

// A point-in-time download job snapshot (ModelDownloads -> [DownloadStatus]). The repo poll loop
// projects these into the Downloads view; reaching Completed triggers a catalog refresh.
struct DecodedDownloadStatus {
    quint64 id = 0;
    QString modelRepo; // resolved from the job's ModelRef::Hf
    QString modelFile;
    QString state; // "Queued"|"Downloading"|"Completed"|"Paused"|"Cancelled"|"Failed"
    quint64 downloadedBytes = 0;
    quint64 totalBytes = 0;
    quint32 filesDone = 0;
    quint32 filesTotal = 0;
    QString error; // set when state == "Failed"
};

// An installed (downloaded + cataloged) model (ModelCatalog -> [InstalledModel]). The Installed
// list renders these; activate -> ModelActivate(id), delete -> ModelDelete(id).
struct DecodedInstalledModel {
    QString id;
    QString displayName;
    QString repo; // from ModelRef::Hf, when applicable
    QString file;
    QString engine; // "llama" | "mistral_rs"
    QString localPath;
    quint64 sizeBytes = 0;
    QString quant;
    quint64 installedAtMs = 0;
    QString arch;
    bool hasContextLength = false;
    quint32 contextLength = 0;
    QString fileType;
    // Wire v27: the on-disk path of the paired vision-projector (mmproj) companion; empty for
    // text-only models and for projector records themselves.
    QString mmprojPath;
};

// One ranked quant candidate inside a recommendation (so the picker can show the full ladder).
struct DecodedQuantCandidate {
    QString quant;
    QString file;
    bool hasSizeBytes = false;
    quint64 sizeBytes = 0;
    bool fits = false;
};

// The hardware-aware quant recommendation for a repo (ModelRecommend -> QuantRecommendation). The
// quant picker pre-highlights `quant`/`file` with a "Recommended / fits ~N" badge.
struct DecodedQuantRecommendation {
    QString engine; // "llama" | "mistral_rs"
    QString repo;
    QString file; // the chosen GGUF (llama); empty for mistral.rs whole-repo + ISQ
    QString quant;
    bool hasSizeBytes = false;
    quint64 sizeBytes = 0;
    quint64 budgetBytes = 0;
    bool fits = false;
    QString reason;
    QList<DecodedQuantCandidate> candidates;
};

// --- Filesystem surface (Phase 4 DaemonFsService) ------------------------------------------------
// A root id is carried as an opaque client string: "workspace" | "host:<id>" | "session:<id>",
// round-tripping the daemon FsRootId Host/Workspace/Session choice.

// One root the daemon exposes (FsRoots -> [FsRoot]).
struct DecodedFsRoot {
    QString id;    // canonical "workspace" | "host:<id>" | "session:<id>"
    QString label; // human label
    QString kind;  // "host" | "workspace" | "session"
};

// One directory entry (FsList -> [FsEntry] / FsStat -> FsEntry).
struct DecodedFsEntry {
    QString name;
    QString path; // root-relative, POSIX
    QString kind; // "file" | "dir" | "symlink"
    quint64 size = 0;
    quint64 mtimeMs = 0;
    bool ignored = false;
};

// A file's content (FsRead -> FsContent). `revision` is the opaque "mtime:size" etag the IFsService
// seam uses for optimistic concurrency; `truncated` marks a partial read (over max_bytes).
struct DecodedFsContent {
    QByteArray bytes;
    QString revision; // "mtime_ms:size"
    bool truncated = false;
};

// One filesystem change (FsWatch page event).
struct DecodedFsChange {
    QString path;
    QString kind; // "created" | "modified" | "removed"
};

// A page of the watch cursor (FsWatchPoll -> FsWatchPageView; L3-aligned: head_seq + reset).
// `reset`
// => the reader's cursor aged out of the ring; re-list the dir to reconcile.
struct DecodedFsWatchPage {
    QList<DecodedFsChange> events;
    quint64 nextSeq = 0;
    quint64 headSeq = 0;
    bool reset = false;
};

// One project-search hit (FsSearch -> FsSearchPage).
struct DecodedFsSearchHit {
    QString path;
    quint32 line = 1;
    quint32 col = 1;
    QString preview;
};

struct DecodedFsSearchPage {
    QList<DecodedFsSearchHit> hits;
    bool hasMore = false;
};

// The authenticated principal surfaced on AuthOk (wire mirror of daemon_auth::Principal; see
// `principal-view` in the contract). Advisory, for client-side UI gating only - the node enforces
// every capability server-side, so a client must never trust this in lieu of the server's checks.
struct DecodedPrincipalView {
    QString userId;
    QString username;
    QStringList roles;        // snake_case role names, e.g. "operator"
    QStringList capabilities; // snake_case capability names, e.g. "session_write"

    [[nodiscard]] bool isAuthenticated() const { return !userId.isEmpty() || !username.isEmpty(); }
    [[nodiscard]] bool hasCapability(const QString& cap) const {
        return capabilities.contains(cap);
    }
};

// A decoded server->client multiplexed frame (wire L0; daemon-sync-protocol-spec.md §2). The
// `payload` is the raw inner ApiResponse CBOR for Reply/Item (fed straight to the existing
// decoders); it is empty for the control frames.
enum class WireFrameKind {
    Unknown,
    Hello,
    Reply,
    Item,
    End,
    Reset,
    AuthChallenge,
    AuthOk,
    AuthError,
};

struct DecodedWireFrame {
    WireFrameKind kind = WireFrameKind::Unknown;
    quint64 id = 0;        // the Call id this frame answers (0 for Hello)
    QByteArray payload;    // inner ApiResponse CBOR for Reply/Item; empty otherwise
    bool hasError = false; // End closed with an error
    quint64 epoch = 0;     // Reset (L2)
    quint64 headSeq = 0;   // Reset (L2)
    QStringList features;  // Hello: the server's advertised capability strings (mux/stream/...)
    // v2 auth (SASL) fields.
    QStringList authMechanisms;     // Hello: the server's offered SASL mechanisms (empty => unauth)
    QByteArray authData;            // AuthChallenge: opaque mechanism bytes (wire `[* uint]`)
    QString authToken;              // AuthOk: the opaque server-issued session token
    DecodedPrincipalView principal; // AuthOk: the authenticated principal (advisory)
    QString authReason;             // AuthError: a short, non-revealing failure reason
};

// One coalesced durable transcript block (Journal -> journal-record payload Block). The re-baseline
// path (L2) renders these the way the live AgentEvent stream renders, mapped in the turn engine.
struct DecodedTranscriptBlock {
    enum class Kind { Other, Message, ToolCall, ToolResult, Request, Content } kind = Kind::Other;
    QString role;          // Message: "Assistant" | "User" | "System"
    QString text;          // Message text
    QString callId;        // ToolCall / ToolResult call id
    QString toolName;      // ToolCall tool name
    QString argsSummary;   // ToolCall args summary
    bool ok = false;       // ToolResult ok
    QString summary;       // ToolResult summary (the full result content)
    QString detailKind;    // ToolResult structured detail kind (D1; empty when absent)
    QByteArray detailBody; // ToolResult structured detail payload (JSON bytes)
    quint32 requestId = 0; // Request host-request id
    QString hostKind;      // Request: "Approval"|"Input"|"Choice"|"Delegate"|"Spawn"
    QString contentKind;   // Content kind tag
};

// One decoded durable journal record (SessionHistory -> Journal). Carries the resync coordinates
// (cursor/seq/epoch) and either a coalesced transcript Block or a Management note.
struct DecodedJournalRecord {
    quint64 cursor = 0;
    quint64 seq = 0;
    quint64 epoch = 0;
    QString kind;             // record kind tag
    bool isBlock = false;     // false => Management
    QString managementDetail; // Management payload detail
    DecodedTranscriptBlock block;
};

// One decoded node-wide feed notification (EventsSince -> EventsPage; L3 daemon-sync-protocol §5).
// Payload-free pointer: the SubscriptionManager routes by kind and lazily fetches the detail.
struct DecodedNodeEvent {
    enum class Kind {
        Unknown,
        SessionAdvanced,
        SessionMetaChanged,
        RosterChanged,
        ApprovalPending,
        DownloadProgress,
        CatalogChanged,
        ResyncNeeded,
        // [wave2:app-channels-liveness] live fleet + transport presence feed arms (F5/B5).
        FleetChanged,
        TransportChanged,
        // [waveB:app-v30] D2: conversation-set + room-membership deltas (wire v30). Invalidation
        // pointers only — the client refetches the transport's ConvList (+ routing on a self
        // removal); it derives no membership facts locally.
        ConversationsChanged,
        MembershipChanged,
        // [acct-mgmt] A transport's contact roster changed (wire v34): RosterAdd/Update/Remove or a
        // node-side roster sync. Invalidation pointer only (carries just `transport`) — the client
        // refetches that transport's RosterList; it derives no contact facts locally. NOT the
        // GUI-sessions `RosterChanged` (that is the session inbox revision).
        ContactsChanged,
        // Profile roster mutation (wire v31): a create/update/delete/select bumped the profile
        // revision. Invalidation pointer only — the client refetches ProfileList (codec-only here;
        // SubscriptionManager routing lands in a later phase).
        ProfilesChanged
    } kind = Kind::Unknown;
    QString session;        // SessionAdvanced / SessionMetaChanged / ApprovalPending
    quint64 epoch = 0;      // SessionAdvanced
    quint64 headSeq = 0;    // SessionAdvanced
    quint64 rev = 0;        // SessionMetaChanged / RosterChanged / FleetChanged / ProfilesChanged
    QString requestId;      // ApprovalPending
    quint64 downloadId = 0; // DownloadProgress
    quint32 pct = 0;        // DownloadProgress
    QString state;          // DownloadProgress
    quint64 downloadedBytes = 0; // DownloadProgress (wire v26: real byte counters)
    quint64 totalBytes = 0;      // DownloadProgress (0 = unknown total)
    QString scope;               // ResyncNeeded
    // [wave2:app-channels-liveness] TransportChanged (B5): the live per-account connection state
    // (offline|connecting|connected|error) and optional presence (unknown|offline|available|idle|
    // away|busy). `hasPresence` is false when the wire omitted the optional presence field. The
    // v29 wire carries NO disconnect reason (coordinator-confirmed) — only the coarse state.
    QString transport;        // TransportChanged: the transport-instance id
    QString connection;       // TransportChanged: connection-state (lowercase)
    QString presence;         // TransportChanged: presence-state (lowercase; when hasPresence)
    bool hasPresence = false; // TransportChanged: the optional presence field was present
    // [waveB:app-v30] D1: TransportChanged disconnect provenance (wire v30). `reason` is a coarse
    // lowercase token (disconnectReasonName; empty when absent), `message` the node's verbatim
    // human string, `fatal` gates the re-auth affordance. hasReason/hasMessage mark presence.
    QString reason;
    bool hasReason = false;
    QString message;
    bool hasMessage = false;
    bool fatal = false;
    // [waveB:app-v30] D2: ConversationsChanged / MembershipChanged (wire v30). `conv` is the
    // affected conversation id; `convChange` is "added"|"removed"; `member` + `membershipChange`
    // ("joined"|"left"|"invited"|"kicked"|"banned") + optional `actor`/`memberReason` describe a
    // membership delta; `isSelf` marks that the changed member is this node's own identity (the
    // node has already reconciled routing pins on a self removal — the client only refetches).
    QString conv;
    QString convChange;
    QString member;
    QString membershipChange;
    QString actor;
    QString memberReason;
    bool isSelf = false;
};

// A decoded page of the node-wide event feed (EventsSince -> EventsPage). `nextCursor` advances the
// feed subscription; an in-page ResyncNeeded event tells the client to re-baseline.
struct DecodedEventsPage {
    QList<DecodedNodeEvent> events;
    quint64 nextCursor = 0;
    quint64 headCursor = 0;
};

// --- Fleet / subagent tree (Phase 5b) ------------------------------------------------------------
// One node of the supervision tree (Tree -> TreeReport / Unit -> UnitNode). `depth` is set by
// decodeTreeReport when it flattens root + nodes into a pre-order list; raw wire fields otherwise.
struct DecodedUnitNode {
    QString id;
    QString kind;      // "Engine" | "Host" | "Orchestrator"
    QString state;     // "Running" | "Finished" | "Unknown"
    QString endReason; // Finished only
    QString work;
    QStringList children; // child unit ids
    QString profileRef;
    QString sessionId;
    QString title;
    QString role;     // "Primary" | "ManagedChild" | "EphemeralSubagent" | ""
    QString parentId; // set by decodeTreeReport's flatten ("" = the root)
    int depth = 0;    // set by decodeTreeReport's flatten
    // [wave2:app-delegation] F3: authoritative per-child enrichment on the wire (v29 UnitNode).
    // lifetime is the delegation lifetime ("" | "Persistent" | "Ephemeral"); engineKind/engineAgent
    // decode the wire engine-selector ("" | "Core" | "Foreign" + the foreign agent name) so the
    // fleet tree renders lifetime + engine chips without an N-side-read profile join.
    QString lifetime;
    QString engineKind;  // "" | "Core" | "Foreign"
    QString engineAgent; // the foreign agent name (only when engineKind == "Foreign")
};

// The top-level fleet roster (Fleet -> FleetReport): the root unit ids (usage omitted from the
// facade).
struct DecodedFleetReport {
    QStringList children;
};

// One management-event view row (UnitEvents -> [ManageEventView]). Only the Subagent arm is
// projected in full today (the live subagent strip's structured spawn/finish feed); the other arms
// (Started/Progress/Usage/Finished/Error) decode to their kind + seq so a caller can advance past
// them. `phase` is the SubagentPhase wire string ("Spawned" | "Finished"); `child` is the child
// session id; `role` is the SessionRole wire string; `activeChildren` is the parent's live child
// count at that event.
struct DecodedManageEvent {
    enum class Kind {
        Other,
        Started,
        Progress,
        Usage,
        Finished,
        Error,
        Subagent
    } kind = Kind::Other;
    quint64 seq = 0;
    // Subagent arm:
    QString child;
    QString role;  // "Primary" | "ManagedChild" | "EphemeralSubagent"
    QString phase; // "Spawned" | "Finished"
    quint32 activeChildren = 0;
};

// [wave2:app-delegation] F7/DEL-7: the node's delegation guardrail ceilings (Caps -> CapsReport).
// Node-wide policy (keys on nothing session/profile-specific), surfaced read-only — the app cannot
// set them (no wire write op). Drives the read-only "Delegation limits" rows in Settings -> Safety.
struct DecodedCapsReport {
    quint32 orchestrateMaxDepth = 0;
    quint32 orchestrateMaxFanout = 0;
    // The agent-created-agents guardrail ceilings (wire v31): profiles a session may author, and
    // concurrent inline/ephemeral children per session. Optional on the wire (a pre-v31 encoding
    // omits them and they decode as 0).
    quint32 maxComposedProfiles = 0;
    quint32 maxEphemeralPerSession = 0;
};

// The node gateway's live status (wire v32 gateway-status; GatewayGet/GatewaySet -> GatewayStatus).
// `enabled` is the operator toggle; `listening` is whether the listener is actually bound; `addr`
// is the bind address (optional-null); `lastError` carries the most recent bind/serve failure
// (optional-null). Read-only status the node owns — the client renders it and sends intents.
struct DecodedGatewayStatus {
    bool enabled = false;
    bool hasAddr = false;
    QString addr;
    bool listening = false;
    bool hasLastError = false;
    QString lastError;
};

// --- Interactive auth (AuthProviders / AuthBegin / AuthComplete; app-wizard-auth stream) --------
// One family the node offers interactive auth for (AuthProviders -> [AuthProviderInfo]). The
// params schema drives the begin form (e.g. Matrix SSO wants a `homeserver`).
struct DecodedAuthParamField {
    QString key;
    QString label;
    bool required = false;
};

struct DecodedAuthProviderInfo {
    QString family;                            // e.g. "matrix"
    QString flowKind;                          // "MatrixSso" | "OAuth2Pkce"
    QString displayName;                       // human label
    QList<DecodedAuthParamField> paramsSchema; // required/optional begin params
};

// [waveB:app-v31] The wire v31 generalized interactive-auth challenge (daemon-api AuthChallenge):
// the flow is a challenge/response state machine, and each step presents ONE challenge telling the
// client how to collect the next AuthStepInput. The kind picks which per-kind payload below is
// valid; the client renders exactly that arm (redirect URL / form fields / QR / message).
enum class AuthChallengeKind {
    Redirect, // open `authorizationUrl` in a browser + capture the callback (SSO / OAuth2)
    Form,     // collect `formFields` from the user + reply with a key->value map
    Qr,       // render `qrPayload` / `qrImage` + poll every `qrPollIntervalMs` (device pairing)
    Message,  // display `messageText` + poll (informational, e.g. "approve on your other device")
};

struct DecodedAuthChallenge {
    AuthChallengeKind kind = AuthChallengeKind::Message;
    QString authorizationUrl;                // Redirect: the URL to open in a browser
    QString formTitle;                       // Form: a human title for the field set
    QList<DecodedAuthParamField> formFields; // Form: the fields to collect (key/label/required)
    QString qrPayload;                       // Qr: the payload the peer device scans
    QByteArray qrImage;                      // Qr: optional pre-rendered image bytes (empty = none)
    quint64 qrPollIntervalMs = 0;            // Qr: how often to re-poll (ms)
    QString messageText;                     // Message: the text to show
};

// [waveB:app-v31] The client's response to a challenge (daemon-api AuthStepInput). One of the three
// arms, keyed by `kind`: Fields answers a Form (the filled key->value map), Callback answers a
// Redirect (the captured URL/query), Poll answers a Qr/Message (a no-payload "landed yet?").
enum class AuthStepInputKind {
    Fields,
    Callback,
    Poll,
};

// [waveB:app-v31] A parked flow handle (AuthBegin -> AuthBegun): the flow id, its INITIAL
// challenge, and the flow TTL (`expiresAt`, unix seconds). The client presents `challenge` and
// drives the flow forward with AuthStep until it completes.
struct DecodedAuthBeginResponse {
    QString flowId;
    DecodedAuthChallenge challenge;
    quint64 expiresAt = 0;
};

// A finished flow (AuthComplete -> AuthCompleted / AuthStepResult::Completed): where the credential
// blob landed + the resolved account identity. The client never sees the secret itself.
struct DecodedAuthCompleteResponse {
    QString credentialRef;
    QString accountLabel;
    QString transportInstance;
    bool hasBoundProfile = false;
    QString boundProfile;
};

// [waveB:app-v31] The result of advancing a flow one step (AuthStep -> AuthStepped): either the
// next challenge to present (`completed == false`, `challenge` valid) or the completed outcome
// (`completed == true`, `completion` valid).
struct DecodedAuthStepResult {
    bool completed = false;
    DecodedAuthChallenge challenge;         // valid when !completed
    DecodedAuthCompleteResponse completion; // valid when completed
};

// --- Local re-quantization (ModelQuantize/ModelQuantizes; A5) ------------------------------------
// A point-in-time quantize job snapshot (ModelQuantizes -> [QuantizeStatus]); mirrors
// DecodedDownloadStatus so the jobs UI renders both with one idiom.
struct DecodedQuantizeStatus {
    quint64 id = 0;
    QString repo;
    QString sourceFile;
    QString targetQuant;
    QString state;      // "Queued"|"Preparing"|"Quantizing"|"Completed"|"Failed"
    QString outputPath; // set when Completed
    QString modelId;    // the cataloged id of the produced model, when Completed
    QString error;      // set when Failed
};

// --- GGUF introspection (ModelInspect; A6 ghost probe) -------------------------------------------
// The node-read GGUF metadata for an installed model. The ghost-model probe only needs the call to
// SUCCEED (an Error reply on a cataloged id means the artifact is unreadable/missing on disk);
// the metadata itself backs provenance display.
struct DecodedGgufInfo {
    QString architecture;
    QString name;
    QString fileType;
    bool hasContextLength = false;
    quint32 contextLength = 0;
    quint64 sizeBytes = 0;
};

// One outbound delivery target of a session (SessionDetail.delivery_targets -> [DeliveryTarget]).
// `kind` is the wire SinkKind: "Primary" (the authoritative reply sink; exactly one) or
// "Spectator" (a read-only observer). CHA-9 detail hydration.
struct DecodedDeliveryTarget {
    QString transport;
    QString route;
    QString kind = QStringLiteral("Primary"); // "Primary" | "Spectator"
};

// One offered Model choice inside a foreign session's advertised Model selector (model-choice).
struct DecodedModelChoice {
    QString id;
    QString label;
};

// A resident foreign (ACP) session's advertised Model selector (wire v30 model-selector; surfaced
// as live session state): the option id, the current value, and the offered choices (flattened
// across groups). Only present for a foreign session whose agent advertises a Model config option.
struct DecodedModelSelector {
    QString optionId;
    QString current;
    QList<DecodedModelChoice> choices;
};

// The on-demand full detail for one session (SessionGet -> SessionDetail(opt)). CHA-9 follow-on:
// the roster (SessionPage) carries only the SessionInfo row; this hydrates the extra per-session
// facets the node owns - the live overlay's resolved model + approval mode, the outbound delivery
// targets, the child session ids, and the durable checkpoint count. Every facet past `info` is
// optional on the wire (a `has*` flag guards it). The null SessionDetail arm (unknown session)
// surfaces as decodeSessionDetail(..., found=false).
struct DecodedSessionDetail {
    CachedSessionRow info; // the same SessionInfo row shape the roster page carries
    bool hasModel = false;
    QString model; // overlay's resolved model id, when the session pins one
    bool hasApprovalMode = false;
    QString approvalMode; // overlay approval mode: "ask"|"accept_edits"|"auto_allow"|"deny"
    QList<DecodedDeliveryTarget> deliveryTargets;
    QStringList children; // child session ids (delegated children / subagents)
    bool hasCheckpointCount = false;
    quint32 checkpointCount = 0;
    // The foreign agent's advertised Model selector (wire v30 model_selector, optional-null). Set
    // only for a resident foreign session whose agent advertises a Model config option.
    bool hasModelSelector = false;
    DecodedModelSelector modelSelector;
};

enum class ApiResponseKind {
    Unknown,
    Health,
    SessionPage,
    SessionDetail,
    LogPage,
    EventsPage,
    Journal,
    FsRoots,
    FsList,
    FsStat,
    FsRead,
    FsWrite,
    FsWatch,
    FsSearch,
    Fleet,
    Tree,
    Unit,
    UnitEvents,
    Adapters,
    TransportInstances,
    Conversations,
    Profile,
    Distribution,
    Revisions,
    Ok,
    SessionCreated,
    Credentials,
    Models,
    ModelCurrent,
    ProviderCatalog,
    ProviderModels,
    Profiles,
    Drained,
    Approvals,
    Commands,
    CommandOutput,
    SessionSearch,
    Error,
    ModelSearch,
    ModelFiles,
    ModelDownloadStarted,
    ModelDownloads,
    ModelCatalog,
    ModelRecommend,
    AgentCatalog,
    // app-wizard-auth stream additions (appended; keep ordering append-only for sibling merges).
    AuthProviders,
    AuthBegun,
    AuthCompleted,
    // [waveB:app-v31] the generalized multi-step result (AuthStep -> AuthStepped).
    AuthStepped,
    ModelQuantizeStarted,
    ModelQuantizes,
    ModelInspect,
    // app-client-verticals stream additions (appended, order-stable for sibling merges).
    Checkpoints,
    ChatRoutes,
    ChatRoute,
    Rooms,
    // user feedback over OpenTelemetry (wire v32).
    FeedbackAck,
    TelemetryConsent,
    // node gateway status (wire v32; GatewayGet/GatewaySet -> GatewayStatus).
    GatewayStatus,
};

// Input for a FeedbackSubmit request (the app-side view-model builds this from the message anchor /
// the app-feedback dialog). `isResponse` picks the FeedbackKind (response vs app). For response
// feedback `session`+`cursor` locate the rated turn on the durable journal (the node reads the
// descriptor + rated text there); `hasTrace`/`trace` is the optional turn trace id. `rating` is
// 0 (none/absent) | 1 (up) | -1 (down). `hasComment` gates the optional comment. `includeContent`
// is the per-event consent to attach the rated response text to the exported event.
// `hasDiagnostics` gates the optional `appVersion`/`os` (each further optional — empty = absent).
// `surface` is the free-form UI-surface label.
struct FeedbackSubmitInput {
    bool isResponse = false;
    QString session;
    quint64 cursor = 0;
    bool hasTrace = false;
    quint64 trace = 0;
    int rating = 0; // 0 = none, 1 = up, -1 = down
    bool hasComment = false;
    QString comment;
    bool includeContent = false;
    bool hasDiagnostics = false;
    QString appVersion;
    QString os;
    QString surface;
};

// Thin C++ facade over the zcbor-generated NodeApi codec (codec/generated). The generated C is
// produced and drift-checked by the superproject (flake daemon-zcbor-codec / `just update-codec`)
// from the daemon-node contract; this facade never hand-rolls CBOR.
//
// Scope is the daemon slice the client currently exercises: encode the requests it issues and
// decode the responses it consumes. The raw CBOR bytes travel over DaemonTransport; GUI/TUI view
// models never see CBOR - repositories call this codec and write typed rows into DaemonCacheStore.
class NodeApiCodec {
public:
    // Requests.
    [[nodiscard]] static QByteArray encodeHealthRequest();
    // SessionsQuery at the daemon's default scope. `hasSinceRev` adds the L4 `since_rev` delta read
    // (the server then returns only sessions changed past `sinceRev` + a `removed` list); absent =
    // a full page (back-compat / cold cache). A non-empty `byProfile` sets the query scope to
    // `SessionScope::ByProfile(byProfile)` (the per-agent view; the id already exists in the CDDL
    // session-scope union, so this is encoder-only — no contract change). `archivedScope` (F6)
    // sets scope = `SessionScope::Archived` (archived primaries). A non-empty `byTransport` (B4)
    // sets scope = `SessionScope::ByTransport(id)` (the sessions routed over one transport
    // instance — the node resolves membership; the client never re-derives it). Precedence when
    // several are passed: byProfile > archived > byTransport.
    // `after` (when non-empty) resumes past the previous page's next_cursor (the roster page
    // loop; wire v24 bounds every page at kWirePageMax).
    [[nodiscard]] static QByteArray
    encodeSessionsQueryRequest(bool hasSinceRev = false, quint64 sinceRev = 0,
                               const QString& byProfile = QString(),
                               const QString& after = QString(), bool archivedScope = false,
                               const QString& byTransport = QString());
    // Subscribe to a session's merged log from afterSeq (exclusive), up to max entries.
    [[nodiscard]] static QByteArray encodeSubscribeRequest(const QString& sessionId,
                                                           quint64 afterSeq, quint32 max);
    // Read the durable verifiable journal for a session from afterCursor (exclusive), up to max
    // records (SessionHistory -> Journal). The L2 re-baseline source after an epoch change / Reset.
    [[nodiscard]] static QByteArray encodeSessionHistoryRequest(const QString& sessionId,
                                                                quint64 afterCursor, quint32 max);
    // Read the node-wide event feed past `cursor` (exclusive). Served as a push stream over Open
    // (the SubscriptionManager's single feed) or a one-shot/long-poll page over Call. `hasWaitMs`
    // adds the long-poll hold for the one-shot form (the push lane ignores it).
    [[nodiscard]] static QByteArray encodeEventsSinceRequest(quint64 cursor, bool hasWaitMs = false,
                                                             quint32 waitMs = 0);

    // Basic chat (CHA-1): start a turn in `session` with `text`. Empty `profile` omits the optional
    // profile binding (daemon uses the session's / active profile); `requestId` correlates the
    // turn.
    [[nodiscard]] static QByteArray encodeSubmitStartTurnRequest(const QString& sessionId,
                                                                 const QString& text,
                                                                 const QString& profile = QString(),
                                                                 quint32 requestId = 1);
    // Drain a session's outbound queue (Poll -> Drained). Mainly a harness/diagnostic path; the
    // client renders from Subscribe (non-destructive) instead. max == 0 means "no limit".
    [[nodiscard]] static QByteArray encodePollRequest(const QString& sessionId, quint32 max = 0);

    // Node-authoritative session creation (WireVersion v23): create a blank, profile-bound, UN-RUN
    // session on the node (SessionCreate{session?, profile?} -> SessionCreated{session}). An empty
    // `sessionId` lets the node MINT the id (the node-authority path — nothing is client-minted);
    // an empty `profile` binds the node's active default. The client then updates + auto-selects
    // from the SessionCreated reply / the RosterChanged event.
    [[nodiscard]] static QByteArray encodeSessionCreateRequest(const QString& sessionId = QString(),
                                                               const QString& profile = QString());
    // Decode a SessionCreated response into the node-minted/accepted session id.
    static bool decodeSessionCreated(const QByteArray& responseCbor, QString* outId);

    // Fetch one session's full detail (SessionGet{session} -> SessionDetail(opt)). CHA-9 follow-on:
    // the on-demand hydration of the per-session facets the roster page omits (delivery targets,
    // child ids, checkpoint count, resolved model/approval mode).
    [[nodiscard]] static QByteArray encodeSessionGetRequest(const QString& sessionId);

    // Patch a session's node-owned metadata (SessionUpdateMeta{session, patch} -> Ok). Each
    // optional is Some(value) = set that field, std::nullopt = leave it untouched (the key is
    // omitted from the SessionMetaPatch map). The node persists the change, replies Ok, and emits
    // SessionMetaChanged so the roster re-projects from the authoritative row - the app never
    // caches-and-mutates the pin/archive/title state locally. (Only the value arm is ever sent;
    // clearing a title via the wire's `null` arm is not a current UI affordance.)
    [[nodiscard]] static QByteArray encodeSessionUpdateMetaRequest(const QString& sessionId,
                                                                   std::optional<bool> pinned,
                                                                   std::optional<bool> archived,
                                                                   std::optional<QString> title);

    // Onboarding (CON-4 / CON-6): credentials + model discovery/selection.
    // Store a provider secret under `profile` (CredentialSet -> Ok). The secret never returns.
    [[nodiscard]] static QByteArray encodeCredentialSetRequest(const QString& profile,
                                                               const QString& secret);
    // List redacted credentials (CredentialList -> Credentials).
    [[nodiscard]] static QByteArray encodeCredentialListRequest();
    // Remove the secret stored under `profile` (CredentialRemove -> Ok).
    [[nodiscard]] static QByteArray encodeCredentialRemoveRequest(const QString& profile);
    // [acct-mgmt] wire v35: persist a credential display label (CredentialSetLabel -> Ok; the node
    // emits no event, so the caller refetches after Ok). `hasLabel=false` emits an explicit null to
    // clear it node-side.
    [[nodiscard]] static QByteArray
    encodeCredentialSetLabelRequest(const QString& profile, bool hasLabel, const QString& label);
    // Discover models (Models -> Models(model-page)). `after` (wire v25) resumes past the
    // previous page's `next` cursor (empty = first page).
    [[nodiscard]] static QByteArray encodeModelsRequest(const QString& after = QString());
    // The current model for `profile` (empty = the active default profile) -> ModelCurrent(opt).
    [[nodiscard]] static QByteArray encodeModelCurrentRequest(const QString& profile = QString());
    // Set the model (and optionally re-bind the provider) for a live session -> Ok.
    [[nodiscard]] static QByteArray
    encodeSetSessionModelRequest(const QString& sessionId, const QString& model,
                                 const QString& provider = QString());
    // Enumerate the providers the node offers (ProviderCatalog -> [ProviderDescriptor]). Zero-arg.
    [[nodiscard]] static QByteArray encodeProviderCatalogRequest();
    // Discover a provider's models (ProviderModels{provider, credential_ref?, transient_key?,
    // after?} -> model-page). `provider` is the ProviderDescriptor.id string. Pass `transientKey`
    // when listing a key-requiring provider before a credential is stored (first-run); pass
    // `credentialRef` (the profile id) for an existing profile's stored key. Both empty = keyless.
    // `after` (wire v25) resumes past the previous page's `next` cursor (empty = first page).
    [[nodiscard]] static QByteArray encodeProviderModelsRequest(const QString& provider,
                                                                const QString& credentialRef = {},
                                                                const QString& transientKey = {},
                                                                const QString& after = {});
    // List profiles (ProfileList -> Profiles) so the client can find the active default profile.
    [[nodiscard]] static QByteArray encodeProfileListRequest();
    // Switch the node's active profile (ProfileSelect -> Ok). New sessions bind to it (PRO-5).
    [[nodiscard]] static QByteArray encodeProfileSelectRequest(const QString& id);
    // Delete a profile (ProfileDelete -> Ok). The default profile is protected node-side (PRO-4).
    [[nodiscard]] static QByteArray encodeProfileDeleteRequest(const QString& id);
    // Create a new profile from a full spec (ProfileCreate -> ProfileId). PRO-2.
    [[nodiscard]] static QByteArray encodeProfileCreateRequest(const DecodedProfileSpec& spec);
    // Replace a profile's spec in place (ProfileUpdate -> Ok). The sole durable editor. PRO-3.
    [[nodiscard]] static QByteArray encodeProfileUpdateRequest(const DecodedProfileSpec& spec);
    // Clone an existing profile under a new id (ProfileClone -> ProfileId). PRO-2 clone path.
    [[nodiscard]] static QByteArray encodeProfileCloneRequest(const QString& source,
                                                              const QString& newId);
    // Fetch a profile's full spec (ProfileGet -> Profile(opt)). PRO-3 editor hydration.
    [[nodiscard]] static QByteArray encodeProfileGetRequest(const QString& id);

    // --- Foreign-agent catalog (foreign engines) ----------------------------------------------
    // List the node's foreign-agent catalog (AgentCatalog -> [AgentEntry]): manual registrations +
    // the last discovery scan. Zero-arg; the engine picker + Agents settings render the rows.
    [[nodiscard]] static QByteArray encodeAgentCatalogRequest();
    // Decode an AgentCatalog response into catalog rows (name/source/protocol/installed/version —
    // the launch recipe deliberately stays node-side).
    static bool decodeAgentCatalog(const QByteArray& responseCbor, QList<DecodedAgentEntry>* out);
    // Register a manual foreign agent (AgentRegister): the node forces source=Manual and re-probes,
    // so a caller-supplied `installed` is never trusted. Ok/Error reply (responseKind/decodeError).
    [[nodiscard]] static QByteArray encodeAgentRegisterRequest(const DecodedAgentRecipeInput& in);
    // Remove a manual foreign agent by catalog name (AgentRemove). Ok/Error reply.
    [[nodiscard]] static QByteArray encodeAgentRemoveRequest(const QString& name);

    // --- Profile distribution + history (PRO-7 / PRO-8) --------------------------------------
    [[nodiscard]] static QByteArray encodeProfileExportRequest(const QString& id);
    [[nodiscard]] static QByteArray encodeProfileImportRequest(const DecodedDistribution& dist,
                                                               const QString& newId = QString());
    // `after` (wire v25) resumes past the previous page's `next` cursor (the stringified seq of
    // the last served revision; empty = first page).
    [[nodiscard]] static QByteArray encodeProfileHistoryRequest(const QString& id,
                                                                const QString& after = QString());
    [[nodiscard]] static QByteArray encodeProfileAtRequest(const QString& id, quint64 seq);
    [[nodiscard]] static QByteArray encodeProfileRevertRequest(const QString& id, quint64 seq);
    // Decode a Distribution response (ProfileExport).
    static bool decodeDistribution(const QByteArray& responseCbor, DecodedDistribution* out);
    // Decode a Revisions page (ProfileHistory) into the revision list, newest last. `*next` (when
    // non-null) gets the resume cursor (cleared on the last page).
    static bool decodeRevisions(const QByteArray& responseCbor, QList<DecodedRevision>* out,
                                QString* next = nullptr);

    // --- Local model track (Phase 2) requests ------------------------------------------------
    // Engine/sort args are friendly wire strings: engine "llama"|"mistral_rs" (default "llama"),
    // sort "trending"|"downloads"|"likes"|"modified"|"created" (default "trending").
    // Step 1: search HF repos loadable by `engine` (ModelSearch -> SearchPage).
    [[nodiscard]] static QByteArray
    encodeModelSearchRequest(const QString& text, const QString& engine = QStringLiteral("llama"),
                             const QString& sort = QStringLiteral("trending"), quint32 page = 0,
                             quint32 limit = 25);
    // Step 2: list a repo's loadable files (ModelFiles -> model-file-page). Empty revision =
    // main. `after` (wire v25) resumes past the previous page's `next` cursor.
    [[nodiscard]] static QByteArray
    encodeModelFilesRequest(const QString& repo, const QString& engine = QStringLiteral("llama"),
                            const QString& revision = QString(), const QString& after = QString());
    // The hardware-aware quant recommendation for a repo (ModelRecommend -> QuantRecommendation).
    // hasBudget=false auto-detects VRAM/RAM node-side.
    [[nodiscard]] static QByteArray encodeModelRecommendRequest(
        const QString& repo, const QString& engine = QStringLiteral("llama"),
        bool hasBudget = false, quint64 budgetBytes = 0, const QString& revision = QString());
    // Start a download of one repo file (ModelDownload{ModelRef::Hf} -> ModelDownloadStarted).
    [[nodiscard]] static QByteArray
    encodeModelDownloadRequest(const QString& repo, const QString& file,
                               const QString& engine = QStringLiteral("llama"),
                               const QString& revision = QStringLiteral("main"));
    // Poll all download jobs (ModelDownloads -> [DownloadStatus]).
    [[nodiscard]] static QByteArray encodeModelDownloadsRequest();
    // The installed-model catalog (ModelCatalog -> [InstalledModel]).
    [[nodiscard]] static QByteArray encodeModelCatalogRequest();
    // Delete an installed model (ModelDelete -> Ok).
    [[nodiscard]] static QByteArray encodeModelDeleteRequest(const QString& id);
    // Activate an installed model for `profile` (empty = default local profile) -> Ok.
    [[nodiscard]] static QByteArray encodeModelActivateRequest(const QString& id,
                                                               const QString& profile = QString());
    // Download lifecycle controls (ModelCancel/Pause/Resume -> Ok). id is the DownloadStatus id.
    [[nodiscard]] static QByteArray encodeModelCancelRequest(quint64 id);
    [[nodiscard]] static QByteArray encodeModelPauseRequest(quint64 id);
    [[nodiscard]] static QByteArray encodeModelResumeRequest(quint64 id);

    // --- HITL (CHA-4 / CHA-5) ---
    // Resolve a parked Approval HostRequest (request_id from the stream) with allow/deny.
    // `allowPermanent` (wire v28) rides the inline `Approved{ approved, ? allow_permanent }` body:
    // when true the node adds the approved command's fingerprint to a per-session allow-list. Only
    // send it for a gate the node marked `allow_permanent_offered`; false leaves the field absent.
    // [wave2:app-approvals-safety] D3: `reason` (wire v29) rides `Approved.reason` — an operator
    // deny explanation threaded to the model on the next turn. Emitted only when non-empty (and
    // meaningful only on a deny); a plain allow/deny stays byte-identical to the pre-v29 shape.
    [[nodiscard]] static QByteArray encodeRespondApprovalRequest(const QString& sessionId,
                                                                 quint32 requestId, bool allow,
                                                                 bool allowPermanent = false,
                                                                 const QString& reason = QString());
    // Resolve a parked Input (clarify free-text) HostRequest.
    [[nodiscard]] static QByteArray
    encodeRespondInputRequest(const QString& sessionId, quint32 requestId, const QString& text);
    // Resolve a parked Choice (clarify) HostRequest with the chosen option index.
    [[nodiscard]] static QByteArray encodeRespondChoiceRequest(const QString& sessionId,
                                                               quint32 requestId, quint32 chosen);
    // Decide an inbox approval (PRO-11; request_id is the string id from ApprovalInfo).
    // `allowPermanent` (wire v28) rides the optional field: when an allow chose "allow permanently"
    // the node adds the approved command's fingerprint to a per-session allow-list. Only send it
    // for an approval the node marked as fingerprinted (DecodedApprovalInfo::hasFingerprint); false
    // leaves it absent.
    // [wave2:app-approvals-safety] D3: `reason` (wire v29) rides the optional
    // `ApprovalDecide.reason` — the operator deny explanation the node threads into the model's
    // next turn as the tool refusal text. Emitted only when non-empty (meaningful on a deny);
    // pre-v29 shape otherwise.
    [[nodiscard]] static QByteArray
    encodeApprovalDecideRequest(const QString& sessionId, const QString& requestId, bool allow,
                                bool allowPermanent = false, const QString& reason = QString());
    // Set a session's approval mode (CHA-4): "ask" | "accept_edits" | "auto_allow" | "deny".
    [[nodiscard]] static QByteArray encodeSetSessionModeRequest(const QString& sessionId,
                                                                const QString& mode);
    // List pending approvals (PRO-11). Empty session = across all sessions. `after` (wire v25)
    // resumes past the previous page's `next` cursor (empty = first page).
    [[nodiscard]] static QByteArray
    encodeApprovalsPendingRequest(const QString& sessionId = QString(),
                                  const QString& after = QString());

    // [wave2:app-approvals-safety] D2 tool inventory + D4 fingerprint management (wire v29).
    // ToolList is node-wide (no session arg). FingerprintList/Revoke are per-session (the node's
    // allow-list is per-session); revoke names the exact fingerprint hex to drop.
    [[nodiscard]] static QByteArray encodeToolListRequest();
    // [waveB:app-v30] D4: enable/disable a tool (ToolSetEnabled). The node owns gating; re-fetch
    // ToolList after to render the authoritative overlay result.
    [[nodiscard]] static QByteArray encodeToolSetEnabledRequest(const QString& tool, bool enabled);
    [[nodiscard]] static QByteArray encodeFingerprintListRequest(const QString& sessionId);
    [[nodiscard]] static QByteArray encodeFingerprintRevokeRequest(const QString& sessionId,
                                                                   const QString& fingerprint);

    // --- CHA-6 interrupt / steer ---
    [[nodiscard]] static QByteArray encodeSubmitInterruptRequest(const QString& sessionId,
                                                                 const QString& reason = QString());
    [[nodiscard]] static QByteArray
    encodeSubmitSteerRequest(const QString& sessionId, const QString& text, quint32 requestId = 1);

    // --- CHA-7 slash commands ---
    [[nodiscard]] static QByteArray encodeCommandListRequest();
    [[nodiscard]] static QByteArray
    encodeCommandInvokeRequest(const QString& name, const QString& args = QString(),
                               const QString& sessionId = QString());

    // --- CHA-8 session search ---
    [[nodiscard]] static QByteArray encodeSessionSearchRequest(const QString& query, quint32 limit);

    // --- Filesystem surface (Phase 4) --------------------------------------------------------
    // `rootId` is the opaque client string ("workspace" | "host:<id>" | "session:<id>").
    [[nodiscard]] static QByteArray encodeFsRootsRequest();
    // `after` (wire v24) resumes past the previous page's `next` cursor (empty = first page).
    [[nodiscard]] static QByteArray encodeFsListRequest(const QString& rootId, const QString& dir,
                                                        bool showIgnored = false,
                                                        const QString& after = QString());
    [[nodiscard]] static QByteArray encodeFsReadRequest(const QString& rootId, const QString& path,
                                                        quint64 maxBytes = 0);
    // `baseRevision` is the opaque "mtime:size" etag (empty = unconditional write); `force`
    // overrides a stale-revision conflict.
    [[nodiscard]] static QByteArray encodeFsWriteRequest(const QString& rootId, const QString& path,
                                                         const QByteArray& bytes,
                                                         const QString& baseRevision = QString(),
                                                         bool force = false);
    [[nodiscard]] static QByteArray encodeFsWatchPollRequest(const QString& rootId,
                                                             const QString& dir, quint64 afterSeq,
                                                             quint32 max = 0);
    [[nodiscard]] static QByteArray encodeFsSearchRequest(const QString& rootId,
                                                          const QString& query, bool regex = false,
                                                          bool caseSensitive = false,
                                                          quint32 maxResults = 0, quint32 page = 0);

    static bool decodeFsRoots(const QByteArray& responseCbor, QList<DecodedFsRoot>* out);
    // Decode one fs_list page (the uniform WirePage over fs-entry): the items plus the resume
    // cursor (`*next` is cleared on the last page). Pass a null `next` for single-shot uses.
    static bool decodeFsList(const QByteArray& responseCbor, QList<DecodedFsEntry>* out,
                             QString* next = nullptr);
    static bool decodeFsRead(const QByteArray& responseCbor, DecodedFsContent* out);
    // FsWrite returns the new revision as the opaque "mtime:size" etag.
    static bool decodeFsWrite(const QByteArray& responseCbor, QString* revision);
    static bool decodeFsWatch(const QByteArray& responseCbor, DecodedFsWatchPage* out);
    static bool decodeFsSearch(const QByteArray& responseCbor, DecodedFsSearchPage* out);

    // --- Fleet / subagent tree (Phase 5b: PRO-9 view + PRO-10 control) -----------------------
    // `after` (wire v25) resumes past the previous TreeReport's `next` cursor (empty = first
    // page); `root` rides every page.
    [[nodiscard]] static QByteArray encodeTreeRequest(const QString& after = QString());
    [[nodiscard]] static QByteArray encodeFleetRequest();
    [[nodiscard]] static QByteArray encodeUnitRequest(const QString& unitId);
    [[nodiscard]] static QByteArray encodeUnitEventsRequest(const QString& unitId, quint32 max);
    [[nodiscard]] static QByteArray encodePauseRequest(const QString& unitId);
    [[nodiscard]] static QByteArray encodeResumeRequest(const QString& unitId);
    [[nodiscard]] static QByteArray encodeScaleRequest(const QString& unitId, quint32 n);

    // Decode a Tree page (wire v25: `nodes` arrives in unit-id cursor pages) into the RAW node
    // list (no flatten — the caller accumulates pages and calls flattenTreeNodes at the end).
    // `outRoot` (optional) gets the root unit id (rides every page); `*next` (when non-null) gets
    // the resume cursor (cleared on the last page).
    static bool decodeTreeReport(const QByteArray& responseCbor, QList<DecodedUnitNode>* outFlat,
                                 QString* outRoot = nullptr, QString* next = nullptr);
    // Flatten an accumulated (multi-page) node union into the pre-order list with depth/parent
    // set (empty when `root` is empty — a fresh daemon's valid empty tree).
    static QList<DecodedUnitNode> flattenTreeNodes(const QList<DecodedUnitNode>& nodes,
                                                   const QString& root);
    // Decode a Unit response (Some/None). Sets *found=false on the null arm (unknown unit).
    static bool decodeUnit(const QByteArray& responseCbor, DecodedUnitNode* out, bool* found);
    static bool decodeFleetReport(const QByteArray& responseCbor, DecodedFleetReport* out);
    // Decode a UnitEvents response into the management-event views (the Subagent arm is the live
    // subagent strip's structured spawn/finish feed; other arms carry kind + seq only).
    static bool decodeUnitEvents(const QByteArray& responseCbor, QList<DecodedManageEvent>* out);

    // [wave2:app-delegation] F7/DEL-7: the payload-free Caps request + its CapsReport decode.
    [[nodiscard]] static QByteArray encodeCapsRequest();
    static bool decodeCaps(const QByteArray& responseCbor, DecodedCapsReport* out);

    // --- Channels / Events-IO read surface (story 04: EIO-1/3/8/9) ---------------------------
    [[nodiscard]] static QByteArray encodeTransportAdaptersRequest();
    [[nodiscard]] static QByteArray encodeTransportInstancesRequest();
    // [waveB:app-v30] D1: tear down a live transport session (TransportDisconnect) or fully remove
    // the account (TransportRemove — node-side sequences disconnect + conv close + routing unbind +
    // credential drop + config drop). One intent; the client renders the node's reported outcome.
    [[nodiscard]] static QByteArray encodeTransportDisconnectRequest(const QString& transport);
    [[nodiscard]] static QByteArray encodeTransportRemoveRequest(const QString& transport);
    // [acct-mgmt] wire v35: reversible transport lifecycle + persisted metadata (ControlApi-level;
    // available for every transport). Connect re-spawns the adapter family serve loop (idempotent).
    // SetEnabled persists desired state (disable also disconnects; enable does NOT auto-connect —
    // call Connect). SetLabel persists the display label; `hasLabel=false` emits an explicit null
    // to clear it. One intent; the client re-reads the node's reported state
    // (TransportChanged/refetch).
    [[nodiscard]] static QByteArray encodeTransportConnectRequest(const QString& transport);
    [[nodiscard]] static QByteArray encodeTransportSetEnabledRequest(const QString& transport,
                                                                     bool enabled);
    [[nodiscard]] static QByteArray
    encodeTransportSetLabelRequest(const QString& transport, bool hasLabel, const QString& label);
    // `after` (wire v25) resumes past the previous page's `next` cursor (empty = first page).
    [[nodiscard]] static QByteArray encodeConvListRequest(const QString& transport,
                                                          const QString& after = QString());
    static bool decodeAdapters(const QByteArray& responseCbor, QList<DecodedAdapterInfo>* out);
    static bool decodeTransportInstances(const QByteArray& responseCbor,
                                         QList<DecodedTransportInstance>* out);
    // Decode one conv-page (wire v25). `*next` (when non-null) gets the resume cursor (cleared on
    // the last page). [acct-mgmt] Now also decodes each conversation's optional `members`.
    static bool decodeConversations(const QByteArray& responseCbor, QList<DecodedConversation>* out,
                                    QString* next = nullptr);

    // --- [acct-mgmt] Room lifecycle + member management (wire v32) -----------------------------
    // Two-phase form ops: the node describes the form (ConvJoinDetails / ConvCreateDetails), the
    // app renders it, then sends the filled form (ConvJoin / ConvCreate). Single-verb ops
    // (leave/delete) name the target conversation. ConvGet hydrates one conversation incl. members.
    [[nodiscard]] static QByteArray encodeConvJoinDetailsRequest(const QString& transport);
    [[nodiscard]] static QByteArray encodeConvJoinRequest(const QString& transport,
                                                          const ConvJoinForm& form);
    [[nodiscard]] static QByteArray encodeConvCreateDetailsRequest(const QString& transport);
    [[nodiscard]] static QByteArray encodeConvCreateRequest(const QString& transport,
                                                            const ConvCreateForm& form);
    [[nodiscard]] static QByteArray encodeConvLeaveRequest(const QString& transport,
                                                           const QString& conv);
    [[nodiscard]] static QByteArray encodeConvDeleteRequest(const QString& transport,
                                                            const QString& conv);
    [[nodiscard]] static QByteArray encodeConvGetRequest(const QString& transport,
                                                         const QString& conv);
    // Membership verbs: `contactId` names the target member; `role` is a MemberRole wire string
    // (None|Voice|HalfOp|Op|Founder). Kick/Ban carry an optional reason (empty = absent).
    [[nodiscard]] static QByteArray encodeMemberInviteRequest(const QString& transport,
                                                              const QString& conv,
                                                              const QString& contactId);
    [[nodiscard]] static QByteArray encodeMemberRemoveRequest(const QString& transport,
                                                              const QString& conv,
                                                              const QString& contactId,
                                                              const QString& reason = QString());
    [[nodiscard]] static QByteArray encodeMemberBanRequest(const QString& transport,
                                                           const QString& conv,
                                                           const QString& contactId,
                                                           const QString& reason = QString());
    [[nodiscard]] static QByteArray encodeMemberSetRoleRequest(const QString& transport,
                                                               const QString& conv,
                                                               const QString& contactId,
                                                               const QString& role);
    // Decode the two details responses + the ConvGet response (Some/None -> *found).
    static bool decodeConvJoinDetails(const QByteArray& responseCbor,
                                      DecodedChannelJoinDetails* out);
    static bool decodeConvCreateDetails(const QByteArray& responseCbor,
                                        DecodedCreateConversationDetails* out);
    static bool decodeConversation(const QByteArray& responseCbor, DecodedConversation* out,
                                   bool* found);

    // --- [acct-mgmt] Transport contacts / roster (wire v34) -----------------------------------
    // The roster read is paged: RosterList carries the transport + an optional `after` cursor
    // (empty = first page); decodeContactPage returns one page + the resume cursor (cleared on the
    // last page), so the caller loops `next` up to kWirePageMax like every other page loop. The
    // mutations (RosterAdd/Update/Remove) carry a full contact-info and return Ok; add/remove need
    // only the id, update carries the editable fields. ContactGetProfile returns a node-rendered
    // profile string; ContactSetAlias sets/clears the local alias (empty = clear); DirectorySearch
    // returns an unpaged contact list for the people-picker (the node bounds it).
    [[nodiscard]] static QByteArray encodeRosterListRequest(const QString& transport,
                                                            const QString& after = QString());
    [[nodiscard]] static QByteArray encodeRosterAddRequest(const QString& transport,
                                                           const DecodedContact& contact);
    [[nodiscard]] static QByteArray encodeRosterUpdateRequest(const QString& transport,
                                                              const DecodedContact& contact);
    [[nodiscard]] static QByteArray encodeRosterRemoveRequest(const QString& transport,
                                                              const DecodedContact& contact);
    [[nodiscard]] static QByteArray encodeContactGetProfileRequest(const QString& transport,
                                                                   const QString& contactId);
    // `hasAlias` false clears the alias (the optional is absent); true sets it to `alias`.
    [[nodiscard]] static QByteArray encodeContactSetAliasRequest(const QString& transport,
                                                                 const QString& contactId,
                                                                 bool hasAlias,
                                                                 const QString& alias);
    [[nodiscard]] static QByteArray encodeDirectorySearchRequest(const QString& transport,
                                                                 const QString& query = QString());
    // Decode a ContactPage (`*next` gets the resume cursor, cleared on the last page), the unpaged
    // DirectorySearch Contacts list, and the ContactProfile string.
    static bool decodeContactPage(const QByteArray& responseCbor, QList<DecodedContact>* out,
                                  QString* next = nullptr);
    static bool decodeContacts(const QByteArray& responseCbor, QList<DecodedContact>* out);
    static bool decodeContactProfile(const QByteArray& responseCbor, QString* out);

    // --- Multiplexed socket envelope (wire L0; daemon-sync-protocol-spec.md §2) ---
    // The envelope is hand-coded (not zcbor-generated): it wraps the already-encoded request bytes
    // and slices the response bytes back out, so the generated codec stays scoped to
    // api-request/api-response. Canonical CBOR (definite-length maps; "id" before "req"/"res").
    // The wire version this client speaks (negotiated by the Hello handshake). v2 adds the
    // SASL-style authentication exchange (AuthStart/AuthStep/AuthResume -> AuthChallenge/AuthOk/
    // AuthError) and the server Hello's auth_mechanisms list.
    static constexpr quint32 kWireVersion = 2;
    // The daemon-api CONTRACT version this client implements (daemon-node
    // `daemon_common::WireVersion::CURRENT` at the time this hand-rolled codec and the vendored
    // `codec/generated` were last regenerated) - distinct from kWireVersion (the envelope). This
    // is the ONLY app-side copy: bump it together with `just update-codec` whenever the node
    // contract version moves. The server advertises its own version as the "api/<N>" Hello
    // feature; the connection service compares the two at connect and replaces (app-managed) or
    // refuses (attach) a mismatched daemon instead of silently serving stale wire shapes.
    static constexpr quint32 kDaemonApiVersion = 35;
    // The wire page bound (daemon-api WIRE_PAGE_MAX): a paged response carries at most this many
    // array elements per page — the generated codec decodes into fixed 64-element buffers — so
    // clients loop on the page cursors instead of ever asking for more per response.
    static constexpr quint32 kWirePageMax = 64;
    // The client->server Hello opening the multiplexed/streaming session.
    [[nodiscard]] static QByteArray encodeHelloFrame();
    // Begin a SASL exchange with `mechanism`, carrying its initial client response (may be empty).
    // The auth byte payload encodes as a CBOR array of uints (`[* uint]`), per the frozen contract.
    [[nodiscard]] static QByteArray encodeAuthStartFrame(const QString& mechanism,
                                                         const QByteArray& initial);
    // A subsequent client response in a multi-step mechanism (answers a prior AuthChallenge).
    [[nodiscard]] static QByteArray encodeAuthStepFrame(const QByteArray& data);
    // Re-authenticate by presenting a previously issued opaque session token (reconnect fast-path).
    [[nodiscard]] static QByteArray encodeAuthResumeFrame(const QString& token);
    // Wrap an already-encoded ApiRequest as a one-shot Call with correlation `id`.
    [[nodiscard]] static QByteArray encodeCallFrame(quint64 id, const QByteArray& requestCbor);
    // Wrap an already-encoded streaming ApiRequest (e.g. Subscribe) as an Open with stream `id`.
    [[nodiscard]] static QByteArray encodeOpenFrame(quint64 id, const QByteArray& requestCbor);
    // Tear down a streaming exchange.
    [[nodiscard]] static QByteArray encodeCancelFrame(quint64 id);
    // Decode one server->client frame, slicing the inner ApiResponse bytes for Reply/Item.
    static bool decodeWireFrame(const QByteArray& frameCbor, DecodedWireFrame* out);

    // Response inspection / decode.
    [[nodiscard]] static ApiResponseKind responseKind(const QByteArray& responseCbor);
    static bool decodeHealth(const QByteArray& responseCbor, DecodedHealth* out);
    // Decode a SessionPage. `rev` (L4) is the roster revision this page reflects (persist + pass
    // back as since_rev); `removed` (L4 delta reads) is the ids the client should prune from its
    // cache.
    static bool decodeSessionPage(const QByteArray& responseCbor, QList<CachedSessionRow>* out,
                                  QString* nextCursor = nullptr, quint64* rev = nullptr,
                                  QStringList* removed = nullptr);
    // Decode a SessionDetail response (SessionGet). Sets *found=false on the null arm (unknown
    // session). CHA-9 follow-on: hydrates the per-session facets the roster page omits.
    static bool decodeSessionDetail(const QByteArray& responseCbor, DecodedSessionDetail* out,
                                    bool* found);
    // Decode a LogPage into cache rows tagged with sessionId (seq/direction/disposition + the
    // next/head cursors). The full payload/origin now generate from the unified contract; use
    // decodeLogPageEntries() when the typed payload (AgentEvent) is needed (transcript rendering).
    //
    // NOTE: CachedLogRow.payloadCbor is still left empty here - zcbor does not retain raw per-entry
    // bytes and there is no standalone session-payload encoder to re-encode with, so durable
    // payload persistence routes through decodeLogPageEntries() typed events instead (see Phase 3).
    static bool decodeLogPage(const QByteArray& responseCbor, const QString& sessionId,
                              QList<CachedLogRow>* out, quint64* nextSeq = nullptr,
                              quint64* headSeq = nullptr);
    // Decode a LogPage into typed entries including the session-payload (AgentEvent when the arm is
    // Event). This is the transcript-rendering path (CHA-2): the turn engine appends TextDeltas and
    // ends on TurnFinished. nextSeq/headSeq advance the subscribe cursor.
    static bool decodeLogPageEntries(const QByteArray& responseCbor, QList<DecodedLogEntry>* out,
                                     quint64* nextSeq = nullptr, quint64* headSeq = nullptr,
                                     quint64* epoch = nullptr);
    // Decode an EventsPage response (EventsSince; L3) into the node-wide notifications + the feed
    // cursors. The SubscriptionManager routes each event by kind and advances `nextCursor`.
    static bool decodeEventsPage(const QByteArray& responseCbor, DecodedEventsPage* out);
    // Decode a Journal response (SessionHistory) into durable records + the resync cursors. The L2
    // re-baseline path replays these coalesced blocks; `sealedAfter` (when present) marks a rewind
    // boundary the client truncates its rendered transcript to before applying the tail.
    static bool decodeJournal(const QByteArray& responseCbor, QList<DecodedJournalRecord>* out,
                              quint64* nextCursor = nullptr, quint64* headCursor = nullptr,
                              bool* hasSealedAfter = nullptr, quint64* sealedAfter = nullptr);
    // Decode a Drained response (Poll) into the outbound agent events (Event arms only; host
    // requests are skipped here). Used by the harness/diagnostic poll path.
    static bool decodeDrained(const QByteArray& responseCbor, QList<DecodedAgentEvent>* out);

    // Decode a Credentials response into redacted entries.
    static bool decodeCredentials(const QByteArray& responseCbor,
                                  QList<DecodedCredentialInfo>* out);
    // Decode one Models page (wire v25 model-page) into the discoverable model list. `*next`
    // (when non-null) gets the resume cursor (cleared on the last page).
    static bool decodeModels(const QByteArray& responseCbor, QList<DecodedModelDescriptor>* out,
                             QString* next = nullptr);
    // Decode a ProviderCatalog response into the offered-provider list.
    static bool decodeProviderCatalog(const QByteArray& responseCbor,
                                      QList<DecodedProviderDescriptor>* out);
    // Decode one ProviderModels page (same model-page shape as Models). `*next` (when non-null)
    // gets the resume cursor (cleared on the last page).
    static bool decodeProviderModels(const QByteArray& responseCbor,
                                     QList<DecodedModelDescriptor>* out, QString* next = nullptr);
    // Decode a ModelCurrent response. Sets *hasModel=false when the daemon resolves no model
    // (null).
    static bool decodeModelCurrent(const QByteArray& responseCbor, DecodedModelDescriptor* out,
                                   bool* hasModel);
    // Decode an Error response (now that api-error is in the unified contract) so a service can map
    // a failed CredentialSet/SetSessionModel to an actionable message rather than a raw decode
    // miss.
    static bool decodeError(const QByteArray& responseCbor, DecodedApiError* out);
    // Decode a Profiles response into profile listing entries.
    static bool decodeProfiles(const QByteArray& responseCbor, QList<DecodedProfileInfo>* out);
    // Decode a Profile response (ProfileGet) into a full spec. Sets *found=false on the null arm
    // (unknown id). PRO-3.
    static bool decodeProfile(const QByteArray& responseCbor, DecodedProfileSpec* out, bool* found);
    // Decode a ProfileId response (ProfileCreate/Clone) into the new profile id.
    static bool decodeProfileId(const QByteArray& responseCbor, QString* outId);

    // --- Local model track (Phase 2) decoders ------------------------------------------------
    // Decode a ModelSearch response (SearchPage) into repo hits. `hasMore`/`page` advance paging.
    static bool decodeModelSearch(const QByteArray& responseCbor, QList<DecodedSearchHit>* out,
                                  bool* hasMore = nullptr, quint32* page = nullptr);
    // Decode one ModelFiles page (wire v25 model-file-page) into the repo's loadable files.
    // `*next` (when non-null) gets the resume cursor (cleared on the last page).
    static bool decodeModelFiles(const QByteArray& responseCbor, QList<DecodedModelFile>* out,
                                 QString* next = nullptr);
    // Decode a ModelDownloadStarted response into the new download job id.
    static bool decodeModelDownloadStarted(const QByteArray& responseCbor, quint64* outId);
    // Decode a ModelDownloads response into the in-flight/finished job snapshots.
    static bool decodeModelDownloads(const QByteArray& responseCbor,
                                     QList<DecodedDownloadStatus>* out);
    // Decode a ModelCatalog response into the installed-model list.
    static bool decodeModelCatalog(const QByteArray& responseCbor,
                                   QList<DecodedInstalledModel>* out);
    // Decode a ModelRecommend response into the hardware-aware quant recommendation.
    static bool decodeModelRecommend(const QByteArray& responseCbor,
                                     DecodedQuantRecommendation* out);
    // Decode one Approvals page (PRO-11; wire v25 approval-page) into pending entries. `*next`
    // (when non-null) gets the resume cursor (cleared on the last page).
    static bool decodeApprovals(const QByteArray& responseCbor, QList<DecodedApprovalInfo>* out,
                                QString* next = nullptr);
    // [wave2:app-approvals-safety] D2/D4 (wire v29): decode a Tools response into the node-wide
    // tool inventory, and a Fingerprints response into a session's remembered allow-list.
    static bool decodeTools(const QByteArray& responseCbor, QList<DecodedToolInfo>* out);
    static bool decodeFingerprints(const QByteArray& responseCbor,
                                   QList<DecodedRememberedFingerprint>* out);
    // Decode a Commands response (CHA-7) into the slash-command catalog.
    static bool decodeCommands(const QByteArray& responseCbor, QList<DecodedCommandSpec>* out);
    // Decode a CommandOutput response (CHA-7) into its text.
    static bool decodeCommandOutput(const QByteArray& responseCbor, QString* outText);
    // Decode a SessionSearch response (CHA-8) into hits.
    static bool decodeSessionSearch(const QByteArray& responseCbor,
                                    QList<DecodedSessionSearchHit>* out);

    // --- app-wizard-auth stream (appended as one block for sibling-merge friendliness) --------
    //
    // Interactive auth (begin -> browser -> complete; daemon-interactive-auth-spec.md). The op
    // name strings live ONLY here + in AuthRepository so the announced Acp*/Auth* wire renames
    // stay mechanical.
    // Discover which families support interactive auth (AuthProviders -> [AuthProviderInfo]).
    [[nodiscard]] static QByteArray encodeAuthProvidersRequest();
    // Begin a flow: the daemon mints an authorization URL against the CLIENT-owned `redirectUri`
    // and parks the pending flow. `params` is the family-specific string map (Matrix SSO:
    // {homeserver}). A non-empty `bindProfile` asks the node to bind the resulting account to
    // that profile on success (`bindCredentialRef` optionally names the CredentialStore key).
    [[nodiscard]] static QByteArray encodeAuthBeginRequest(const QString& family,
                                                           const QVariantMap& params,
                                                           const QString& redirectUri,
                                                           const QString& bindProfile = QString(),
                                                           const QString& bindCredentialRef = {});
    // [waveB:app-v31] Advance a parked flow one step (AuthStep): feed the AuthStepInput collected
    // for the current challenge. `kind` selects the arm — Fields carries `fields` (the filled
    // key->value map answering a Form), Callback carries `callback` (the captured redirect
    // URL/query answering a Redirect), Poll carries nothing (answering a Qr/Message "landed yet?").
    // The reply is an AuthStepped (decodeAuthStepped): the next challenge, or completion.
    [[nodiscard]] static QByteArray encodeAuthStepRequest(const QString& flowId,
                                                          AuthStepInputKind kind,
                                                          const QVariantMap& fields = {},
                                                          const QString& callback = {});
    // Finish a flow from the captured redirect (full URL or query string). Single-use flow_id. The
    // node keeps AuthComplete as a compat wrapper over AuthStep(Callback); the app drives AuthStep.
    [[nodiscard]] static QByteArray encodeAuthCompleteRequest(const QString& flowId,
                                                              const QString& callback);
    // Drop a pending flow early (user cancelled). Idempotent node-side.
    [[nodiscard]] static QByteArray encodeAuthCancelRequest(const QString& flowId);
    static bool decodeAuthProviders(const QByteArray& responseCbor,
                                    QList<DecodedAuthProviderInfo>* out);
    static bool decodeAuthBegun(const QByteArray& responseCbor, DecodedAuthBeginResponse* out);
    // [waveB:app-v31] Decode an AuthStepped (AuthStepResult): sets out->completed and populates
    // either out->challenge (the next challenge) or out->completion (the finished outcome).
    static bool decodeAuthStepped(const QByteArray& responseCbor, DecodedAuthStepResult* out);
    static bool decodeAuthCompleted(const QByteArray& responseCbor,
                                    DecodedAuthCompleteResponse* out);

    // Foreign-agent discovery (a fresh PATH/endpoint scan; the response reuses the AgentCatalog
    // shape, so decodeAgentCatalog reads it). Zero-arg.
    [[nodiscard]] static QByteArray encodeAgentDiscoverRequest();

    // Local re-quantization (A5): start a quantize job for `repo` toward `targetQuant`
    // (ModelQuantize -> ModelQuantizeStarted). Empty `sourceFile` lets the node pick the best
    // installed source; empty `revision` = main.
    [[nodiscard]] static QByteArray encodeModelQuantizeRequest(const QString& repo,
                                                               const QString& targetQuant,
                                                               const QString& sourceFile = {},
                                                               const QString& revision = {});
    // Poll all quantize jobs (ModelQuantizes -> [QuantizeStatus]).
    [[nodiscard]] static QByteArray encodeModelQuantizesRequest();
    static bool decodeModelQuantizeStarted(const QByteArray& responseCbor, quint64* outId);
    static bool decodeModelQuantizes(const QByteArray& responseCbor,
                                     QList<DecodedQuantizeStatus>* out);

    // GGUF introspection (A6 ghost probe): read an installed model's on-disk metadata
    // (ModelInspect -> GgufInfo). An Error reply for a CATALOGED id is the ghost signal (the
    // registry keeps records for deleted files).
    [[nodiscard]] static QByteArray encodeModelInspectRequest(const QString& id);
    static bool decodeModelInspect(const QByteArray& responseCbor, DecodedGgufInfo* out);

    // --- Checkpoints (E4/TOOL-9) --------------------------------------------------------------
    // List a session's durable checkpoints (CheckpointList -> Checkpoints). Empty session = every
    // session; `after` (wire v25) resumes past the previous page's `next` cursor.
    [[nodiscard]] static QByteArray
    encodeCheckpointListRequest(const QString& sessionId = QString(),
                                const QString& after = QString());
    // Rewind `sessionId` to `checkpointId` (CheckpointRewind -> Ok). DISTINCT from the turn-level
    // Rewind/RewindTo ops: this restores the node's durable tool-event checkpoint.
    [[nodiscard]] static QByteArray encodeCheckpointRewindRequest(const QString& sessionId,
                                                                  const QString& checkpointId);
    // Decode one Checkpoints page. `*next` (when non-null) gets the resume cursor (cleared on the
    // last page).
    static bool decodeCheckpoints(const QByteArray& responseCbor, QList<DecodedCheckpointInfo>* out,
                                  QString* next = nullptr);

    // --- Routing (B6/ROU: origin->session pins; wire v28) --------------------------------------
    // List the explicit chat pins (RoutingListChats -> ChatRoutes). `after` (wire v25) resumes
    // past the previous page's `next` cursor.
    [[nodiscard]] static QByteArray encodeRoutingListChatsRequest(const QString& after = QString());
    // Resolve one origin's pin (RoutingGet -> ChatRoute(opt)).
    [[nodiscard]] static QByteArray encodeRoutingGetRequest(const DecodedOrigin& origin);
    // Set a full route (RoutingSet{chat_route} -> Ok): origin -> session (+profile/isolation).
    [[nodiscard]] static QByteArray encodeRoutingSetRequest(const DecodedChatRoute& route);
    // Pin an origin to a session (+ optional agent) (RoutingBindChat -> Ok).
    [[nodiscard]] static QByteArray encodeRoutingBindChatRequest(const DecodedOrigin& origin,
                                                                 const QString& session,
                                                                 const QString& profile = {});
    // Remove an origin's pin (RoutingUnbindChat -> Ok).
    [[nodiscard]] static QByteArray encodeRoutingUnbindChatRequest(const DecodedOrigin& origin);
    // List a transport instance's bindable rooms (TransportRooms -> Rooms). `after` resumes.
    [[nodiscard]] static QByteArray encodeTransportRoomsRequest(const QString& transport,
                                                                const QString& after = QString());
    // Decode one ChatRoutes page. `*next` (when non-null) gets the resume cursor.
    static bool decodeChatRoutes(const QByteArray& responseCbor, QList<DecodedChatRoute>* out,
                                 QString* next = nullptr);
    // Decode a ChatRoute response (RoutingGet). Sets *found=false on the null arm (no pin).
    static bool decodeChatRoute(const QByteArray& responseCbor, DecodedChatRoute* out, bool* found);
    // Decode one Rooms page. `*next` (when non-null) gets the resume cursor.
    static bool decodeRooms(const QByteArray& responseCbor, QList<DecodedRoomInfo>* out,
                            QString* next = nullptr);

    // --- User feedback over OpenTelemetry (wire v32) -------------------------------------------
    // Submit thumbs up/down + optional comment on an agent response, or general app feedback
    // (FeedbackSubmit -> FeedbackAck). Explicit feedback is per-event consent: the node accepts +
    // queues it even when the global telemetry toggle is off. The rated response content (when the
    // submitter consented via `includeContent`) is read node-side from the journal at `cursor`.
    [[nodiscard]] static QByteArray encodeFeedbackSubmitRequest(const FeedbackSubmitInput& in);
    // Read / set the node-owned global telemetry consent (TelemetryConsentGet/Set ->
    // TelemetryConsent{enabled}). Default OFF (opt-in); passive telemetry is gated by it, explicit
    // feedback is not.
    [[nodiscard]] static QByteArray encodeTelemetryConsentGetRequest();
    [[nodiscard]] static QByteArray encodeTelemetryConsentSetRequest(bool enabled);
    // Decode a FeedbackAck (accepted+queued to the durable outbox; delivery is a separate drain).
    static bool decodeFeedbackAck(const QByteArray& responseCbor, bool* accepted, bool* queued);
    // Decode a TelemetryConsent response into the node's current consent state.
    static bool decodeTelemetryConsent(const QByteArray& responseCbor, bool* enabled);

    // --- Node gateway (wire v32) --------------------------------------------------------------
    // Read the gateway's live status (GatewayGet -> GatewayStatus). Zero-arg.
    [[nodiscard]] static QByteArray encodeGatewayGetRequest();
    // Toggle the gateway on/off (GatewaySet{enabled, ? addr} -> GatewayStatus). An empty `addr`
    // omits the optional bind address (the node keeps/derives its own); a non-empty `addr` sets it.
    [[nodiscard]] static QByteArray encodeGatewaySetRequest(bool enabled,
                                                            const QString& addr = QString());
    // Decode a GatewayStatus response into the live gateway state.
    static bool decodeGatewayStatus(const QByteArray& responseCbor, DecodedGatewayStatus* out);
};

} // namespace daemonapp::daemon

Q_DECLARE_METATYPE(daemonapp::daemon::DecodedPrincipalView)
