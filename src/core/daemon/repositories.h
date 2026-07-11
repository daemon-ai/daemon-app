// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "daemon/daemon_cache_store.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon/page_loop.h"
#include "daemonnet/irouting_actions.h"

#include <optional>
#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QVariant>

class QTimer;

namespace daemonapp::daemon {

class RepositoryBase : public QObject {
public:
    RepositoryBase(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

protected:
    [[nodiscard]] NodeApiClient* client() const { return m_client; }
    [[nodiscard]] DaemonCacheStore* cache() const { return m_cache; }

private:
    NodeApiClient* m_client = nullptr;
    DaemonCacheStore* m_cache = nullptr;
};

// The surviving direct session verb seam (AD): node-authoritative CREATE (SessionCreate — the
// node mints the id), the operator Submit ops, and the on-demand SessionGet detail hydration.
// The legacy roster read path + cache feed + SessionUpdateMeta died with AD: roster reads live
// on the mirror `sessions` table (ingestor-fed); session-meta mutations ride the outbox lane.
class SessionRepository : public RepositoryBase {
    Q_OBJECT

public:
    SessionRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

    // --- Operator submit (F4/DEL-4, wire-reachable at v28) ------------------------------------
    // Session-addressable Submit ops for ANY session id (delegated children included) — the
    // operator steer/cancel row actions, distinct from the focused tab's DaemonTurnEngine (which
    // owns the active session's full turn lifecycle). The node authorizes per session ownership
    // (same-owner always passes; cross-owner needs the operator override) — a denial surfaces via
    // submitFailed, never a silent no-op.
    void startTurn(const QString& sessionId, const QString& text); // Submit{StartTurn}
    void steer(const QString& sessionId, const QString& text);     // Submit{Steer}
    void interrupt(const QString& sessionId);                      // Submit{Interrupt}

    // Node-authoritative session creation (WireVersion v23): ask the node to create a blank,
    // profile-bound, UN-RUN session (SessionCreate{profile}). The node MINTS the id and replies
    // SessionCreated{session} + emits RosterChanged; on the reply, sessionCreated(sessionId,
    // profileId) fires so the caller can update + auto-select event-driven. `profileId` empty =
    // the node's active default. Nothing is client-minted.
    void createSession(const QString& profileId = QString());

    // CHA-9 follow-on: fetch one session's full detail on demand (SessionGet -> SessionDetail).
    // The roster page carries only the SessionInfo row; this hydrates the extra facets (delivery
    // targets, child ids, checkpoint count, resolved model/approval mode) into an in-memory cache.
    // On success sessionDetailLoaded(id) fires and cachedDetail(id) is populated; an unknown
    // session (null arm) still fires sessionDetailLoaded (with an empty detail). On Error/transport
    // failure detailFailed(id, message) fires. Deduped: an in-flight fetch for the same id is not
    // re-issued.
    void getSessionDetail(const QString& sessionId);
    // The last hydrated detail for `id`, if a SessionGet has resolved it (the session-detail
    // projections + subagent title enrichment read this; the wire is never touched here).
    [[nodiscard]] bool cachedDetail(const QString& id, DecodedSessionDetail* out) const;

signals:
    // A transport-level SessionCreate failure (the create is dead; surfaced, never silent).
    void refreshFailed(const QString& message);
    // A node SessionCreate resolved: `sessionId` is the node-minted id, `profileId` the profile it
    // was bound under (echoed from the request so the auto-select path knows the agent).
    void sessionCreated(const QString& sessionId, const QString& profileId);
    // A SessionGet resolved and cachedDetail(sessionId) is now populated (empty on the null arm).
    void sessionDetailLoaded(const QString& sessionId);
    // A SessionGet was rejected (node ApiError) or failed at the transport. Dedicated so a
    // detail-hydration miss is distinguishable from a create failure.
    void detailFailed(const QString& sessionId, const QString& message);
    // An operator Submit (startTurn/steer/interrupt) was accepted by the node.
    void submitted(const QString& sessionId);
    // An operator Submit was rejected (node ApiError, e.g. Forbidden for a non-owner) or failed
    // at the transport. Surfaced (toast) instead of silently swallowed.
    void submitFailed(const QString& sessionId, const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    // Operator Submit reply (F4): non-Error = accepted -> submitted(); Error -> submitFailed().
    void applySubmitReply(const QString& sessionId, const QByteArray& responseCbor);

    // SessionCreated handling: node-authoritative create reply -> emit sessionCreated.
    void applySessionCreated(const QByteArray& responseCbor);
    // SessionGet reply: decode SessionDetail -> cache + sessionDetailLoaded; Error -> detailFailed.
    void applySessionDetail(const QString& sessionId, const QByteArray& responseCbor);

    static constexpr auto kCreateCorrelation = "repo/session-create";
    // SessionGet correlations carry the target session id on the tail (prefix-routed).
    static constexpr auto kDetailPrefix = "repo/session-get/";
    // Operator Submit correlations carry the target session id (distinct from the turn engine's
    // "turn/submit/<id>" so replies never cross wires).
    static constexpr auto kSubmitPrefix = "repo/submit/";

    // The profile the in-flight SessionCreate carried, echoed on sessionCreated so the caller's
    // auto-select knows which agent the new session belongs to.
    QString m_pendingCreateProfile;

    // CHA-9 detail hydration: the last-decoded detail per session id, plus the set of ids with an
    // in-flight SessionGet (dedupes re-issues while one is outstanding).
    QHash<QString, DecodedSessionDetail> m_details;
    QSet<QString> m_detailInFlight;
};

class ProfileRepository : public RepositoryBase {
    Q_OBJECT

public:
    ProfileRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

    bool upsertCachedProfile(const CachedProfileRow& row);
    [[nodiscard]] QList<CachedProfileRow> cachedProfiles() const;
    // Offline-first seed: reconstruct profiles() + the spec cache from the persisted
    // daemon_profiles rows (no connection needed), so the Profiles UI renders last-known agents on
    // a cold/offline start. A later live refreshProfiles() reconciles. Does not emit; the store
    // calls rebuild().
    void loadCachedProfiles();

    [[nodiscard]] const QList<DecodedProfileInfo>& profiles() const { return m_profiles; }
    // The active default profile id (what new sessions bind to), or empty if unknown. Falls back to
    // the first listed profile when none is flagged active. Onboarding stores credentials under it.
    [[nodiscard]] QString activeProfileId() const;

    // The full spec for `id` if a ProfileGet has loaded it (the editor's hydration source).
    [[nodiscard]] bool cachedSpec(const QString& id, DecodedProfileSpec* out) const;

    // Issue a ProfileList; on success profilesRefreshed() fires with profiles() populated.
    void refreshProfiles();
    // Switch the node's active profile (ProfileSelect); on Ok the list is re-fetched (PRO-5).
    void selectProfile(const QString& id);
    // Delete a profile (ProfileDelete); on Ok the list is re-fetched (PRO-4).
    void deleteProfile(const QString& id);
    // Create a profile from a full spec (ProfileCreate -> ProfileId); on success re-list (PRO-2).
    void createProfile(const DecodedProfileSpec& spec);
    // Replace a profile's spec in place (ProfileUpdate -> Ok); on success re-list + re-get (PRO-3).
    void updateProfile(const DecodedProfileSpec& spec);
    // Clone `source` into `newId` (ProfileClone -> ProfileId); on success re-list (PRO-2 clone).
    void cloneProfile(const QString& source, const QString& newId);
    // Fetch a profile's full spec (ProfileGet); on success caches it + fires profileSpecLoaded.
    void getProfile(const QString& id);

    // Persona (SOUL.md) seam (wire v36). requestSoul issues a SoulGet; on SoulText the text is
    // cached and soulLoaded(id) fires. setSoul issues a SoulSet; on Ok it re-issues a SoulGet
    // (authoritative refetch — the node validates/scans/caps + normalizes, so the app renders the
    // stored text, never the submitted string), which lands as soulLoaded; on a node ApiError
    // soulOpFailed(message) fires (e.g. Unsupported for a Foreign-engine profile, or persona
    // validation rejection). A SoulGet error is a capability/existence gap the persona editor
    // already gates on (foreign profiles hide the surface), so it is a silent no-op — never a
    // toast.
    void requestSoul(const QString& id);
    void setSoul(const QString& id, const QString& text);
    // The last-known persona for `id` if a SoulGet has answered (empty otherwise). Synchronous.
    [[nodiscard]] QString cachedSoul(const QString& id) const;

