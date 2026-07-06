// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "daemon/daemon_cache_store.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon/page_loop.h"

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

// Sessions are the first live daemon slice: refreshSessions() issues a SessionsQuery via the
// NodeApiClient, the response is decoded by NodeApiCodec, and the resulting rows are written into
// the DaemonCacheStore. UI/adapters read the cache, never the wire.
class SessionRepository : public RepositoryBase {
    Q_OBJECT

public:
    SessionRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

    bool upsertCachedSession(const CachedSessionRow& row);
    [[nodiscard]] QList<CachedSessionRow> cachedSessions() const;

    // Issue a SessionsQuery; on success the cache is updated and sessionsRefreshed() fires.
    void refreshSessions();
    // Issue a SessionsQuery scoped to `SessionScope::ByProfile(profileId)` — the per-agent view
    // (Fleet membership, daemon-supervision-spec §0). The returned rows are merged into the cache
    // ADDITIVELY (no prune), so an agent-scoped fetch augments the roster without clobbering it;
    // sessionsRefreshed() fires so the sidebar/list re-project. Encoder-only (the ByProfile arm is
    // already in the CDDL); no contract change.
    void refreshSessionsByProfile(const QString& profileId);

    // Node-authoritative session creation (WireVersion v23): ask the node to create a blank,
    // profile-bound, UN-RUN session (SessionCreate{profile}). The node MINTS the id and replies
    // SessionCreated{session} + emits RosterChanged; on the reply, sessionCreated(sessionId,
    // profileId) fires so the caller can update + auto-select event-driven. `profileId` empty =
    // the node's active default. Nothing is client-minted.
    void createSession(const QString& profileId = QString());

    // Node-authoritative session-metadata patch (SessionUpdateMeta{session, patch} -> Ok). Each
    // optional is Some=set, std::nullopt=leave unchanged. NOTHING is written client-side: on Ok the
    // node emits SessionMetaChanged (debounced roster refetch) and we ALSO issue an immediate
    // authoritative refreshSessions() so the pin/archive/title reflects the node's stored row. On
    // Error/transport failure metaUpdateFailed() fires so a rejected change (e.g. Forbidden for a
    // non-owner) is surfaced, never silently mistaken for applied.
    void updateSessionMeta(const QString& sessionId, std::optional<bool> pinned,
                           std::optional<bool> archived, std::optional<QString> title);

signals:
    void sessionsRefreshed();
    void refreshFailed(const QString& message);
    // A SessionUpdateMeta was rejected (node ApiError) or failed at the transport. `sessionId`
    // echoes the target; `message` is the node's reason. The UI surfaces this (GUI toast / TUI
    // notification) instead of leaving a silent no-op that looks applied.
    void metaUpdateFailed(const QString& sessionId, const QString& message);
    // A node SessionCreate resolved: `sessionId` is the node-minted id, `profileId` the profile it
    // was bound under (echoed from the request so the auto-select path knows the agent).
    void sessionCreated(const QString& sessionId, const QString& profileId);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    // SessionPage handling, split out of handleResponse so the dispatcher stays flat and the
    // replace/delta cache reconciliation is not nested four deep.
    void applySessionPage(const QByteArray& responseCbor);
    // ByProfile page handling: decode + additive upsert (no prune), then emit sessionsRefreshed().
    void applyByProfilePage(const QByteArray& responseCbor);
    void pruneSessionsMissingFrom(const QList<CachedSessionRow>& rows);
    void pruneRemovedSessions(const QList<QString>& removed);

    // SessionCreated handling: node-authoritative create reply -> emit sessionCreated.
    void applySessionCreated(const QByteArray& responseCbor);
    // SessionUpdateMeta reply: Ok -> authoritative refreshSessions(); Error -> metaUpdateFailed.
    void applyMetaUpdate(const QByteArray& responseCbor);

    static constexpr auto kSessionsCorrelation = "repo/sessions-query";
    static constexpr auto kByProfileCorrelation = "repo/sessions-by-profile";
    static constexpr auto kCreateCorrelation = "repo/session-create";
    static constexpr auto kUpdateMetaCorrelation = "repo/session-update-meta";
    static constexpr auto kRosterRevScope = "roster-rev"; // L4 persisted roster revision

    // The profile the in-flight SessionCreate carried, echoed on sessionCreated so the caller's
    // auto-select knows which agent the new session belongs to.
    QString m_pendingCreateProfile;

    // The session id the in-flight SessionUpdateMeta targets, echoed on metaUpdateFailed so the UI
    // can name the affected row.
    QString m_pendingMetaSession;

    // L4: the since_rev the in-flight SessionsQuery carried (0 = a full request), so the response
    // handler knows whether to merge a delta or replace the roster (and detects a daemon-reset
    // fallback when the returned rev went backwards).
    bool m_sentRosterDelta = false;
    quint64 m_sentRosterSinceRev = 0;

