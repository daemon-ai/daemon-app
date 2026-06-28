#pragma once

#include "daemon/daemon_cache_store.h"

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

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
};

// A discoverable model (Models / ModelCurrent). Optional pricing/context carry a `has*` flag since
// the wire models them as `(uint / null)`.
struct DecodedModelDescriptor {
    QString id;
    QString provider; // ProviderSelector wire string: "mock" | "genai" | "llama_cpp" | "mistral_rs"
    bool hasContextLength = false;
    quint32 contextLength = 0;
    bool hasInputPrice = false;
    quint64 inputPriceMicrosPerMtok = 0;
    bool hasOutputPrice = false;
    quint64 outputPriceMicrosPerMtok = 0;
    bool local = false;
};

// A decoded error envelope (ApiResponse::Error): the variant kind + its human-readable message.
struct DecodedApiError {
    QString kind; // "UnknownSession" | "Unsupported" | "Conflict" | "Other"
    QString message;
};

// A profile listing entry (ProfileList -> Profiles). `isActive` marks the node's active default,
// which is the profile new sessions bind to (so onboarding stores the credential under it).
struct DecodedProfileInfo {
    QString id;
    QString provider;
    QString model;
    bool isActive = false;
};

// A transport-instance account bound to a profile (names only, never secrets).
struct DecodedBoundAccount {
    QString transportInstance;
    QString credentialRef;
};

// The full ProfileSpec (concrete wire profile-spec). Carries every field so a ProfileGet ->
// edit -> ProfileUpdate round-trip preserves the fields the editor does not touch. Nullable
// fields use a `has*` flag (Option::None on the wire); enum-ish fields are wire strings.
struct DecodedProfileSpec {
    QString id;
    QString provider = QStringLiteral("genai"); // "mock"|"genai"|"llama_cpp"|"mistral_rs"
    QString model;
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
    quint32 inputTokens = 0;    // Usage delta (CHA-3)
    quint32 outputTokens = 0;   // Usage delta (CHA-3)
    quint32 costMicros = 0;     // Usage delta (CHA-3)
    quint32 contextUsed = 0;    // Context status used_tokens (CHA-3)
    bool hasContextMax = false; // Context status max_tokens present
    quint32 contextMax = 0;     // Context status max_tokens (CHA-3)
    QString endReason;          // TurnFinished: Completed | Failed | Interrupted | ...
    QString finalText;          // TurnFinished: the turn's final assistant text (if any)
    bool turnCompleted = false; // TurnFinished with end_reason == Completed
};

// One decoded session-log entry (Subscribe -> LogPage). The transcript engine consumes the
// `event` when payloadKind == Event; other payload kinds (Command echoes, Meta, etc.) are surfaced
// so callers can advance the cursor without rendering them.
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
    // Valid when payloadKind == Request (a parked HostRequest the turn is blocked on, HITL).
    quint32 hostRequestId = 0;
    QString hostKind; // "Approval" | "Input" | "Choice" | "Delegate" | "Spawn"
    QString hostPrompt;
    QStringList hostOptions; // Choice options
};

// A pending approval (ApprovalsPending -> Approvals). request_id is a STRING here (the aggregate
// inbox), distinct from the in-stream HostRequest.request_id (uint).
struct DecodedApprovalInfo {
    QString session;
    QString requestId;
    QString prompt;
    bool hasPath = false;
    QString path;
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
};

struct DecodedWireFrame {
    WireFrameKind kind = WireFrameKind::Unknown;
    quint64 id = 0;        // the Call id this frame answers (0 for Hello)
    QByteArray payload;    // inner ApiResponse CBOR for Reply/Item; empty otherwise
    bool hasError = false; // End closed with an error
    quint64 epoch = 0;     // Reset (L2)
    quint64 headSeq = 0;   // Reset (L2)
    QStringList features;  // Hello: the server's advertised capability strings (mux/stream/...)
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
    QString summary;       // ToolResult summary
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
        ResyncNeeded
    } kind = Kind::Unknown;
    QString session;        // SessionAdvanced / SessionMetaChanged / ApprovalPending
    quint64 epoch = 0;      // SessionAdvanced
    quint64 headSeq = 0;    // SessionAdvanced
    quint64 rev = 0;        // SessionMetaChanged / RosterChanged
    QString requestId;      // ApprovalPending
    quint64 downloadId = 0; // DownloadProgress
    quint32 pct = 0;        // DownloadProgress
    QString state;          // DownloadProgress
    QString scope;          // ResyncNeeded
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
};