    // PRO-7: export a profile (ProfileExport -> Distribution); distributionExported carries the raw
    // response bytes (the portable artifact) + the decoded form. Import a distribution
    // (ProfileImport -> ProfileId); on success re-lists + imported() fires.
    void exportProfile(const QString& id);
    void importProfile(const DecodedDistribution& dist, const QString& newId = QString());
    // PRO-8: load a profile's revision history (ProfileHistory -> Revisions); revert to a revision
    // (ProfileRevert -> Ok), then re-get + re-list.
    void fetchProfileHistory(const QString& id);
    void revertProfile(const QString& id, quint64 seq);
    // Whether the connected daemon advertised the `versioning` capability in its Hello (valid once
    // handshaked; capabilitiesChanged fires when it becomes known).
    [[nodiscard]] bool daemonSupportsVersioning() const;

signals:
    void profilesRefreshed();
    void refreshFailed(const QString& message);
    void operationFailed(const QString& message);
    // A ProfileGet completed and cachedSpec(id) is now populated.
    void profileSpecLoaded(const QString& id);
    // A SoulGet answered (directly, or after a SoulSet re-fetch): cachedSoul(id) now holds fresh
    // authoritative persona text. The store re-emits this as the seam's soulChanged(id).
    void soulLoaded(const QString& id);
    // A SoulSet was rejected (node ApiError) or failed at the transport. The store re-emits this
    // as the seam's profileOpFailed(message). SoulGet errors do NOT come here (silent).
    void soulOpFailed(const QString& message);
    // PRO-7/8 results.
    void distributionExported(const QString& id, const QByteArray& responseBytes,
                              const DecodedDistribution& dist);
    void imported(const QString& newId);
    void historyLoaded(const QString& id, const QList<DecodedRevision>& revisions);
    // The daemon hosts no revision log (Unsupported) - versioning is a capability gap, not an
    // error.
    void historyUnavailable(const QString& id, const QString& message);
    void reverted(const QString& id);
    // The Hello handshake completed; daemonSupportsVersioning() is now valid.
    void capabilitiesChanged();

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    // handleResponse fans out via an exact-match table + a prefix-route loop; each correlation's
    // body lives in its own handler so the dispatcher (and the per-branch decode/cache nesting)
    // stays small.
    bool dispatchExactResponse(const QString& correlationId, const QByteArray& responseCbor);
    void dispatchPrefixedResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleProfilesResponse(const QByteArray& responseCbor);
    void handleProfileImportResponse(const QByteArray& responseCbor);
    void handleProfileMutationResponse(const QByteArray& responseCbor);
    void handleProfileGetResponse(const QString& id, const QByteArray& responseCbor);
    // SoulGet reply: SoulText -> cache + soulLoaded; error/decode-miss -> silent no-op.
    void handleSoulGetResponse(const QString& id, const QByteArray& responseCbor);
    // SoulSet reply: Ok -> re-issue SoulGet (authoritative refetch); error -> soulOpFailed.
    void handleSoulSetResponse(const QString& id, const QByteArray& responseCbor);
    void handleProfileExportResponse(const QString& id, const QByteArray& responseCbor);
    void handleProfileHistoryResponse(const QString& id, const QByteArray& responseCbor);
    void handleProfileRevertResponse(const QString& id, const QByteArray& responseCbor);
    void persistFetchedProfile(const QString& id, const QByteArray& responseCbor);
    void pruneStaleProfiles();
    [[nodiscard]] static bool isProfileOperation(const QString& correlationId);

    static constexpr auto kProfilesCorrelation = "repo/profile-list";
    static constexpr auto kSelectCorrelation = "repo/profile-select";
    static constexpr auto kDeleteCorrelation = "repo/profile-delete";
    static constexpr auto kCreateCorrelation = "repo/profile-create";
    static constexpr auto kUpdateCorrelation = "repo/profile-update";
    static constexpr auto kCloneCorrelation = "repo/profile-clone";
    static constexpr auto kGetPrefix = "repo/profile-get/";
    static constexpr auto kSoulGetPrefix = "repo/soul-get/";
    static constexpr auto kSoulSetPrefix = "repo/soul-set/";
    static constexpr auto kExportPrefix = "repo/profile-export/";
    static constexpr auto kImportCorrelation = "repo/profile-import";
    static constexpr auto kHistoryPrefix = "repo/profile-history/";
    static constexpr auto kRevertPrefix = "repo/profile-revert/";

    QList<DecodedProfileInfo> m_profiles;
    QHash<QString, DecodedProfileSpec> m_specs;
    // Last-known persona (SOUL.md) text per profile id, from the most recent SoulGet answer.
    QHash<QString, QString> m_souls;
    // Per-profile history page loop (wire v25): accumulate revisions across `after` pages, then
    // emit historyLoaded ONCE with the full history.
    QHash<QString, PageLoop<DecodedRevision>> m_historyLoops;
};

// Onboarding credential slice (CON-4): a `CredentialList` keeps a redacted view in memory (no
// secret ever lands here), and `CredentialSet`/`CredentialRemove` mutate the daemon store and
// re-list. The DaemonAccountsService projects this list into its account rows.
class CredentialRepository : public RepositoryBase {
    Q_OBJECT

public:
    CredentialRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

    [[nodiscard]] const QList<DecodedCredentialInfo>& credentials() const { return m_credentials; }

    // Issue a CredentialList; on success listRefreshed() fires with credentials() populated.
    void refreshList();
    // Store `secret` under `profile` (CredentialSet); on Ok the list is re-fetched.
    void setCredential(const QString& profile, const QString& secret);
    // Remove the secret under `profile` (CredentialRemove); on Ok the list is re-fetched.
    void removeCredential(const QString& profile);
    // [acct-mgmt] wire v35: persist the credential's display label (CredentialSetLabel;
    // hasLabel=false clears it to null). The node emits no event, so on Ok the list is re-fetched.
    void setCredentialLabel(const QString& profile, bool hasLabel, const QString& label);

signals:
    void listRefreshed();
    void operationFailed(const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    [[nodiscard]] static bool isOwnCorrelation(const QString& correlationId);

    static constexpr auto kListCorrelation = "repo/credential-list";
    static constexpr auto kSetCorrelation = "repo/credential-set";
    static constexpr auto kRemoveCorrelation = "repo/credential-remove";
    // [acct-mgmt] wire v35: set-label intent; on Ok the list is re-fetched (no node event).
    static constexpr auto kSetLabelCorrelation = "repo/credential-set-label";

    QList<DecodedCredentialInfo> m_credentials;
};

// Onboarding model slice (CON-6): `Models` discovery + `ModelCurrent` resolution kept in memory,
// plus `SetSessionModel` to choose a model for a live session. The DaemonModelCatalog projects
// these into its discover/installed rows + current selection.
class ModelRepository : public RepositoryBase {
    Q_OBJECT

public:
    ModelRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

    [[nodiscard]] const QList<DecodedModelDescriptor>& models() const { return m_models; }
    [[nodiscard]] bool hasCurrent() const { return m_hasCurrent; }
    [[nodiscard]] const DecodedModelDescriptor& currentModel() const { return m_current; }

    // Issue a Models request; on success modelsRefreshed() fires with models() populated.
    void refreshModels();
    // Resolve the current model for `profile` (empty = active default); currentRefreshed() fires.
    void refreshCurrent(const QString& profile = QString());
    // Set the model (and optionally re-bind provider) for a live session; on Ok modelSet() fires.
    void setSessionModel(const QString& sessionId, const QString& model,
                         const QString& provider = QString());

    // --- Local model track (Phase 2) ---------------------------------------------------------
    // Cached decoded views (the UI/catalog read these, never the wire).
    [[nodiscard]] const QList<DecodedSearchHit>& searchHits() const { return m_searchHits; }
    [[nodiscard]] const QList<DecodedModelFile>& files() const { return m_files; }
    [[nodiscard]] QString filesRepo() const { return m_filesRepo; }
    [[nodiscard]] bool hasRecommendation() const { return m_hasRecommendation; }
    [[nodiscard]] const DecodedQuantRecommendation& recommendation() const {
        return m_recommendation;
    }
    [[nodiscard]] const QList<DecodedDownloadStatus>& downloads() const { return m_downloads; }
    [[nodiscard]] const QList<DecodedInstalledModel>& installed() const { return m_installed; }