    // Roster page loops (the shared PageLoop accumulator): a full refresh accumulates rows across
    // next_cursor pages and runs the replace + prune ONCE over the whole set (pruning against a
    // single page would drop every session beyond it). Delta reads stay single-page. ByProfile
    // loops too, but merges per page (additive, so incremental application is safe — its loop
    // carries only the runaway guard); it needs the profile id to re-issue the continuation.
    PageLoop<CachedSessionRow> m_rosterLoop;
    PageLoop<CachedSessionRow> m_byProfileLoop; // guard-only (pages merge incrementally)
    QString m_byProfileId;
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
    static constexpr auto kExportPrefix = "repo/profile-export/";
    static constexpr auto kImportCorrelation = "repo/profile-import";
    static constexpr auto kHistoryPrefix = "repo/profile-history/";
    static constexpr auto kRevertPrefix = "repo/profile-revert/";

    QList<DecodedProfileInfo> m_profiles;
    QHash<QString, DecodedProfileSpec> m_specs;
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

signals:
    void providersRefreshed();
    void providerModelsRefreshed(const QString& provider);
    void operationFailed(const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    static constexpr auto kCatalogCorrelation = "repo/provider-catalog";
    static constexpr auto kModelsPrefix = "repo/provider-models/";

    QList<DecodedProviderDescriptor> m_providers;
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
    // fires.
    void decide(const QString& sessionId, const QString& requestId, bool allow);

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

// The agent fleet / subagent tree (PRO-9/10): refreshTree() issues a Tree query, decode() flattens
// it into daemon_fleet_units (offline-first cache), and treeRefreshed() fires. pause/resume/scale
// issue the control op; on Ok the tree is re-fetched, on Unsupported/error controlFailed() fires
// (PRO-10: control is only meaningful on orchestrator units; engine leaves return Unsupported).
class FleetRepository : public RepositoryBase {
    Q_OBJECT

public:
    FleetRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

    [[nodiscard]] QList<CachedFleetUnitRow> cachedUnits() const;

    // `source` labels why the tree is being re-queried (baseline/control/subscription/request); it
    // is used only for debug instrumentation (FIX 2).
    void refreshTree(const QString& source = QStringLiteral("request"));
    void pause(const QString& unitId);
    void resume(const QString& unitId);
    void scale(const QString& unitId, quint32 n);

signals:
    void treeRefreshed();
    void refreshFailed(const QString& message);
    void controlFailed(const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    void handleTreeResponse(const QByteArray& responseCbor);
    void handleUnitControlResponse(const QByteArray& responseCbor);
    void syncFleetUnits(const QList<DecodedUnitNode>& flat);

    static constexpr auto kTreeCorrelation = "repo/tree";
    static constexpr auto kControlCorrelation = "repo/unit-control";

    // Tree page loop (wire v25): accumulate raw nodes across `after` pages (`root` rides every
    // page; the first one fixes it), then flatten + sync ONCE over the whole union.
    PageLoop<DecodedUnitNode> m_treeLoop;
    QString m_treeRoot;
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

signals:
    void adaptersRefreshed();
    void instancesRefreshed();
    void conversationsRefreshed(const QString& transport);
    void operationFailed(const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    void handleAdaptersResponse(const QByteArray& responseCbor);
    void handleInstancesResponse(const QByteArray& responseCbor);
    void handleConversationsResponse(const QString& transport, const QByteArray& responseCbor);
    void syncTransportInstances(const QList<DecodedTransportInstance>& live);
    void syncConversations(const QString& transport, const QList<DecodedConversation>& live);
    [[nodiscard]] static bool isOwnCorrelation(const QString& correlationId);

    QList<DecodedAdapterInfo> m_adapters; // in-memory (connect-only); not cached
    // Per-transport conversations page loop (wire v25): accumulate across `after` pages, then
    // sync (replace + prune) ONCE over the whole listing — pruning against a single page would
    // drop every conversation beyond it.
    QHash<QString, PageLoop<DecodedConversation>> m_convLoops;

    static constexpr auto kAdaptersCorrelation = "repo/transport-adapters";
    static constexpr auto kInstancesCorrelation = "repo/transport-instances";
    static constexpr auto kConvPrefix = "repo/conv-list/";
};

// Not part of the first daemon slice: kept as a cache/NodeApi-aware stub until checkpoint
// timelines are modeled in the daemon-api codec subset.
class CheckpointRepository : public RepositoryBase {
public:
    using RepositoryBase::RepositoryBase;
};

// --- ACP agent catalog (foreign engines; wire v23) -------------------------------------------
// The node's ACP agent catalog (AcpCatalog: durable manual registrations + the last discovery
// scan), kept in memory for the new-agent dialog's engine picker. Read-only: registration /
// removal stay operator ops (no recipe ever travels through the client); a profile references an
// entry BY NAME via the ProfileSpec engine selector.
class AcpRepository : public RepositoryBase {
    Q_OBJECT

public:
    AcpRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

    [[nodiscard]] const QList<DecodedAcpAgentEntry>& entries() const { return m_entries; }
    // QML-consumable rows for the engine picker: {name, source, installed, version}.
    [[nodiscard]] Q_INVOKABLE QVariantList agents() const;

    // Issue an AcpCatalog; on success catalogRefreshed() fires with entries() populated.
    Q_INVOKABLE void refreshCatalog();

signals:
    void catalogRefreshed();
    void operationFailed(const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    static constexpr auto kCatalogCorrelation = "repo/acp-catalog";

    QList<DecodedAcpAgentEntry> m_entries;
};

} // namespace daemonapp::daemon