// The top-level fleet roster (Fleet -> FleetReport): the root unit ids (usage omitted from the
// facade).
struct DecodedFleetReport {
    QStringList children;
};

enum class ApiResponseKind {
    Unknown,
    Health,
    SessionPage,
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
    Profile,
    Distribution,
    Revisions,
    Ok,
    Credentials,
    Models,
    ModelCurrent,
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
    // a full page (back-compat / cold cache).
    [[nodiscard]] static QByteArray encodeSessionsQueryRequest(bool hasSinceRev = false,
                                                               quint64 sinceRev = 0);
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

    // Onboarding (CON-4 / CON-6): credentials + model discovery/selection.
    // Store a provider secret under `profile` (CredentialSet -> Ok). The secret never returns.
    [[nodiscard]] static QByteArray encodeCredentialSetRequest(const QString& profile,
                                                               const QString& secret);
    // List redacted credentials (CredentialList -> Credentials).
    [[nodiscard]] static QByteArray encodeCredentialListRequest();
    // Remove the secret stored under `profile` (CredentialRemove -> Ok).
    [[nodiscard]] static QByteArray encodeCredentialRemoveRequest(const QString& profile);
    // Discover models (Models -> Models([ModelDescriptor])).
    [[nodiscard]] static QByteArray encodeModelsRequest();
    // The current model for `profile` (empty = the active default profile) -> ModelCurrent(opt).
    [[nodiscard]] static QByteArray encodeModelCurrentRequest(const QString& profile = QString());
    // Set the model (and optionally re-bind the provider) for a live session -> Ok.
    [[nodiscard]] static QByteArray
    encodeSetSessionModelRequest(const QString& sessionId, const QString& model,
                                 const QString& provider = QString());
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

    // --- Profile distribution + history (PRO-7 / PRO-8) --------------------------------------
    [[nodiscard]] static QByteArray encodeProfileExportRequest(const QString& id);
    [[nodiscard]] static QByteArray encodeProfileImportRequest(const DecodedDistribution& dist,
                                                               const QString& newId = QString());
    [[nodiscard]] static QByteArray encodeProfileHistoryRequest(const QString& id);
    [[nodiscard]] static QByteArray encodeProfileAtRequest(const QString& id, quint64 seq);
    [[nodiscard]] static QByteArray encodeProfileRevertRequest(const QString& id, quint64 seq);
    // Decode a Distribution response (ProfileExport).
    static bool decodeDistribution(const QByteArray& responseCbor, DecodedDistribution* out);
    // Decode a Revisions response (ProfileHistory) into the revision list, newest last.
    static bool decodeRevisions(const QByteArray& responseCbor, QList<DecodedRevision>* out);

    // --- Local model track (Phase 2) requests ------------------------------------------------
    // Engine/sort args are friendly wire strings: engine "llama"|"mistral_rs" (default "llama"),
    // sort "trending"|"downloads"|"likes"|"modified"|"created" (default "trending").
    // Step 1: search HF repos loadable by `engine` (ModelSearch -> SearchPage).
    [[nodiscard]] static QByteArray
    encodeModelSearchRequest(const QString& text, const QString& engine = QStringLiteral("llama"),
                             const QString& sort = QStringLiteral("trending"), quint32 page = 0,
                             quint32 limit = 25);
    // Step 2: list a repo's loadable files (ModelFiles -> [ModelFile]). Empty revision = main.
    [[nodiscard]] static QByteArray
    encodeModelFilesRequest(const QString& repo, const QString& engine = QStringLiteral("llama"),
                            const QString& revision = QString());
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
    [[nodiscard]] static QByteArray encodeRespondApprovalRequest(const QString& sessionId,
                                                                 quint32 requestId, bool allow);
    // Resolve a parked Input (clarify free-text) HostRequest.
    [[nodiscard]] static QByteArray
    encodeRespondInputRequest(const QString& sessionId, quint32 requestId, const QString& text);
    // Resolve a parked Choice (clarify) HostRequest with the chosen option index.
    [[nodiscard]] static QByteArray encodeRespondChoiceRequest(const QString& sessionId,
                                                               quint32 requestId, quint32 chosen);
    // Decide an inbox approval (PRO-11; request_id is the string id from ApprovalInfo).
    [[nodiscard]] static QByteArray
    encodeApprovalDecideRequest(const QString& sessionId, const QString& requestId, bool allow);
    // Set a session's approval mode (CHA-4): "ask" | "accept_edits" | "auto_allow" | "deny".
    [[nodiscard]] static QByteArray encodeSetSessionModeRequest(const QString& sessionId,
                                                                const QString& mode);
    // List pending approvals (PRO-11). Empty session = across all sessions.
    [[nodiscard]] static QByteArray
    encodeApprovalsPendingRequest(const QString& sessionId = QString());

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
    [[nodiscard]] static QByteArray encodeFsListRequest(const QString& rootId, const QString& dir,
                                                        bool showIgnored = false);
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
    static bool decodeFsList(const QByteArray& responseCbor, QList<DecodedFsEntry>* out);
    static bool decodeFsRead(const QByteArray& responseCbor, DecodedFsContent* out);
    // FsWrite returns the new revision as the opaque "mtime:size" etag.
    static bool decodeFsWrite(const QByteArray& responseCbor, QString* revision);
    static bool decodeFsWatch(const QByteArray& responseCbor, DecodedFsWatchPage* out);
    static bool decodeFsSearch(const QByteArray& responseCbor, DecodedFsSearchPage* out);