    // Step 1: search HF repos (ModelSearch); on success searchHitsChanged() fires with the fresh
    // first page. `has_more` from the response drives searchHasMore()/searchMore().
    void search(const QString& text, const QString& engine = QStringLiteral("llama"),
                const QString& sort = QStringLiteral("trending"));
    // Whether the last search page reported another page available.
    [[nodiscard]] bool searchHasMore() const { return m_searchHasMore; }
    // Fetch the NEXT page of the current search and APPEND its hits to searchHits();
    // searchHitsChanged() fires with the accumulated set. No-op when !searchHasMore().
    void searchMore();
    // Step 2: list a repo's loadable files (ModelFiles); on success filesLoaded(repo) fires.
    void requestFiles(const QString& repo, const QString& engine = QStringLiteral("llama"));
    // The hardware-aware quant recommendation for a repo; on success recommendLoaded(repo) fires.
    void recommend(const QString& repo, const QString& engine = QStringLiteral("llama"),
                   bool hasBudget = false, quint64 budgetBytes = 0);
    // Start a download of one repo file (ModelDownload); on success downloadStarted(id) fires and
    // the poll loop begins refreshing downloads() until all jobs settle.
    void download(const QString& repo, const QString& file,
                  const QString& engine = QStringLiteral("llama"),
                  const QString& revision = QStringLiteral("main"));
    // Poll all download jobs (ModelDownloads); on success downloadsChanged() fires. Reaching a
    // Completed job that was not completed before triggers a catalog refresh. Used for the initial
    // snapshot (on download start / lifecycle ops); live progress now arrives via the L3 feed.
    void refreshDownloads();
    // Apply one L3 DownloadProgress notification (from the SubscriptionManager): patch the matching
    // job row in place (insert if new), emit downloadsChanged(), and refresh the catalog when a job
    // newly reaches Completed. Replaces the retired 600ms poll. `downloadedBytes`/`totalBytes` are
    // the event's real byte counters (wire v26); a zero total keeps any previously-known total.
    void applyDownloadProgress(quint64 id, const QString& state, quint64 downloadedBytes,
                               quint64 totalBytes);
    // Refresh the installed catalog (ModelCatalog); on success catalogChanged() fires.
    void refreshCatalog();
    // Delete an installed model (ModelDelete); on Ok the catalog is re-fetched.
    void deleteModel(const QString& id);
    // Activate an installed model for `profile` (empty = default local); on Ok modelActivated()
    // fires.
    void activate(const QString& id, const QString& profile = QString());
    // Download lifecycle controls; on Ok the downloads list is re-polled.
    void cancelDownload(quint64 id);
    void pauseDownload(quint64 id);
    void resumeDownload(quint64 id);

    // --- Local re-quantization (A5) + ghost probe (A6); app-wizard-auth stream ---------------
    [[nodiscard]] const QList<DecodedQuantizeStatus>& quantizes() const { return m_quantizes; }
    // Start a quantize job for `repo` toward `targetQuant` (ModelQuantize). Empty `sourceFile`
    // lets the node pick the best installed source. On success quantizeStarted(id) fires and the
    // jobs poll loop begins refreshing quantizes() until every job settles.
    void quantize(const QString& repo, const QString& targetQuant,
                  const QString& sourceFile = QString());
    // Poll all quantize jobs (ModelQuantizes); on success quantizesChanged() fires. A job newly
    // reaching Completed refreshes the installed catalog (the produced model is cataloged).
    void refreshQuantizes();
    // A6 ghost probe: ModelInspect on a CATALOGED id. Success -> inspected(id, info) (the
    // artifact is present + readable); an Error reply -> inspectFailed(id, message) — the ghost
    // signal (the registry keeps records for deleted files). Detection stays behind this one seam
    // so a future node `on_disk` flag replaces the probe without UI changes.
    void inspect(const QString& id);

signals:
    void modelsRefreshed();
    void currentRefreshed();
    void modelSet();
    void operationFailed(const QString& message);
    void searchHitsChanged();
    void filesLoaded(const QString& repo);
    void recommendLoaded(const QString& repo);
    void downloadsChanged();
    void catalogChanged();
    void downloadStarted(quint64 id);
    void modelActivated();
    // app-wizard-auth stream additions (A5/A6).
    void quantizesChanged();
    void quantizeStarted(quint64 id);
    void inspected(const QString& id, const daemonapp::daemon::DecodedGgufInfo& info);
    void inspectFailed(const QString& id, const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    // One handler per correlation; handleResponse routes to them through a member-function-pointer
    // table so the dispatcher itself carries no branch complexity.
    void handleModelsResponse(const QByteArray& responseCbor);
    void handleModelCurrentResponse(const QByteArray& responseCbor);
    void handleSetSessionModelResponse(const QByteArray& responseCbor);
    void handleModelSearchResponse(const QByteArray& responseCbor);
    void handleModelFilesResponse(const QByteArray& responseCbor);
    void handleModelRecommendResponse(const QByteArray& responseCbor);
    void handleModelDownloadResponse(const QByteArray& responseCbor);
    void handleModelDownloadsResponse(const QByteArray& responseCbor);
    void handleModelCatalogResponse(const QByteArray& responseCbor);
    void handleModelDeleteResponse(const QByteArray& responseCbor);
    void handleModelActivateResponse(const QByteArray& responseCbor);
    void handleModelLifecycleResponse(const QByteArray& responseCbor);
    // Shared "ApiError -> message, else fallback" failure tail used by the set/download/activate
    // handlers.
    void emitOperationError(const QByteArray& responseCbor, const QString& fallback);

    // One handler per new correlation (app-wizard-auth stream, A5/A6).
    void handleModelQuantizeResponse(const QByteArray& responseCbor);
    void handleModelQuantizesResponse(const QByteArray& responseCbor);

    static constexpr auto kModelsCorrelation = "repo/models";
    static constexpr auto kCurrentCorrelation = "repo/model-current";
    static constexpr auto kSetModelCorrelation = "repo/set-session-model";
    static constexpr auto kSearchCorrelation = "repo/model-search";
    static constexpr auto kFilesCorrelation = "repo/model-files";
    static constexpr auto kRecommendCorrelation = "repo/model-recommend";
    static constexpr auto kDownloadCorrelation = "repo/model-download";
    static constexpr auto kDownloadsCorrelation = "repo/model-downloads";
    static constexpr auto kCatalogCorrelation = "repo/model-catalog";
    static constexpr auto kDeleteCorrelation = "repo/model-delete";
    static constexpr auto kActivateCorrelation = "repo/model-activate";
    static constexpr auto kLifecycleCorrelation = "repo/model-lifecycle";
    // app-wizard-auth stream correlations (A5/A6). Inspect is prefixed (the id rides the tail).
    static constexpr auto kQuantizeCorrelation = "repo/model-quantize";
    static constexpr auto kQuantizesCorrelation = "repo/model-quantizes";
    static constexpr auto kInspectPrefix = "repo/model-inspect/";

    QList<DecodedModelDescriptor> m_models;
    DecodedModelDescriptor m_current;
    bool m_hasCurrent = false;
    // Page loops (wire v25): models() and files() accumulate across `after` pages and their
    // refreshed/loaded signals fire ONCE with the complete listing.
    PageLoop<DecodedModelDescriptor> m_modelsLoop;
    PageLoop<DecodedModelFile> m_filesLoop;

    QList<DecodedSearchHit> m_searchHits;
    // Search paging state: the query the next page re-issues verbatim, the 0-based page the last
    // response answered, whether the node reported another page, and whether the in-flight
    // request should APPEND (searchMore) rather than replace (a fresh search).
    QString m_searchText;
    QString m_searchEngine = QStringLiteral("llama");
    QString m_searchSort = QStringLiteral("trending");
    quint32 m_searchPage = 0;
    bool m_searchHasMore = false;
    bool m_searchAppending = false;
    QList<DecodedModelFile> m_files;
    QString m_filesRepo;
    QString m_pendingFilesRepo;
    QString m_pendingFilesEngine; // re-issued verbatim on the files page loop's continuations
    DecodedQuantRecommendation m_recommendation;
    bool m_hasRecommendation = false;
    QString m_pendingRecommendRepo;
    QList<DecodedDownloadStatus> m_downloads;
    QList<DecodedInstalledModel> m_installed;
    QSet<quint64> m_completedDownloads; // ids already seen Completed (debounces catalog refresh)
    // app-wizard-auth stream state (A5): the quantize job snapshots + the settle poll timer +
    // the ids already seen Completed (debounces the catalog refresh, mirroring downloads).
    QList<DecodedQuantizeStatus> m_quantizes;
    QTimer* m_quantizePoll = nullptr;
    QSet<quint64> m_completedQuantizes;
};

// Provider/model discovery slice: `ProviderCatalog` enumerates the providers the node offers
// (local engines + every genai cloud vendor + Daemon Cloud), and `ProviderModels{provider,
// credential_ref?, transient_key?}` lists one provider's models. Both are kept in memory; the
// DaemonProviderCatalog seam projects them into provider/model rows. `provider` is the
// ProviderDescriptor.id STRING (not a ProviderSelector): genai vendors share one selector and are
// disambiguated by id.
class ProviderRepository : public RepositoryBase {
    Q_OBJECT

public:
    ProviderRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

