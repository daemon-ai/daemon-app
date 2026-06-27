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
    Error,
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

    // Response inspection / decode.
    [[nodiscard]] static ApiResponseKind responseKind(const QByteArray& responseCbor);
    static bool decodeHealth(const QByteArray& responseCbor, DecodedHealth* out);
    static bool decodeSessionPage(const QByteArray& responseCbor, QList<CachedSessionRow>* out,
                                  QString* nextCursor = nullptr);
    // Decode a LogPage into cache rows tagged with sessionId. Populates seq/direction/disposition +
    // the next/head cursors (enough to advance the sync cursor and project the envelope into a
    // domain::SessionLogEntry for the P4 transcript applier).
    //
    // DEFERRED (roadmap P5 - "dn-codec"): the entry's `origin` and `payload` are NOT captured yet.
    // This is blocked at the vendored client codec, not here:
    //   - The client subset (daemon-node .../daemon-api-client.cddl) models `session-payload` as
    //   ONLY
    //     `{ "Command": agent-command }` and `origin.scope` as a flat tstr; the wire union's
    //     Event/Request/Response/Meta arms (the assistant AgentEvents that make up a transcript)
    //     and the Dm/Group/Api/Internal scope union are not generated (a zcbor codegen collision
    //     the CDDL itself flags as future work).
    //   - There is no `encode_session_payload` and zcbor does not retain the raw per-entry bytes,
    //   so
    //     `CachedLogRow.payloadCbor` cannot be filled by re-encoding either.
    // Capturing payload/origin therefore requires growing that client CDDL subset + regenerating
    // the vendored codec in daemon-node (a separate, cross-repo task) before the daemon path can
    // feed full transcript content into the shared store.
    static bool decodeLogPage(const QByteArray& responseCbor, const QString& sessionId,
                              QList<CachedLogRow>* out, quint64* nextSeq = nullptr,
                              quint64* headSeq = nullptr);

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
};

} // namespace daemonapp::daemon
