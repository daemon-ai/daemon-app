#pragma once

#include "daemon/daemon_cache_store.h"

#include <QByteArray>
#include <QList>
#include <QString>

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

enum class ApiResponseKind {
    Unknown,
    Health,
    SessionPage,
    LogPage,
    FsRoots,
    Ok,
    Credentials,
    Models,
    ModelCurrent,
    Profiles,
    Drained,
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
    // SessionsQuery with an empty query map (daemon applies its default scope).
    [[nodiscard]] static QByteArray encodeSessionsQueryRequest();
    // Subscribe to a session's merged log from afterSeq (exclusive), up to max entries.
    [[nodiscard]] static QByteArray encodeSubscribeRequest(const QString& sessionId,
                                                           quint64 afterSeq, quint32 max);

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

    // Response inspection / decode.
    [[nodiscard]] static ApiResponseKind responseKind(const QByteArray& responseCbor);
    static bool decodeHealth(const QByteArray& responseCbor, DecodedHealth* out);
    static bool decodeSessionPage(const QByteArray& responseCbor, QList<CachedSessionRow>* out,
                                  QString* nextCursor = nullptr);
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
                                     quint64* nextSeq = nullptr, quint64* headSeq = nullptr);
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
};

} // namespace daemonapp::daemon