    [[nodiscard]] const QList<DecodedProviderDescriptor>& providers() const { return m_providers; }
    // The last-fetched models for `provider` (empty until a ProviderModels for it resolves).
    [[nodiscard]] QList<DecodedModelDescriptor> modelsFor(const QString& provider) const {
        return m_models.value(provider);
    }

    // Issue a ProviderCatalog; on success providersRefreshed() fires with providers() populated.
    void refreshProviders();
    // Issue a ProviderModels for `provider` (the descriptor id). Pass `transientKey` to list a
    // key-requiring provider before a credential exists (first-run); `credentialRef` (the profile
    // id) for an existing profile. On success providerModelsRefreshed(provider) fires.
    void refreshProviderModels(const QString& provider, const QString& credentialRef = QString(),
                               const QString& transientKey = QString());

    // The persisted user-defined custom providers (the editor's read-your-writes view; empty until
    // a CustomProviderList resolves).
    [[nodiscard]] const QList<DecodedCustomProvider>& customProviders() const {
        return m_customProviders;
    }
    // Issue a CustomProviderList; on success customProvidersRefreshed() fires.
    void refreshCustomProviders();
    // Create/update a custom provider (CustomProviderSet). On the node's ack, BOTH the custom list
    // and the merged provider catalog are re-fetched (a custom row surfaces in the picker).
    void setCustomProvider(const DecodedCustomProvider& provider);
    // Remove a user-defined custom provider by id (CustomProviderRemove). On ack, both re-fetch.
    void removeCustomProvider(const QString& id);

signals:
    void providersRefreshed();
    void providerModelsRefreshed(const QString& provider);
    void customProvidersRefreshed();
    void operationFailed(const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    static constexpr auto kCatalogCorrelation = "repo/provider-catalog";
    static constexpr auto kModelsPrefix = "repo/provider-models/";
    static constexpr auto kCustomListCorrelation = "repo/custom-provider-list";
    static constexpr auto kCustomSetCorrelation = "repo/custom-provider-set";
    static constexpr auto kCustomRemoveCorrelation = "repo/custom-provider-remove";

    QList<DecodedProviderDescriptor> m_providers;
    QList<DecodedCustomProvider> m_customProviders;
    QHash<QString, QList<DecodedModelDescriptor>> m_models; // provider id -> its offered models
    // Per-provider ProviderModels page loop (wire v25) + the credentials the continuation pages
    // re-issue verbatim; providerModelsRefreshed fires ONCE with the complete listing.
    struct ProviderModelsLoop {
        PageLoop<DecodedModelDescriptor> loop;
        QString credentialRef;
        QString transientKey;
    };
    QHash<QString, ProviderModelsLoop> m_modelsLoops;
};

// (FsRepository removed in Phase 4: DaemonFsService talks to NodeApiClient + the codec directly and
// uses DaemonCacheStore::{upsertFsEntry,fsEntries} for the daemon_fs_entries offline cache, so the
// pass-through repository layer was redundant.)

// Pending approvals are a live daemon slice (PRO-11): refreshPending() issues an ApprovalsPending
// query and decode() populates pending(); decide() resolves one via ApprovalDecide. The aggregate
// inbox keys on the STRING request_id (ApprovalInfo.request_id), distinct from the in-stream
// HostRequest gate (uint) the turn engine resolves with Respond.
class ApprovalRepository : public RepositoryBase {
    Q_OBJECT

public:
    ApprovalRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

    [[nodiscard]] const QList<DecodedApprovalInfo>& pending() const { return m_pending; }

    // Issue an ApprovalsPending query (empty session = across all sessions); on success
    // pendingRefreshed() fires with pending() populated.
    void refreshPending(const QString& sessionId = QString());
    // Resolve a pending approval (ApprovalDecide); on Ok the list is re-fetched and decided()
    // fires. `allowPermanent` (wire v28) is only meaningful for an allow of a fingerprinted
    // approval (see DecodedApprovalInfo::hasFingerprint); it rides the optional ApprovalDecide
    // field.
    // [wave2:app-approvals-safety] D3: `reason` (wire v29) rides ApprovalDecide.reason on a deny —
    // the operator explanation the node threads into the model's next turn. Empty = absent.
    void decide(const QString& sessionId, const QString& requestId, bool allow,
                bool allowPermanent = false, const QString& reason = QString());

signals:
    void pendingRefreshed();
    void decided();
    void operationFailed(const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    static constexpr auto kPendingCorrelation = "repo/approvals-pending";
    static constexpr auto kDecideCorrelation = "repo/approval-decide";

    QList<DecodedApprovalInfo> m_pending;
    QString m_lastSession; // the session scope of the last refresh, re-used after a decide
    // Pending page loop (wire v25): accumulate across `after` pages, then swap pending() and emit
    // pendingRefreshed ONCE.
    PageLoop<DecodedApprovalInfo> m_pendingLoop;
};

// The fleet CONTROL seam (PRO-10; AD): pause/resume/scale issue the control op; on Ok
// controlAcked() fires (the graph/MirrorFleetTree bridges it to the ingestor's Tree refetch so
// the MIRROR tree re-renders the node's stored state); on Unsupported/error controlFailed()
// fires (control is only meaningful on orchestrator units; engine leaves return Unsupported).
// The legacy tree FEED (refreshTree/cachedUnits/daemon_fleet_units) died with AD — the tree
// reads live on the mirror `fleet_units` table.
class FleetRepository : public RepositoryBase {
    Q_OBJECT

public:
    FleetRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

    void pause(const QString& unitId);
    void resume(const QString& unitId);
    void scale(const QString& unitId, quint32 n);

signals:
    // A control op was acked by the node; the mirror Tree refetch renders the outcome.
    void controlAcked();
    void controlFailed(const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    void handleUnitControlResponse(const QByteArray& responseCbor);

    static constexpr auto kControlCorrelation = "repo/unit-control";
};

// [wave2:app-delegation] F7/DEL-7: the node's delegation guardrail ceilings (Caps -> CapsReport).
// refresh() issues the payload-free Caps request; on success capsRefreshed() fires with the two
// ceilings populated. Node-wide, read-only policy (no cache: re-fetched on focus, never stale-
// rendered as settable) — the app cannot mutate it (no wire write op). Backs the read-only
// "Delegation limits" rows in Settings -> Safety.
class CapsRepository : public RepositoryBase {
    Q_OBJECT
    Q_PROPERTY(quint32 orchestrateMaxDepth READ orchestrateMaxDepth NOTIFY capsRefreshed)
    Q_PROPERTY(quint32 orchestrateMaxFanout READ orchestrateMaxFanout NOTIFY capsRefreshed)
    // Agent-authored-profile guardrail ceilings (wire v31, phase H): profiles a session may author
    // and concurrent inline/ephemeral children per session. Advisory display only — the node
    // enforces. Already decoded into m_caps; surfaced read-only beside depth/fanout.
    Q_PROPERTY(quint32 maxComposedProfiles READ maxComposedProfiles NOTIFY capsRefreshed)
    Q_PROPERTY(quint32 maxEphemeralPerSession READ maxEphemeralPerSession NOTIFY capsRefreshed)
    Q_PROPERTY(bool loaded READ loaded NOTIFY capsRefreshed)

public:
    explicit CapsRepository(NodeApiClient* client, QObject* parent = nullptr);

    [[nodiscard]] quint32 orchestrateMaxDepth() const { return m_caps.orchestrateMaxDepth; }
    [[nodiscard]] quint32 orchestrateMaxFanout() const { return m_caps.orchestrateMaxFanout; }
    [[nodiscard]] quint32 maxComposedProfiles() const { return m_caps.maxComposedProfiles; }
    [[nodiscard]] quint32 maxEphemeralPerSession() const { return m_caps.maxEphemeralPerSession; }
    [[nodiscard]] bool loaded() const { return m_loaded; }