    // --- Fleet / subagent tree (Phase 5b: PRO-9 view + PRO-10 control) -----------------------
    [[nodiscard]] static QByteArray encodeTreeRequest();
    [[nodiscard]] static QByteArray encodeFleetRequest();
    [[nodiscard]] static QByteArray encodeUnitRequest(const QString& unitId);
    [[nodiscard]] static QByteArray encodeUnitEventsRequest(const QString& unitId, quint32 max);
    [[nodiscard]] static QByteArray encodePauseRequest(const QString& unitId);
    [[nodiscard]] static QByteArray encodeResumeRequest(const QString& unitId);
    [[nodiscard]] static QByteArray encodeScaleRequest(const QString& unitId, quint32 n);

    // Decode a Tree response, flattening root + nodes into a pre-order list with depth set (empty
    // when the tree has no root). `outRoot` (optional) gets the root unit id.
    static bool decodeTreeReport(const QByteArray& responseCbor, QList<DecodedUnitNode>* outFlat,
                                 QString* outRoot = nullptr);
    // Decode a Unit response (Some/None). Sets *found=false on the null arm (unknown unit).
    static bool decodeUnit(const QByteArray& responseCbor, DecodedUnitNode* out, bool* found);
    static bool decodeFleetReport(const QByteArray& responseCbor, DecodedFleetReport* out);

    // --- Multiplexed socket envelope (wire L0; daemon-sync-protocol-spec.md §2) ---
    // The envelope is hand-coded (not zcbor-generated): it wraps the already-encoded request bytes
    // and slices the response bytes back out, so the generated codec stays scoped to
    // api-request/api-response. Canonical CBOR (definite-length maps; "id" before "req"/"res").
    // The wire version this client speaks (negotiated by the Hello handshake).
    static constexpr quint32 kWireVersion = 1;
    // The client->server Hello opening the multiplexed/streaming session.
    [[nodiscard]] static QByteArray encodeHelloFrame();
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
    // Decode a Models response into the discoverable model list.
    static bool decodeModels(const QByteArray& responseCbor, QList<DecodedModelDescriptor>* out);
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
    // Decode a ModelFiles response into the repo's loadable files.
    static bool decodeModelFiles(const QByteArray& responseCbor, QList<DecodedModelFile>* out);
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
    // Decode an Approvals response (PRO-11) into pending entries.
    static bool decodeApprovals(const QByteArray& responseCbor, QList<DecodedApprovalInfo>* out);
    // Decode a Commands response (CHA-7) into the slash-command catalog.
    static bool decodeCommands(const QByteArray& responseCbor, QList<DecodedCommandSpec>* out);
    // Decode a CommandOutput response (CHA-7) into its text.
    static bool decodeCommandOutput(const QByteArray& responseCbor, QString* outText);
    // Decode a SessionSearch response (CHA-8) into hits.
    static bool decodeSessionSearch(const QByteArray& responseCbor,
                                    QList<DecodedSessionSearchHit>* out);
};

} // namespace daemonapp::daemon
