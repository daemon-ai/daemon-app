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
    bool appendCachedLog(const CachedLogRow& row);
    [[nodiscard]] QList<CachedLogRow> cachedLog(const QString& sessionId, quint64 afterSeq = 0,
                                                int limit = 0) const;
    bool setLogCursor(const QString& sessionId, quint64 seq);
    [[nodiscard]] quint64 logCursor(const QString& sessionId) const;

    // Issue a SessionsQuery; on success the cache is updated and sessionsRefreshed() fires.
    void refreshSessions();
    // Subscribe to a session's merged log from its persisted cursor; on success the new entries are
    // appended to daemon_session_log, the cursor is advanced, and logUpdated(sessionId) fires.
    void subscribe(const QString& sessionId);

signals:
    void sessionsRefreshed();
    void refreshFailed(const QString& message);
    void logUpdated(const QString& sessionId);

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void handleFailure(const QString& correlationId, const QString& message);
    [[nodiscard]] static QString subscribeCorrelation(const QString& sessionId);

    static constexpr auto kSessionsCorrelation = "repo/sessions-query";
    static constexpr auto kSubscribePrefix = "repo/subscribe/";
};

class ProfileRepository : public RepositoryBase {
    Q_OBJECT

public:
    ProfileRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent = nullptr);

    bool upsertCachedProfile(const CachedProfileRow& row);
    [[nodiscard]] QList<CachedProfileRow> cachedProfiles() const;

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

signals:
    void profilesRefreshed();
    void refreshFailed(const QString& message);
    void operationFailed(const QString& message);
    // A ProfileGet completed and cachedSpec(id) is now populated.
    void profileSpecLoaded(const QString& id);

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
    // Completed job that was not completed before triggers a catalog refresh.
    void refreshDownloads();
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
    // Ensure the QTimer poll loop runs while any download job is unsettled; stop it once idle.
    void syncDownloadPolling();

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
    QTimer* m_downloadPoll = nullptr;
};

class FsRepository : public RepositoryBase {
public:
    using RepositoryBase::RepositoryBase;

    bool upsertCachedEntry(const CachedFsEntryRow& row);
    [[nodiscard]] QList<CachedFsEntryRow> cachedEntries(const QString& rootId) const;
};

class ApprovalRepository : public RepositoryBase {
public:
    using RepositoryBase::RepositoryBase;

    bool upsertCachedApproval(const CachedApprovalRow& row);
    [[nodiscard]] QList<CachedApprovalRow> cachedApprovals() const;
};

// Not part of the first daemon slice: kept as a cache/NodeApi-aware stub until checkpoint
// timelines are modeled in the daemon-api codec subset.
class CheckpointRepository : public RepositoryBase {
public:
    using RepositoryBase::RepositoryBase;
};

} // namespace daemonapp::daemon