    Q_INVOKABLE void refresh();

signals:
    void capsRefreshed();
    void refreshFailed(const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    static constexpr auto kCapsCorrelation = "repo/caps";
    DecodedCapsReport m_caps;
    bool m_loaded = false;
};

// Channels / Events-IO read surface (Phase 6a, story 04). Projects the node's transport adapter
// registry (TransportAdapters), configured accounts + live status (TransportInstances; EIO-9), and
// live per-account conversations (ConvList; EIO-8). Instances + conversations are cached for the
// offline-first read; adapters are connect-only (the "Add channel" picker needs a live daemon).
class TransportRepository : public RepositoryBase {
    Q_OBJECT

public:
    TransportRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

    [[nodiscard]] QList<DecodedAdapterInfo> adapters() const { return m_adapters; }
    [[nodiscard]] QList<CachedTransportInstanceRow> cachedInstances() const;
    [[nodiscard]] QList<CachedConversationRow> cachedConversations(const QString& transport) const;

    void refreshAdapters();
    void refreshInstances();
    void refreshConversations(const QString& transport);
    // [waveB:app-v30] D2: the connect-ready ConvList baseline — fetch every cached account's
    // conversations once (replaces the retired per-tab-enter / per-expand polling; the feed's
    // ConversationsChanged / MembershipChanged then keeps them fresh incrementally).
    void refreshAllConversations();

    // [waveB:app-v30] D1: transport-instance lifecycle. `disconnect` tears down the live session
    // (TransportDisconnect); `remove` fully removes the account (TransportRemove — the node
    // sequences disconnect + conv close + routing unbind + credential drop + config drop). Both
    // send ONE intent; on Ok the instances re-list so the client renders the node's reported
    // outcome (never an optimistic local mutation).
    void disconnect(const QString& transport);
    void remove(const QString& transport);

    // [acct-mgmt] wire v35: reversible lifecycle + persisted metadata (ControlApi-level; available
    // for every transport). connectTransport re-spawns the adapter family serve loop
    // (TransportConnect; idempotent). setEnabled persists the desired state (TransportSetEnabled;
    // disable also disconnects, enable does NOT auto-connect — the caller connects separately).
    // setTransportLabel persists the display label (TransportSetLabel; hasLabel=false clears it to
    // null). Each sends ONE intent and, on Ok, re-lists so the client renders the node's reported
    // state (the node also emits TransportChanged) — never an optimistic local mutation.
    void connectTransport(const QString& transport);
    void setEnabled(const QString& transport, bool enabled);
    void setTransportLabel(const QString& transport, bool hasLabel, const QString& label);

    // [wave2:app-channels-liveness] B5: apply a live TransportChanged node-event in place. Patches
    // the cached row's connection/presence (preserving family/displayName/boundProfile) and emits
    // instancesRefreshed() so the DaemonPresenceService re-projects and the status dots re-read -
    // no round-trip. Falls back to refreshInstances() when the transport is not yet cached (a
    // brand-new account before its first TransportInstances). Mirrors
    // ModelRepository::applyDownloadProgress.
    // [waveB:app-v30] D1: also patches the node-reported disconnect provenance
    // (reason/message/fatal); hasReason/hasMessage mark which optionals the event carried.
    void applyTransportChanged(const QString& transport, const QString& connection,
                               const QString& presence, bool hasPresence, const QString& reason,
                               bool hasReason, const QString& message, bool hasMessage, bool fatal);

    // [wave2:app-channels-liveness] B2: presentation-only "new room" tracking. A conversation id
    // that appears in a ConvList refresh but was NOT in the transport's prior known set is badged
    // "new" until the operator views it. Membership itself is the node's (ConvList) - this only
    // tracks which rooms the operator has already seen. In-memory, per session (no cache/schema
    // change).
    [[nodiscard]] bool isNewConversation(const QString& transport, const QString& conv) const;
    void markConversationSeen(const QString& transport, const QString& conv);

    // --- [acct-mgmt] Room lifecycle + member management (Phase B, wire v32) --------------------
    // Two-phase form ops: conversationJoinDetails / conversationCreateDetails fetch the node's form
    // (ConvJoinDetails / ConvCreateDetails) and fire joinDetailsReady / createDetailsReady with a
    // QVariantMap descriptor the dialogs render; joinRoom / createRoom send the filled form
    // (ConvJoin / ConvCreate) and, on Ok, refresh that transport's ConvList. Single-verb ops
    // (leaveRoom / deleteRoom) name the conversation and refresh on Ok. conversationMembers issues
    // ConvGet and fires membersReady with the decoded member list; the member verbs
    // (memberInvite/memberKick/memberBan/memberSetRole) act, and on Ok re-issue ConvGet so the
    // member palette re-renders (the node's MembershipChanged event also refetches the ConvList).
    void conversationJoinDetails(const QString& transport);
    void joinRoom(const QString& transport, const QVariantMap& form);
    void conversationCreateDetails(const QString& transport);
    void createRoom(const QString& transport, const QVariantMap& form);
    void leaveRoom(const QString& transport, const QString& conversation);
    void deleteRoom(const QString& transport, const QString& conversation);
    void conversationMembers(const QString& transport, const QString& conversation);
    void memberInvite(const QString& transport, const QString& conversation,
                      const QString& contactId);
    void memberKick(const QString& transport, const QString& conversation,
                    const QString& contactId);
    void memberBan(const QString& transport, const QString& conversation, const QString& contactId);
    void memberSetRole(const QString& transport, const QString& conversation,
                       const QString& contactId, const QString& role);

    // --- [integrations wire v38] Account settings (read + configure) ---------------------------
    // The node owns the persisted NON-SECRET account-settings values. refreshSettings(transport)
    // issues TransportSettings and, on the reply, caches the values + fires settingsRefreshed;
    // settings(transport) returns the last-known map (empty until a refresh lands). configure
    // (transport, values) issues TransportConfigure (merge-edit; the node validates + reconnects)
    // and, on Ok, re-reads the settings (authoritative — the client renders the node's stored
    // values, never the submitted ones). Secrets never ride these ops.
    [[nodiscard]] QMap<QString, QString> settings(const QString& transport) const {
        return m_settings.value(transport);
    }
    void refreshSettings(const QString& transport);
    void configure(const QString& transport, const QMap<QString, QString>& values);

signals:
    void adaptersRefreshed();
    void instancesRefreshed();
    void conversationsRefreshed(const QString& transport);
    void operationFailed(const QString& message);
    // [integrations wire v38] A transport's account settings landed (TransportSettings decoded /
    // a configure re-read them). `settings(transport)` is now populated.
    void settingsRefreshed(const QString& transport);
    // [acct-mgmt] Two-phase form descriptors + the member list for the member palette. The forms
    // are QVariantMaps the dialogs render (see the codec's DecodedChannelJoinDetails /
    // DecodedCreateConversationDetails); `members` is a QVariantList of member maps.
    void joinDetailsReady(const QString& transport, const QVariantMap& form);
    void createDetailsReady(const QString& transport, const QVariantMap& form);
    void membersReady(const QString& transport, const QString& conversation,
                      const QVariantList& members);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    void handleAdaptersResponse(const QByteArray& responseCbor);
    void handleInstancesResponse(const QByteArray& responseCbor);
    void handleConversationsResponse(const QString& transport, const QByteArray& responseCbor);
    void syncTransportInstances(const QList<DecodedTransportInstance>& live);
    void syncConversations(const QString& transport, const QList<DecodedConversation>& live);
    [[nodiscard]] static bool isOwnCorrelation(const QString& correlationId);

    // [acct-mgmt] Two-phase form + ConvGet + member-op reply handlers. Ok-only ops (join/create/
    // leave/delete) refresh the transport's ConvList; member ops re-issue ConvGet for the conv.
    void handleJoinDetailsResponse(const QString& transport, const QByteArray& responseCbor);
    void handleCreateDetailsResponse(const QString& transport, const QByteArray& responseCbor);
    void handleConversationGetResponse(const QString& transport, const QString& conv,
                                       const QByteArray& responseCbor);
    void handleRoomOkResponse(const QString& transport, const QByteArray& responseCbor,
                              const QString& failMessage);
    void handleMemberOpResponse(const QString& transport, const QString& conv,
                                const QByteArray& responseCbor);
    // [integrations wire v38] TransportSettings reply -> cache values + settingsRefreshed; a
    // TransportConfigure Ok -> re-read the settings (authoritative); Error -> operationFailed.
    void handleSettingsResponse(const QString& transport, const QByteArray& responseCbor);
    void handleConfigureResponse(const QString& transport, const QByteArray& responseCbor);

