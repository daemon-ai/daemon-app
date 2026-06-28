#pragma once

#include "daemon/daemon_cache_store.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>

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

signals:
    void sessionsRefreshed();
    void refreshFailed(const QString& message);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);

    static constexpr auto kSessionsCorrelation = "repo/sessions-query";
    static constexpr auto kRosterRevScope = "roster-rev"; // L4 persisted roster revision

    // L4: the since_rev the in-flight SessionsQuery carried (0 = a full request), so the response
    // handler knows whether to merge a delta or replace the roster (and detects a daemon-reset
    // fallback when the returned rev went backwards).
    bool m_sentRosterDelta = false;
    quint64 m_sentRosterSinceRev = 0;
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

    // Step 1: search HF repos (ModelSearch); on success searchHitsChanged() fires.
    void search(const QString& text, const QString& engine = QStringLiteral("llama"),
                const QString& sort = QStringLiteral("trending"));
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
    // newly reaches Completed. Replaces the retired 600ms poll.
    void applyDownloadProgress(quint64 id, quint32 pct, const QString& state);
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

    QList<DecodedSearchHit> m_searchHits;
    QList<DecodedModelFile> m_files;
    QString m_filesRepo;
    QString m_pendingFilesRepo;
    DecodedQuantRecommendation m_recommendation;
    bool m_hasRecommendation = false;
    QString m_pendingRecommendRepo;
    QList<DecodedDownloadStatus> m_downloads;
    QList<DecodedInstalledModel> m_installed;
    QSet<quint64> m_completedDownloads; // ids already seen Completed (debounces catalog refresh)
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

    void refreshTree();
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

    static constexpr auto kTreeCorrelation = "repo/tree";
    static constexpr auto kControlCorrelation = "repo/unit-control";
};

// Not part of the first daemon slice: kept as a cache/NodeApi-aware stub until checkpoint
// timelines are modeled in the daemon-api codec subset.
class CheckpointRepository : public RepositoryBase {
public:
    using RepositoryBase::RepositoryBase;
};

} // namespace daemonapp::daemon