    QList<DecodedAdapterInfo> m_adapters; // in-memory (connect-only); not cached
    // Per-transport conversations page loop (wire v25): accumulate across `after` pages, then
    // sync (replace + prune) ONCE over the whole listing — pruning against a single page would
    // drop every conversation beyond it.
    QHash<QString, PageLoop<DecodedConversation>> m_convLoops;
    // [wave2:app-channels-liveness] B2 "new room" tracking (presentation-only, in-memory).
    // m_knownConversations is the per-transport set of ids the operator has already been shown; the
    // first refresh of a transport in the session establishes it (no badges), so a restart doesn't
    // badge everything. m_newConversations holds ids that appeared on a LATER refresh and haven't
    // been viewed.
    QHash<QString, QSet<QString>> m_knownConversations;
    QHash<QString, QSet<QString>> m_newConversations;

    static constexpr auto kAdaptersCorrelation = "repo/transport-adapters";
    static constexpr auto kInstancesCorrelation = "repo/transport-instances";
    static constexpr auto kConvPrefix = "repo/conv-list/";
    // [waveB:app-v30] D1: disconnect/remove intents; on Ok both re-list the instances.
    static constexpr auto kDisconnectCorrelation = "repo/transport-disconnect";
    static constexpr auto kRemoveCorrelation = "repo/transport-remove";
    // [acct-mgmt] wire v35: connect/set-enabled/set-label intents; on Ok all re-list the instances.
    static constexpr auto kConnectCorrelation = "repo/transport-connect";
    static constexpr auto kSetEnabledCorrelation = "repo/transport-set-enabled";
    static constexpr auto kSetLabelCorrelation = "repo/transport-set-label";

    // [acct-mgmt] Room lifecycle + member ops. The transport (and conv for two-key ops) ride the
    // correlation tail; a unit-separator (\x1f, never in an id) splits transport from conv so the
    // reply handler recovers both without a side map.
    static constexpr auto kJoinDetailsPrefix = "repo/conv-join-details/";
    static constexpr auto kCreateDetailsPrefix = "repo/conv-create-details/";
    static constexpr auto kJoinPrefix = "repo/conv-join/";
    static constexpr auto kCreatePrefix = "repo/conv-create/";
    static constexpr auto kLeavePrefix = "repo/conv-leave/";
    static constexpr auto kDeletePrefix = "repo/conv-delete/";
    static constexpr auto kConvGetPrefix = "repo/conv-get/";
    static constexpr auto kMemberOpPrefix = "repo/member-op/";
    // [integrations wire v38] Account settings read/configure; the transport rides the tail.
    static constexpr auto kSettingsPrefix = "repo/transport-settings/";
    static constexpr auto kConfigurePrefix = "repo/transport-configure/";

    // [integrations wire v38] transport -> last-known non-secret account-settings values.
    QHash<QString, QMap<QString, QString>> m_settings;
};

// [acct-mgmt] Transport contacts / roster (Phase D, wire v34). The node owns the roster; this
// repository is a thin seam over its verbs. refreshContacts(transport) issues RosterList and loops
// the `next` cursor (kWirePageMax) into an in-memory per-transport list (re-fetched on focus + the
// ContactsChanged feed event — not persisted, like RoutingRepository's rooms); addContact/
// updateContact/removeContact/setAlias mutate (RosterAdd/Update/Remove/ContactSetAlias) and, on Ok,
// re-refresh that transport (the node's ContactsChanged event also refetches). getProfile fires
// profileReady with the node-rendered ContactProfile string; searchDirectory fires directoryReady
// with the DirectorySearch results (a separate list from the roster). NOTHING is derived client-
// side. DISTINCT from the session-inbox roster (SessionRepository / RosterChanged).
class ContactsRepository : public RepositoryBase {
    Q_OBJECT

public:
    ContactsRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

    // The last-known contacts / directory results for `transport` (empty until a refresh/search
    // resolves). In-memory — the wire is never touched by these getters.
    [[nodiscard]] QList<DecodedContact> contacts(const QString& transport) const {
        return m_contacts.value(transport);
    }
    [[nodiscard]] QList<DecodedContact> directory(const QString& transport) const {
        return m_directory.value(transport);
    }

    // RosterList (paged): accumulate across `next` cursors, then swap contacts(transport) and emit
    // contactsRefreshed ONCE.
    void refreshContacts(const QString& transport);
    // RosterAdd/Update/Remove: `contactId` names the target (add/remove need only the id; update
    // carries the editable fields). On Ok the transport re-refreshes.
    void addContact(const QString& transport, const QString& contactId);
    void updateContact(const QString& transport, const DecodedContact& contact);
    void removeContact(const QString& transport, const QString& contactId);
    // ContactSetAlias: empty `alias` clears the local alias. On Ok the transport re-refreshes.
    void setAlias(const QString& transport, const QString& contactId, const QString& alias);
    // ContactGetProfile -> profileReady(transport, contactId, profile).
    void getProfile(const QString& transport, const QString& contactId);
    // DirectorySearch -> directoryReady(transport) with directory(transport) populated.
    void searchDirectory(const QString& transport, const QString& query = QString());

signals:
    void contactsRefreshed(const QString& transport);
    void directoryReady(const QString& transport);
    void profileReady(const QString& transport, const QString& contactId, const QString& profile);
    void operationFailed(const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    void handleRosterListResponse(const QString& transport, const QByteArray& responseCbor);
    void handleDirectoryResponse(const QString& transport, const QByteArray& responseCbor);
    void handleProfileResponse(const QString& transport, const QString& contactId,
                               const QByteArray& responseCbor);
    // Shared Ok-or-error tail for the mutation verbs (add/update/remove/alias): Ok -> re-refresh
    // the transport; Error/transport-failure -> operationFailed.
    void handleMutationResponse(const QString& transport, const QByteArray& responseCbor,
                                const QString& failMessage);
    [[nodiscard]] static bool isOwnCorrelation(const QString& correlationId);

    // The transport (and contact id for the two-key profile op) ride the correlation tail; a
    // unit-separator (\x1f, never in an id) splits transport from contact (the TransportRepository
    // member-op precedent).
    static constexpr auto kListPrefix = "repo/roster-list/";
    static constexpr auto kAddPrefix = "repo/roster-add/";
    static constexpr auto kUpdatePrefix = "repo/roster-update/";
    static constexpr auto kRemovePrefix = "repo/roster-remove/";
    static constexpr auto kAliasPrefix = "repo/contact-alias/";
    static constexpr auto kProfilePrefix = "repo/contact-profile/";
    static constexpr auto kDirectoryPrefix = "repo/directory-search/";

    QHash<QString, QList<DecodedContact>> m_contacts;  // transport -> its roster
    QHash<QString, QList<DecodedContact>> m_directory; // transport -> last directory results
    // Per-transport RosterList page loop (wire v34): accumulate across `next` pages, then swap.
    QHash<QString, PageLoop<DecodedContact>> m_contactLoops;
};

// [integrations wire v38] The node's person/metacontact registry (PersonList -> Persons; re-listed
// on PersonsChanged). refresh() issues PersonList; on the reply persons() is populated and
// personsRefreshed() fires. Read-only at v38 (the node owns the registry). Kept in memory
// (re-fetched on connect/focus + the PersonsChanged feed event), like the contacts roster — small,
// authoritative, no offline schema.
class PersonsRepository : public RepositoryBase {
    Q_OBJECT

public:
    PersonsRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

    [[nodiscard]] const QList<DecodedPerson>& persons() const { return m_persons; }

    // Issue a PersonList; on success personsRefreshed() fires with persons() populated.
    void refresh();

signals:
    void personsRefreshed();
    void operationFailed(const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    static constexpr auto kListCorrelation = "repo/person-list";

    QList<DecodedPerson> m_persons;
};

// [integrations wire v38] Native chat: a conversation's durable transcript (ConvHistory) + the send
// intent (ConvSend). refreshHistory(transport, conv) issues ConvHistory and loops the
// `after_cursor` (kWirePageMax) into an in-memory per-conversation message list (newest cursor
// tracked for the MessagesChanged incremental fetch), swapping
// messages(transport, conv) and firing historyRefreshed ONCE per refresh. send(transport, conv,
// text) issues ConvSend and, on Ok, fires messageSent (no optimistic echo — the node appends the
// Chat record + emits MessagesChanged, which drives the authoritative refetch via
// applyMessagesChanged). applyMessagesChanged(transport, conv) is the feed hook (fetch the tail
// past the known cursor). NOTHING is derived client-side; the node owns the transcript. Not cached
// offline — a live, per-conversation query (like the roster).
class ChatRepository : public RepositoryBase {
    Q_OBJECT

public:
    ChatRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

    // The last-known messages for a conversation (oldest-first; empty until a refresh lands).
    [[nodiscard]] QList<DecodedChatMessage> messages(const QString& transport,
                                                     const QString& conv) const {
        return m_messages.value(convKey(transport, conv));
    }

    // ConvHistory (paged forward from the start): accumulate the full transcript, swap
    // messages(transport, conv), and emit historyRefreshed ONCE. (The v38 wire pages forward from a
    // cursor only — there is no backward "load older" op.)
    void refreshHistory(const QString& transport, const QString& conv);
    // ConvSend a plain-text message; on Ok messageSent(transport, conv) fires. The authoritative
    // transcript update arrives via the MessagesChanged feed event (applyMessagesChanged).
    void send(const QString& transport, const QString& conv, const QString& text);
    // Feed hook (MessagesChanged): re-fetch the conversation's transcript so the tail lands. Kept a
    // full refresh at v38 for simplicity (the node bounds the page); a later delta can page the
    // tail past the known cursor.
    void applyMessagesChanged(const QString& transport, const QString& conv);

signals:
    void historyRefreshed(const QString& transport, const QString& conv);
    void messageSent(const QString& transport, const QString& conv);
    void operationFailed(const QString& transport, const QString& conv, const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);
    void handleHistoryResponse(const QString& transport, const QString& conv,
                               const QByteArray& responseCbor);
    void handleSendResponse(const QString& transport, const QString& conv,
                            const QByteArray& responseCbor);

    // The conversation key ("<transport>\x1f<conv>", the unit-separator never in an id) shared by
    // the message map + the correlation tail (transport and conv both ride it).
    [[nodiscard]] static QString convKey(const QString& transport, const QString& conv);
    // Split a "<transport>\x1f<conv>" tail back into its two ids.
    static bool splitConvKey(const QString& tail, QString* transport, QString* conv);

    static constexpr auto kHistoryPrefix = "repo/conv-history/";
    static constexpr auto kSendPrefix = "repo/conv-send/";

    // conv-key -> its accumulated (oldest-first) message list.
    QHash<QString, QList<DecodedChatMessage>> m_messages;
    // Per-conversation ConvHistory page loop: accumulate across `after_cursor` pages, then swap.
    QHash<QString, PageLoop<DecodedChatMessage>> m_historyLoops;
};

// Durable checkpoints (E4/TOOL-9): refresh(session) issues a CheckpointList (page loop) and
// checkpointsRefreshed() fires with checkpoints() populated; rewind(session, id) issues a
// CheckpointRewind and rewound() fires on Ok. DISTINCT from the turn-level Rewind/RewindTo ops:
// these are the node's durable tool-event checkpoints (they survive restarts). Kept in memory
// (per-session, re-fetched on focus); not cached offline — a rewind affordance must never act on
// stale rows.
class CheckpointRepository : public RepositoryBase {
    Q_OBJECT

public:
    CheckpointRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

    [[nodiscard]] const QList<DecodedCheckpointInfo>& checkpoints() const { return m_checkpoints; }
    [[nodiscard]] QString lastSession() const { return m_lastSession; }

    // Issue a CheckpointList scoped to `sessionId`; on success checkpointsRefreshed(sessionId)
    // fires with checkpoints() populated (paged; accumulates across `after` cursors).
    void refresh(const QString& sessionId);
    // Rewind `sessionId` to `checkpointId` (CheckpointRewind); on Ok rewound(sessionId) fires and
    // the list is re-fetched (the node prunes checkpoints past the restore point).
    void rewind(const QString& sessionId, const QString& checkpointId);

signals:
    void checkpointsRefreshed(const QString& sessionId);
    void rewound(const QString& sessionId);
    void operationFailed(const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    static constexpr auto kListCorrelation = "repo/checkpoint-list";
    static constexpr auto kRewindCorrelation = "repo/checkpoint-rewind";

    QList<DecodedCheckpointInfo> m_checkpoints;
    QString m_lastSession;   // the session scope of the last refresh (echoed on signals)
    QString m_pendingRewind; // the session of the in-flight rewind (echoed on rewound)
    // Checkpoint page loop (wire v25): accumulate across `after` pages, then swap checkpoints()
    // and emit checkpointsRefreshed ONCE.
    PageLoop<DecodedCheckpointInfo> m_listLoop;
};

// --- Foreign-agent catalog (foreign engines; wire v29) ---------------------------------------
// The node's foreign-agent catalog (AgentCatalog: curated builtins + durable manual registrations
// merged with the last discovery scan), kept in memory for the engine picker and the Agents
// settings surface. Register/remove drive AgentRegister/AgentRemove — the node forces
// source=Manual and RE-PROBES (a caller-supplied `installed` is never trusted) and no launch
// recipe ever travels back out; a profile references an entry BY NAME via the ProfileSpec engine
// selector. The wire op names stay inside this repository + the codec (rename isolation).
class AgentRepository : public RepositoryBase {
    Q_OBJECT

public:
    AgentRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

    [[nodiscard]] const QList<DecodedAgentEntry>& entries() const { return m_entries; }
    // QML-consumable rows: {name, source, protocol, installed, version}.
    [[nodiscard]] Q_INVOKABLE QVariantList agents() const;

    // Issue an AgentCatalog; on success catalogRefreshed() fires with entries() populated.
    Q_INVOKABLE void refreshCatalog();
    // Ask the node for a FRESH discovery scan (AgentDiscover: PATH/endpoint probe merged with the
    // durable catalog; the response reuses the AgentCatalog shape). Same catalogRefreshed() signal.
    Q_INVOKABLE void discover();
    // Register a manual foreign agent (AgentRegister). `form` keys: name, protocol ("Acp"|
    // "StreamJson"), program, args (QStringList), env (QVariantMap), endpoint. On the node's Ok
    // (it re-probes) this chains a catalog refresh + discovery and fires agentRegistered().
    Q_INVOKABLE void registerAgent(const QVariantMap& form);
    // Remove a manual foreign agent by catalog name (AgentRemove); chains a refresh on Ok.
    Q_INVOKABLE void removeAgent(const QString& name);

signals:
    void catalogRefreshed();
    void agentRegistered();
    void agentRemoved(const QString& name);
    void operationFailed(const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    static constexpr auto kCatalogCorrelation = "repo/agent-catalog";
    static constexpr auto kDiscoverCorrelation = "repo/agent-discover";
    static constexpr auto kRegisterCorrelation = "repo/agent-register";
    static constexpr auto kRemoveCorrelation = "repo/agent-remove";

    QList<DecodedAgentEntry> m_entries;
    QString m_lastRemovedName; // echoed on agentRemoved
};

// --- Interactive auth (begin -> challenge/response state machine; app-wizard-auth stream) --------
// The wire seam for the generic interactive-auth contract (daemon-interactive-auth-spec.md, wire
// v31): AuthProviders discovers the families, AuthBegin parks a flow + returns its initial
// AuthChallenge, AuthStep advances the flow one step with the client's AuthStepInput (returning the
// next challenge or completion), AuthCancel drops a flow early. Stateless beyond the provider list
// — the flow state machine lives in auth::AuthFlowController; the wire op names live ONLY here + in
// NodeApiCodec (rename isolation).
class AuthRepository : public RepositoryBase {
    Q_OBJECT

public:
    AuthRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

    [[nodiscard]] const QList<DecodedAuthProviderInfo>& providers() const { return m_providers; }

    // Issue an AuthProviders; on success providersRefreshed() fires with providers() populated.
    void refreshProviders();
    // Begin a flow for `family` with the family-specific `params` map against the CLIENT-owned
    // `redirectUri`. Non-empty `bindProfile` binds the resulting account to that profile
    // node-side. On success begun() fires with the flow handle + initial challenge; on
    // Error/transport failure failed("begin", ...) fires.
    void begin(const QString& family, const QVariantMap& params, const QString& redirectUri,
               const QString& bindProfile = QString());
    // Advance `flowId` one step with the collected input (`kind` selects the arm: Fields carries
    // `fields`, Callback carries `callback`, Poll carries nothing). On success stepped() fires with
    // the next challenge or the completion; on failure failed("step", ...) fires.
    void step(const QString& flowId, AuthStepInputKind kind, const QVariantMap& fields = {},
              const QString& callback = {});
    // Drop a pending flow (user cancelled / TTL abandoned). Fire-and-forget: Ok and errors are
    // both terminal for an already-abandoned flow, so no signal is emitted.
    void cancel(const QString& flowId);

signals:
    void providersRefreshed();
    void begun(const daemonapp::daemon::DecodedAuthBeginResponse& response);
    void stepped(const daemonapp::daemon::DecodedAuthStepResult& result);
    // `phase` is "providers" | "begin" | "step"; `message` is the node's reason (an ApiError
    // message when one decodes, else a transport/decode fallback).
    void failed(const QString& phase, const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);
    // Decode an Error reply into its message (fallback otherwise) and emit failed(phase, ...).
    void emitPhaseError(const QString& phase, const QByteArray& responseCbor,
                        const QString& fallback);

    static constexpr auto kProvidersCorrelation = "repo/auth-providers";
    static constexpr auto kBeginCorrelation = "repo/auth-begin";
    static constexpr auto kStepCorrelation = "repo/auth-step";
    static constexpr auto kCancelCorrelation = "repo/auth-cancel";

    QList<DecodedAuthProviderInfo> m_providers;
};

// --- Routing (B6/ROU: the origin->session pin table; wire v28) --------------------------------
// The routing MUTATION seam (M3 → AD): the node-authoritative RoutingBindChat/RoutingUnbindChat
// verbs behind daemonnet::IRoutingActions. On Ok, mutationApplied() fires and the graph bridges
// it to the ingestor's RoutingListChats refetch — the MIRROR pin table is the only read path
// (the repo's dead in-memory routes/rooms cache — the LEDGER-a6 residual — died with AD).
class RoutingRepository : public RepositoryBase, public daemonnet::IRoutingActions {
    Q_OBJECT

public:
    RoutingRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

    // daemonnet::IRoutingActions (M3): converts the domain::Origin to the codec's DecodedOrigin
    // and sends the RoutingBindChat / RoutingUnbindChat wire op.
    void routingBindChat(const domain::Origin& origin, const domain::SessionId& session,
                         const domain::ProfileRef& profile) override;
    void routingUnbindChat(const domain::Origin& origin) override;

signals:
    // A routing mutation was acked by the node; the mirror pin-table refetch renders the outcome.
    void mutationApplied();
    void operationFailed(const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);
    void handleMutationResponse(const QByteArray& responseCbor);

    static constexpr auto kMutateCorrelation = "repo/routing-mutate";
};

// [wave2:app-approvals-safety] D2: node-wide tool inventory (ToolList -> Tools, wire v29).
// refreshTools() issues a ToolList; on success toolsRefreshed() fires with tools() populated.
// Read-only: the node owns tool gating (there is no enable/disable op at v29). Not cached — a live
// inventory, like ApprovalRepository.
class ToolRepository : public RepositoryBase {
    Q_OBJECT

public:
    ToolRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

    [[nodiscard]] const QList<DecodedToolInfo>& tools() const { return m_tools; }

    void refreshTools();
    // [waveB:app-v30] D4: ask the node to enable/disable a tool (ToolSetEnabled). On Ok the tool
    // list re-fetches so the client renders the node-authoritative overlay result (a force-disabled
    // / build-gated tool stays disabled with its `requires`).
    void setEnabled(const QString& tool, bool enabled);

signals:
    void toolsRefreshed();
    void operationFailed(const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    static constexpr auto kToolsCorrelation = "repo/tool-list";
    static constexpr auto kSetEnabledCorrelation = "repo/tool-set-enabled"; // [waveB:app-v30] D4

    QList<DecodedToolInfo> m_tools;
};

// [wave2:app-approvals-safety] D4: per-session remembered exec-approval fingerprints
// (FingerprintList/FingerprintRevoke, wire v29). refreshFingerprints(session) lists the session's
// allow-list; revoke(session, fp) drops one and, on Ok, re-lists so the client renders the node's
// stored state (never an optimistic local drop). Not cached — a live, per-session query.
class FingerprintRepository : public RepositoryBase {
    Q_OBJECT

public:
    FingerprintRepository(NodeApiClient* client, DaemonCacheStore* cache,
                          QObject* parent = nullptr);

    [[nodiscard]] const QList<DecodedRememberedFingerprint>& fingerprints() const {
        return m_fingerprints;
    }
    [[nodiscard]] QString session() const { return m_session; }

    void refreshFingerprints(const QString& sessionId);
    void revoke(const QString& sessionId, const QString& fingerprint);

signals:
    void fingerprintsRefreshed();
    void operationFailed(const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    static constexpr auto kListCorrelation = "repo/fingerprint-list";
    static constexpr auto kRevokeCorrelation = "repo/fingerprint-revoke";

    QList<DecodedRememberedFingerprint> m_fingerprints;
    QString m_session; // the session scope of the last list, re-used after a revoke
};

// --- Node OpenAI-compatible gateway (Phase F; wire v32) --------------------------------------
// The resident OpenAI-compatible gateway the node runs as a ManagedResource (GatewayGet/GatewaySet
// -> GatewayStatus). Ops-ONLY, following the read-only CapsRepository pattern (no cache): refresh()
// reads the live status (GatewayGet) and setEnabled() toggles it (GatewaySet{enabled, ? addr});
// both replies decode a GatewayStatus, cached here and surfaced through statusChanged. There is NO
// gateway node-event, so health/listening freshness is driven by polling Health (owned by
// DaemonConnectionService) + this GatewayGet — this repository NEVER polls Health itself. When the
// node reports the op Unsupported (an older peer without the gateway), `supported` flips false so
// the surfaces hide/disable gracefully. Never surfaces gateway tokens/credentials (the wire carries
// none). Kept in its own region so it merges cleanly beside the provenance sibling's CapsRepository
// edits.
class GatewayRepository : public RepositoryBase {
    Q_OBJECT
    Q_PROPERTY(bool loaded READ loaded NOTIFY statusChanged)
    Q_PROPERTY(bool supported READ supported NOTIFY statusChanged)
    Q_PROPERTY(bool enabled READ enabled NOTIFY statusChanged)
    Q_PROPERTY(bool listening READ listening NOTIFY statusChanged)
    Q_PROPERTY(QString addr READ addr NOTIFY statusChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY statusChanged)

public:
    explicit GatewayRepository(NodeApiClient* client, QObject* parent = nullptr);

    [[nodiscard]] bool loaded() const { return m_loaded; }
    [[nodiscard]] bool supported() const { return m_supported; }
    [[nodiscard]] bool enabled() const { return m_status.enabled; }
    [[nodiscard]] bool listening() const { return m_status.listening; }
    [[nodiscard]] QString addr() const { return m_status.hasAddr ? m_status.addr : QString(); }
    [[nodiscard]] QString lastError() const {
        return m_status.hasLastError ? m_status.lastError : QString();
    }
    [[nodiscard]] const DecodedGatewayStatus& status() const { return m_status; }

    // Read the gateway's live status (GatewayGet -> GatewayStatus). On success statusChanged()
    // fires; an Unsupported error flips supported() false (also via statusChanged).
    Q_INVOKABLE void refresh();
    // Toggle the gateway on/off (GatewaySet{enabled, ? addr}). An empty `addr` keeps the node's
    // own bind; a non-empty one sets it. The reply is a GatewayStatus (statusChanged); an error
    // surfaces operationFailed (never a silent no-op).
    Q_INVOKABLE void setEnabled(bool enabled, const QString& addr = QString());

signals:
    void statusChanged();
    void operationFailed(const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);
    // Shared reply decode for GatewayGet/GatewaySet: an Unsupported error on a GatewayGet marks the
    // gateway unsupported (statusChanged, no failure); any other error emits operationFailed; a
    // GatewayStatus updates the cache + emits statusChanged.
    void applyStatusReply(const QByteArray& responseCbor, bool isGet);

    static constexpr auto kGetCorrelation = "repo/gateway-get";
    static constexpr auto kSetCorrelation = "repo/gateway-set";

    DecodedGatewayStatus m_status;
    bool m_loaded = false;
    bool m_supported = true;
};

} // namespace daemonapp::daemon
