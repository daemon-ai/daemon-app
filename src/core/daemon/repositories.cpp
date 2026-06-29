#include "daemon/repositories.h"

#include "daemon/node_api_codec.h"

#include <QDateTime>
#include <QSet>
#include <QStringList>
#include <QTimer>

namespace daemonapp::daemon {

RepositoryBase::RepositoryBase(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent)
    : QObject(parent), m_client(client), m_cache(cache) {}

SessionRepository::SessionRepository(NodeApiClient* client, DaemonCacheStore* cache,
                                     QObject* parent)
    : RepositoryBase(client, cache, parent) {
    if (this->client() != nullptr) {
        connect(this->client(), &NodeApiClient::responseReady, this,
                &SessionRepository::handleResponse);
        connect(this->client(), &NodeApiClient::failed, this, &SessionRepository::handleFailure);
    }
}

bool SessionRepository::upsertCachedSession(const CachedSessionRow& row) {
    return cache() != nullptr && cache()->upsertSession(row);
}

QList<CachedSessionRow> SessionRepository::cachedSessions() const {
    return cache() != nullptr ? cache()->sessions() : QList<CachedSessionRow>{};
}

void SessionRepository::refreshSessions() {
    if (client() == nullptr) {
        emit refreshFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    // L4: resume from the persisted roster revision so a warm reconnect / RosterChanged pulls only
    // the delta (changed sessions + a removed list) instead of the whole roster. A cold cache (no
    // persisted rev) sends no since_rev -> a full page.
    m_sentRosterSinceRev = 0;
    if (cache() != nullptr) {
        const QString stored = cache()->cursor(QLatin1String(kRosterRevScope));
        if (!stored.isEmpty()) {
            m_sentRosterSinceRev = stored.toULongLong();
        }
    }
    m_sentRosterDelta = m_sentRosterSinceRev > 0;
    client()->sendRequest(
        NodeApiCodec::encodeSessionsQueryRequest(m_sentRosterDelta, m_sentRosterSinceRev),
        QLatin1String(kSessionsCorrelation));
}

void SessionRepository::handleResponse(const QString& correlationId,
                                       const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kSessionsCorrelation)) {
        QList<CachedSessionRow> rows;
        quint64 rev = 0;
        QStringList removed;
        if (!NodeApiCodec::decodeSessionPage(responseCbor, &rows, nullptr, &rev, &removed)) {
            emit refreshFailed(QStringLiteral("Failed to decode SessionPage response"));
            return;
        }
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        // L4: a full page replaces the roster; a delta merges. A delta whose returned rev went
        // *backwards* from what we asked for is a daemon-reset fallback (the server returned a full
        // page because our since_rev was unservable) -> treat it as a replace too.
        const bool fallbackFull = m_sentRosterDelta && rev < m_sentRosterSinceRev;
        const bool replace = !m_sentRosterDelta || fallbackFull;
        if (replace && cache() != nullptr) {
            // Prune any cached session the full page no longer lists (closes the "removed sessions
            // never purged" gap on a cold/fallback resync).
            QSet<QString> keep;
            for (const CachedSessionRow& row : rows) {
                keep.insert(row.sessionId);
            }
            for (const CachedSessionRow& existing : cache()->sessions()) {
                if (!keep.contains(existing.sessionId)) {
                    cache()->deleteSession(existing.sessionId);
                }
            }
        }
        for (CachedSessionRow& row : rows) {
            row.updatedAtMs = now;
            upsertCachedSession(row);
        }
        // L4 delta prune: drop sessions the server reported removed (hard-removed or left the
        // scope).
        if (!replace && cache() != nullptr) {
            for (const QString& id : removed) {
                cache()->deleteSession(id);
            }
        }
        if (cache() != nullptr) {
            // Persist the roster revision so the next refresh resumes as a delta.
            cache()->setCursor(QLatin1String(kRosterRevScope), QString::number(rev), now);
        }
        emit sessionsRefreshed();
        return;
    }
}

void SessionRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kSessionsCorrelation)) {
        emit refreshFailed(message);
    }
}

// --- CredentialRepository (CON-4) -----------------------------------------------------------

CredentialRepository::CredentialRepository(NodeApiClient* client, DaemonCacheStore* cache,
                                           QObject* parent)
    : RepositoryBase(client, cache, parent) {
    if (this->client() != nullptr) {
        connect(this->client(), &NodeApiClient::responseReady, this,
                &CredentialRepository::handleResponse);
        connect(this->client(), &NodeApiClient::failed, this, &CredentialRepository::handleFailure);
    }
}

void CredentialRepository::refreshList() {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeCredentialListRequest(),
                          QLatin1String(kListCorrelation));
}

void CredentialRepository::setCredential(const QString& profile, const QString& secret) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeCredentialSetRequest(profile, secret),
                          QLatin1String(kSetCorrelation));
}

void CredentialRepository::removeCredential(const QString& profile) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeCredentialRemoveRequest(profile),
                          QLatin1String(kRemoveCorrelation));
}

void CredentialRepository::handleResponse(const QString& correlationId,
                                          const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kListCorrelation)) {
        if (!NodeApiCodec::decodeCredentials(responseCbor, &m_credentials)) {
            emit operationFailed(QStringLiteral("Failed to decode Credentials response"));
            return;
        }
        emit listRefreshed();
        return;
    }
    if (correlationId == QLatin1String(kSetCorrelation) ||
        correlationId == QLatin1String(kRemoveCorrelation)) {
        const ApiResponseKind kind = NodeApiCodec::responseKind(responseCbor);
        if (kind == ApiResponseKind::Ok) {
            refreshList(); // reflect the mutation in the redacted view
            return;
        }
        DecodedApiError err;
        if (kind == ApiResponseKind::Error && NodeApiCodec::decodeError(responseCbor, &err)) {
            emit operationFailed(err.message);
        } else {
            emit operationFailed(QStringLiteral("Credential operation failed"));
        }
        return;
    }
}

void CredentialRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kListCorrelation) ||
        correlationId == QLatin1String(kSetCorrelation) ||
        correlationId == QLatin1String(kRemoveCorrelation)) {
        emit operationFailed(message);
    }
}

// --- ModelRepository (CON-6) ----------------------------------------------------------------

ModelRepository::ModelRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent)
    : RepositoryBase(client, cache, parent) {
    if (this->client() != nullptr) {
        connect(this->client(), &NodeApiClient::responseReady, this,
                &ModelRepository::handleResponse);
        connect(this->client(), &NodeApiClient::failed, this, &ModelRepository::handleFailure);
    }
}

void ModelRepository::refreshModels() {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeModelsRequest(), QLatin1String(kModelsCorrelation));
}

void ModelRepository::refreshCurrent(const QString& profile) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeModelCurrentRequest(profile),
                          QLatin1String(kCurrentCorrelation));
}

void ModelRepository::setSessionModel(const QString& sessionId, const QString& model,
                                      const QString& provider) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeSetSessionModelRequest(sessionId, model, provider),
                          QLatin1String(kSetModelCorrelation));
}

void ModelRepository::search(const QString& text, const QString& engine, const QString& sort) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeModelSearchRequest(text, engine, sort),
                          QLatin1String(kSearchCorrelation));
}

void ModelRepository::requestFiles(const QString& repo, const QString& engine) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    m_pendingFilesRepo = repo;
    client()->sendRequest(NodeApiCodec::encodeModelFilesRequest(repo, engine),
                          QLatin1String(kFilesCorrelation));
}

void ModelRepository::recommend(const QString& repo, const QString& engine, bool hasBudget,
                                quint64 budgetBytes) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    m_pendingRecommendRepo = repo;
    client()->sendRequest(
        NodeApiCodec::encodeModelRecommendRequest(repo, engine, hasBudget, budgetBytes),
        QLatin1String(kRecommendCorrelation));
}

void ModelRepository::download(const QString& repo, const QString& file, const QString& engine,
                               const QString& revision) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeModelDownloadRequest(repo, file, engine, revision),
                          QLatin1String(kDownloadCorrelation));
}

void ModelRepository::refreshDownloads() {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeModelDownloadsRequest(),
                          QLatin1String(kDownloadsCorrelation));
}

void ModelRepository::refreshCatalog() {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeModelCatalogRequest(),
                          QLatin1String(kCatalogCorrelation));
}

void ModelRepository::deleteModel(const QString& id) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeModelDeleteRequest(id),
                          QLatin1String(kDeleteCorrelation));
}

void ModelRepository::activate(const QString& id, const QString& profile) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeModelActivateRequest(id, profile),
                          QLatin1String(kActivateCorrelation));
}

void ModelRepository::cancelDownload(quint64 id) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeModelCancelRequest(id),
                          QLatin1String(kLifecycleCorrelation));
}

void ModelRepository::pauseDownload(quint64 id) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeModelPauseRequest(id),
                          QLatin1String(kLifecycleCorrelation));
}

void ModelRepository::resumeDownload(quint64 id) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeModelResumeRequest(id),
                          QLatin1String(kLifecycleCorrelation));
}

void ModelRepository::applyDownloadProgress(quint64 id, quint32 pct, const QString& state) {
    // Patch the matching job row in place (or insert a fresh one), so the Downloads view tracks
    // progress from the push feed instead of a poll. We only carry (id, pct, state) here; derive
    // downloaded_bytes from pct when a total is known so the existing progress bar keeps working.
    bool found = false;
    for (DecodedDownloadStatus& s : m_downloads) {
        if (s.id == id) {
            s.state = state;
            if (s.totalBytes > 0) {
                s.downloadedBytes = (s.totalBytes * pct) / 100;
            }
            found = true;
            break;
        }
    }
    if (!found) {
        DecodedDownloadStatus s;
        s.id = id;
        s.state = state;
        m_downloads.append(s);
    }
    bool newlyCompleted = false;
    if (state == QStringLiteral("Completed") && !m_completedDownloads.contains(id)) {
        m_completedDownloads.insert(id);
        newlyCompleted = true;
    }
    emit downloadsChanged();
    if (newlyCompleted) {
        refreshCatalog();
    }
}

void ModelRepository::handleResponse(const QString& correlationId, const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kModelsCorrelation)) {
        if (!NodeApiCodec::decodeModels(responseCbor, &m_models)) {
            emit operationFailed(QStringLiteral("Failed to decode Models response"));
            return;
        }
        emit modelsRefreshed();
        return;
    }
    if (correlationId == QLatin1String(kCurrentCorrelation)) {
        if (!NodeApiCodec::decodeModelCurrent(responseCbor, &m_current, &m_hasCurrent)) {
            emit operationFailed(QStringLiteral("Failed to decode ModelCurrent response"));
            return;
        }
        emit currentRefreshed();
        return;
    }
    if (correlationId == QLatin1String(kSetModelCorrelation)) {
        const ApiResponseKind kind = NodeApiCodec::responseKind(responseCbor);
        if (kind == ApiResponseKind::Ok) {
            emit modelSet();
            return;
        }
        DecodedApiError err;
        if (kind == ApiResponseKind::Error && NodeApiCodec::decodeError(responseCbor, &err)) {
            emit operationFailed(err.message);
        } else {
            emit operationFailed(QStringLiteral("Set-model operation failed"));
        }
        return;
    }
    if (correlationId == QLatin1String(kSearchCorrelation)) {
        if (!NodeApiCodec::decodeModelSearch(responseCbor, &m_searchHits)) {
            emit operationFailed(QStringLiteral("Failed to decode ModelSearch response"));
            return;
        }
        emit searchHitsChanged();
        return;
    }
    if (correlationId == QLatin1String(kFilesCorrelation)) {
        if (!NodeApiCodec::decodeModelFiles(responseCbor, &m_files)) {
            emit operationFailed(QStringLiteral("Failed to decode ModelFiles response"));
            return;
        }
        m_filesRepo = m_pendingFilesRepo;
        emit filesLoaded(m_filesRepo);
        return;
    }
    if (correlationId == QLatin1String(kRecommendCorrelation)) {
        if (!NodeApiCodec::decodeModelRecommend(responseCbor, &m_recommendation)) {
            emit operationFailed(QStringLiteral("Failed to decode ModelRecommend response"));
            return;
        }
        m_hasRecommendation = true;
        emit recommendLoaded(m_pendingRecommendRepo);
        return;
    }
    if (correlationId == QLatin1String(kDownloadCorrelation)) {
        quint64 id = 0;
        if (NodeApiCodec::decodeModelDownloadStarted(responseCbor, &id)) {
            emit downloadStarted(id);
            refreshDownloads(); // fetch the initial snapshot; live progress arrives via the L3 feed
            return;
        }
        DecodedApiError err;
        if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Error &&
            NodeApiCodec::decodeError(responseCbor, &err)) {
            emit operationFailed(err.message);
        } else {
            emit operationFailed(QStringLiteral("Download could not be started"));
        }
        return;
    }
    if (correlationId == QLatin1String(kDownloadsCorrelation)) {
        if (!NodeApiCodec::decodeModelDownloads(responseCbor, &m_downloads)) {
            emit operationFailed(QStringLiteral("Failed to decode ModelDownloads response"));
            return;
        }
        // Refresh the catalog once for each job that newly reaches Completed.
        bool newlyCompleted = false;
        for (const DecodedDownloadStatus& s : m_downloads) {
            if (s.state == QStringLiteral("Completed") && !m_completedDownloads.contains(s.id)) {
                m_completedDownloads.insert(s.id);
                newlyCompleted = true;
            }
        }
        emit downloadsChanged();
        if (newlyCompleted) {
            refreshCatalog();
        }
        return;
    }
    if (correlationId == QLatin1String(kCatalogCorrelation)) {
        if (!NodeApiCodec::decodeModelCatalog(responseCbor, &m_installed)) {
            emit operationFailed(QStringLiteral("Failed to decode ModelCatalog response"));
            return;
        }
        emit catalogChanged();
        return;
    }
    if (correlationId == QLatin1String(kDeleteCorrelation)) {
        if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Ok) {
            refreshCatalog();
            return;
        }
        emit operationFailed(QStringLiteral("Delete failed"));
        return;
    }
    if (correlationId == QLatin1String(kActivateCorrelation)) {
        if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Ok) {
            emit modelActivated();
            return;
        }
        DecodedApiError err;
        if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Error &&
            NodeApiCodec::decodeError(responseCbor, &err)) {
            emit operationFailed(err.message);
        } else {
            emit operationFailed(QStringLiteral("Activate failed"));
        }
        return;
    }
    if (correlationId == QLatin1String(kLifecycleCorrelation)) {
        // Cancel/Pause/Resume Ok (or Error) -> re-poll so the new state surfaces.
        refreshDownloads();
        return;
    }
}

void ModelRepository::handleFailure(const QString& correlationId, const QString& message) {
    static const QSet<QString> kOurs = {
        QString::fromLatin1(kModelsCorrelation),   QString::fromLatin1(kCurrentCorrelation),
        QString::fromLatin1(kSetModelCorrelation), QString::fromLatin1(kSearchCorrelation),
        QString::fromLatin1(kFilesCorrelation),    QString::fromLatin1(kRecommendCorrelation),
        QString::fromLatin1(kDownloadCorrelation), QString::fromLatin1(kDownloadsCorrelation),
        QString::fromLatin1(kCatalogCorrelation),  QString::fromLatin1(kDeleteCorrelation),
        QString::fromLatin1(kActivateCorrelation), QString::fromLatin1(kLifecycleCorrelation),
    };
    if (kOurs.contains(correlationId)) {
        emit operationFailed(message);
    }
}

ProfileRepository::ProfileRepository(NodeApiClient* client, DaemonCacheStore* cache,
                                     QObject* parent)
    : RepositoryBase(client, cache, parent) {
    if (this->client() != nullptr) {
        connect(this->client(), &NodeApiClient::responseReady, this,
                &ProfileRepository::handleResponse);
        connect(this->client(), &NodeApiClient::failed, this, &ProfileRepository::handleFailure);
        // Forward the handshake's capability set so the store can gate versioning proactively.
        connect(this->client(), &NodeApiClient::handshakeReady, this,
                &ProfileRepository::capabilitiesChanged);
    }
}

bool ProfileRepository::daemonSupportsVersioning() const {
    return client() != nullptr && client()->hasFeature(QStringLiteral("versioning"));
}

bool ProfileRepository::upsertCachedProfile(const CachedProfileRow& row) {
    return cache() != nullptr && cache()->upsertProfile(row);
}

QList<CachedProfileRow> ProfileRepository::cachedProfiles() const {
    return cache() != nullptr ? cache()->profiles() : QList<CachedProfileRow>{};
}

void ProfileRepository::loadCachedProfiles() {
    if (cache() == nullptr) {
        return;
    }
    m_profiles.clear();
    m_specs.clear();
    for (const CachedProfileRow& row : cachedProfiles()) {
        DecodedProfileInfo info;
        info.id = row.profileRef;
        info.isActive = row.active;
        // The persisted spec_cbor is the raw ProfileGet response; decode it to recover
        // provider/model (for the list row) + the full spec (for the editor) offline.
        DecodedProfileSpec spec;
        bool found = false;
        if (!row.specCbor.isEmpty() && NodeApiCodec::decodeProfile(row.specCbor, &spec, &found) &&
            found) {
            info.provider = spec.provider;
            info.model = spec.model;
            m_specs.insert(row.profileRef, spec);
        }
        m_profiles.append(info);
    }
}

QString ProfileRepository::activeProfileId() const {
    for (const DecodedProfileInfo& p : m_profiles) {
        if (p.isActive) {
            return p.id;
        }
    }
    return m_profiles.isEmpty() ? QString() : m_profiles.first().id;
}

void ProfileRepository::refreshProfiles() {
    if (client() == nullptr) {
        emit refreshFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeProfileListRequest(),
                          QLatin1String(kProfilesCorrelation));
}

void ProfileRepository::selectProfile(const QString& id) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeProfileSelectRequest(id),
                          QLatin1String(kSelectCorrelation));
}

void ProfileRepository::deleteProfile(const QString& id) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeProfileDeleteRequest(id),
                          QLatin1String(kDeleteCorrelation));
}

bool ProfileRepository::cachedSpec(const QString& id, DecodedProfileSpec* out) const {
    const auto it = m_specs.constFind(id);
    if (it == m_specs.constEnd()) {
        return false;
    }
    if (out != nullptr) {
        *out = it.value();
    }
    return true;
}

void ProfileRepository::createProfile(const DecodedProfileSpec& spec) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeProfileCreateRequest(spec),
                          QLatin1String(kCreateCorrelation));
}

void ProfileRepository::updateProfile(const DecodedProfileSpec& spec) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeProfileUpdateRequest(spec),
                          QLatin1String(kUpdateCorrelation));
}

void ProfileRepository::cloneProfile(const QString& source, const QString& newId) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeProfileCloneRequest(source, newId),
                          QLatin1String(kCloneCorrelation));
}

void ProfileRepository::getProfile(const QString& id) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeProfileGetRequest(id),
                          QLatin1String(kGetPrefix) + id);
}

void ProfileRepository::exportProfile(const QString& id) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeProfileExportRequest(id),
                          QLatin1String(kExportPrefix) + id);
}

void ProfileRepository::importProfile(const DecodedDistribution& dist, const QString& newId) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeProfileImportRequest(dist, newId),
                          QLatin1String(kImportCorrelation));
}

void ProfileRepository::fetchProfileHistory(const QString& id) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeProfileHistoryRequest(id),
                          QLatin1String(kHistoryPrefix) + id);
}

void ProfileRepository::revertProfile(const QString& id, quint64 seq) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeProfileRevertRequest(id, seq),
                          QLatin1String(kRevertPrefix) + id);
}

void ProfileRepository::handleResponse(const QString& correlationId,
                                       const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kProfilesCorrelation)) {
        if (!NodeApiCodec::decodeProfiles(responseCbor, &m_profiles)) {
            emit refreshFailed(QStringLiteral("Failed to decode Profiles response"));
            return;
        }
        // Offline-first prune: drop any cached profile the live list no longer carries (a deleted
        // agent) so the cache mirrors truth. Surviving rows keep their spec_cbor; the per-profile
        // ProfileGet that the store kicks next refreshes each row's spec + active flag.
        if (cache() != nullptr) {
            QSet<QString> keep;
            for (const DecodedProfileInfo& p : m_profiles) {
                keep.insert(p.id);
            }
            for (const CachedProfileRow& row : cachedProfiles()) {
                if (!keep.contains(row.profileRef)) {
                    cache()->deleteProfile(row.profileRef);
                }
            }
        }
        emit profilesRefreshed();
        return;
    }
    if (correlationId.startsWith(QLatin1String(kGetPrefix))) {
        const QString id = correlationId.mid(int(qstrlen(kGetPrefix)));
        DecodedProfileSpec spec;
        bool found = false;
        if (NodeApiCodec::decodeProfile(responseCbor, &spec, &found) && found) {
            m_specs.insert(id, spec);
            // Offline-first: persist the full profile (raw ProfileGet bytes + the active flag from
            // the list) so the Profiles UI renders this agent from cache on a later offline start.
            if (cache() != nullptr) {
                CachedProfileRow row;
                row.profileRef = id;
                row.displayName = id;
                row.specCbor = responseCbor;
                for (const DecodedProfileInfo& p : m_profiles) {
                    if (p.id == id) {
                        row.active = p.isActive;
                        break;
                    }
                }
                row.updatedAtMs = QDateTime::currentMSecsSinceEpoch();
                upsertCachedProfile(row);
            }
            emit profileSpecLoaded(id);
        }
        return;
    }
    if (correlationId.startsWith(QLatin1String(kExportPrefix))) {
        const QString id = correlationId.mid(int(qstrlen(kExportPrefix)));
        DecodedDistribution dist;
        if (!NodeApiCodec::decodeDistribution(responseCbor, &dist)) {
            emit operationFailed(QStringLiteral("Failed to decode Distribution"));
            return;
        }
        // Hand the store the raw bytes (the portable artifact) + the decoded form.
        emit distributionExported(id, responseCbor, dist);
        return;
    }
    if (correlationId == QLatin1String(kImportCorrelation)) {
        QString newId;
        if (!NodeApiCodec::decodeProfileId(responseCbor, &newId)) {
            DecodedApiError err;
            emit operationFailed(NodeApiCodec::decodeError(responseCbor, &err)
                                     ? err.message
                                     : QStringLiteral("Profile import failed"));
            return;
        }
        refreshProfiles();
        emit imported(newId);
        return;
    }
    if (correlationId.startsWith(QLatin1String(kHistoryPrefix))) {
        const QString id = correlationId.mid(int(qstrlen(kHistoryPrefix)));
        // A daemon with no revision log answers Unsupported("versioning not available"); that is a
        // capability gap, not a decode failure - surface it as historyUnavailable so the UI can
        // hide the panel honestly rather than toast a misleading parse error.
        if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Error) {
            DecodedApiError err;
            emit historyUnavailable(id, NodeApiCodec::decodeError(responseCbor, &err)
                                            ? err.message
                                            : QStringLiteral("Version history unavailable"));
            return;
        }
        QList<DecodedRevision> revs;
        if (!NodeApiCodec::decodeRevisions(responseCbor, &revs)) {
            emit operationFailed(QStringLiteral("Failed to decode Revisions"));
            return;
        }
        emit historyLoaded(id, revs);
        return;
    }
    if (correlationId.startsWith(QLatin1String(kRevertPrefix))) {
        const QString id = correlationId.mid(int(qstrlen(kRevertPrefix)));
        if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Error) {
            DecodedApiError err;
            emit operationFailed(NodeApiCodec::decodeError(responseCbor, &err)
                                     ? err.message
                                     : QStringLiteral("Profile revert failed"));
            return;
        }
        // Rolled forward: refresh the spec (re-get) + the list, then notify.
        getProfile(id);
        refreshProfiles();
        emit reverted(id);
        return;
    }
    if (correlationId == QLatin1String(kSelectCorrelation) ||
        correlationId == QLatin1String(kDeleteCorrelation) ||
        correlationId == QLatin1String(kUpdateCorrelation) ||
        correlationId == QLatin1String(kCreateCorrelation) ||
        correlationId == QLatin1String(kCloneCorrelation)) {
        // Create/Clone answer with ProfileId (ApiResponseKind::Unknown here); Select/Delete/Update
        // answer with Ok. Any non-error response means the membership/active changed - re-list.
        if (NodeApiCodec::responseKind(responseCbor) != ApiResponseKind::Error) {
            refreshProfiles();
            return;
        }
        DecodedApiError err;
        if (NodeApiCodec::decodeError(responseCbor, &err)) {
            emit operationFailed(err.message);
        } else {
            emit operationFailed(QStringLiteral("Profile operation failed"));
        }
        return;
    }
}

void ProfileRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kProfilesCorrelation)) {
        emit refreshFailed(message);
    } else if (correlationId == QLatin1String(kSelectCorrelation) ||
               correlationId == QLatin1String(kDeleteCorrelation) ||
               correlationId == QLatin1String(kUpdateCorrelation) ||
               correlationId == QLatin1String(kCreateCorrelation) ||
               correlationId == QLatin1String(kCloneCorrelation) ||
               correlationId == QLatin1String(kImportCorrelation) ||
               correlationId.startsWith(QLatin1String(kGetPrefix)) ||
               correlationId.startsWith(QLatin1String(kExportPrefix)) ||
               correlationId.startsWith(QLatin1String(kHistoryPrefix)) ||
               correlationId.startsWith(QLatin1String(kRevertPrefix))) {
        emit operationFailed(message);
    }
}

ApprovalRepository::ApprovalRepository(NodeApiClient* client, DaemonCacheStore* cache,
                                       QObject* parent)
    : RepositoryBase(client, cache, parent) {
    if (this->client() != nullptr) {
        connect(this->client(), &NodeApiClient::responseReady, this,
                &ApprovalRepository::handleResponse);
        connect(this->client(), &NodeApiClient::failed, this, &ApprovalRepository::handleFailure);
    }
}

void ApprovalRepository::refreshPending(const QString& sessionId) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    m_lastSession = sessionId;
    client()->sendRequest(NodeApiCodec::encodeApprovalsPendingRequest(sessionId),
                          QLatin1String(kPendingCorrelation));
}

void ApprovalRepository::decide(const QString& sessionId, const QString& requestId, bool allow) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeApprovalDecideRequest(sessionId, requestId, allow),
                          QLatin1String(kDecideCorrelation));
}

void ApprovalRepository::handleResponse(const QString& correlationId,
                                        const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kPendingCorrelation)) {
        if (!NodeApiCodec::decodeApprovals(responseCbor, &m_pending)) {
            emit operationFailed(QStringLiteral("Failed to decode Approvals response"));
            return;
        }
        emit pendingRefreshed();
        return;
    }
    if (correlationId == QLatin1String(kDecideCorrelation)) {
        const ApiResponseKind kind = NodeApiCodec::responseKind(responseCbor);
        if (kind == ApiResponseKind::Ok) {
            emit decided();
            refreshPending(m_lastSession); // re-sync the inbox after a decision
            return;
        }
        DecodedApiError err;
        if (kind == ApiResponseKind::Error && NodeApiCodec::decodeError(responseCbor, &err)) {
            emit operationFailed(err.message);
        } else {
            emit operationFailed(QStringLiteral("Approval decision failed"));
        }
        return;
    }
}

void ApprovalRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kPendingCorrelation) ||
        correlationId == QLatin1String(kDecideCorrelation)) {
        emit operationFailed(message);
    }
}

// --- FleetRepository (PRO-9/10) -------------------------------------------------------------

FleetRepository::FleetRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent)
    : RepositoryBase(client, cache, parent) {
    if (this->client() != nullptr) {
        connect(this->client(), &NodeApiClient::responseReady, this,
                &FleetRepository::handleResponse);
        connect(this->client(), &NodeApiClient::failed, this, &FleetRepository::handleFailure);
    }
}

QList<CachedFleetUnitRow> FleetRepository::cachedUnits() const {
    return cache() != nullptr ? cache()->fleetUnits() : QList<CachedFleetUnitRow>{};
}

void FleetRepository::refreshTree() {
    if (client() == nullptr) {
        emit refreshFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeTreeRequest(), QLatin1String(kTreeCorrelation));
}

void FleetRepository::pause(const QString& unitId) {
    if (client() == nullptr) {
        emit controlFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodePauseRequest(unitId),
                          QLatin1String(kControlCorrelation));
}

void FleetRepository::resume(const QString& unitId) {
    if (client() == nullptr) {
        emit controlFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeResumeRequest(unitId),
                          QLatin1String(kControlCorrelation));
}

void FleetRepository::scale(const QString& unitId, quint32 n) {
    if (client() == nullptr) {
        emit controlFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeScaleRequest(unitId, n),
                          QLatin1String(kControlCorrelation));
}

void FleetRepository::handleResponse(const QString& correlationId, const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kTreeCorrelation)) {
        QList<DecodedUnitNode> flat;
        if (!NodeApiCodec::decodeTreeReport(responseCbor, &flat)) {
            emit refreshFailed(QStringLiteral("Failed to decode Tree response"));
            return;
        }
        if (cache() != nullptr) {
            const qint64 now = QDateTime::currentMSecsSinceEpoch();
            QSet<QString> keep;
            int ordinal = 0;
            for (const DecodedUnitNode& n : flat) {
                CachedFleetUnitRow row;
                row.unitId = n.id;
                row.parentId = n.parentId;
                row.depth = n.depth;
                row.ordinal = ordinal++;
                row.name = n.title.isEmpty() ? n.id : n.title;
                row.kind = n.kind;
                row.state = n.state;
                row.role = n.role;
                row.profileRef = n.profileRef;
                row.sessionId = n.sessionId;
                row.work = n.work;
                row.updatedAtMs = now;
                cache()->upsertFleetUnit(row);
                keep.insert(n.id);
            }
            // Prune units the live tree no longer lists (finished/removed subagents).
            for (const CachedFleetUnitRow& existing : cache()->fleetUnits()) {
                if (!keep.contains(existing.unitId)) {
                    cache()->deleteFleetUnit(existing.unitId);
                }
            }
        }
        emit treeRefreshed();
        return;
    }
    if (correlationId == QLatin1String(kControlCorrelation)) {
        // Pause/Resume/Scale answer Ok (re-fetch the tree) or an ApiError (e.g. Unsupported on an
        // engine-leaf unit -> surface it, do not silently swallow).
        if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Error) {
            DecodedApiError err;
            NodeApiCodec::decodeError(responseCbor, &err);
            emit controlFailed(err.message.isEmpty() ? QStringLiteral("Unit control failed")
                                                     : err.message);
        } else {
            refreshTree();
        }
        return;
    }
}

void FleetRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kTreeCorrelation)) {
        emit refreshFailed(message);
    } else if (correlationId == QLatin1String(kControlCorrelation)) {
        emit controlFailed(message);
    }
}

TransportRepository::TransportRepository(NodeApiClient* client, DaemonCacheStore* cache,
                                         QObject* parent)
    : RepositoryBase(client, cache, parent) {
    if (this->client() != nullptr) {
        connect(this->client(), &NodeApiClient::responseReady, this,
                &TransportRepository::handleResponse);
        connect(this->client(), &NodeApiClient::failed, this, &TransportRepository::handleFailure);
    }
}

QList<CachedTransportInstanceRow> TransportRepository::cachedInstances() const {
    return cache() != nullptr ? cache()->transportInstances() : QList<CachedTransportInstanceRow>{};
}

QList<CachedConversationRow>
TransportRepository::cachedConversations(const QString& transport) const {
    return cache() != nullptr ? cache()->conversations(transport) : QList<CachedConversationRow>{};
}

void TransportRepository::refreshAdapters() {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeTransportAdaptersRequest(),
                          QLatin1String(kAdaptersCorrelation));
}

void TransportRepository::refreshInstances() {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeTransportInstancesRequest(),
                          QLatin1String(kInstancesCorrelation));
}

void TransportRepository::refreshConversations(const QString& transport) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeConvListRequest(transport),
                          QLatin1String(kConvPrefix) + transport);
}

void TransportRepository::handleResponse(const QString& correlationId,
                                         const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kAdaptersCorrelation)) {
        if (!NodeApiCodec::decodeAdapters(responseCbor, &m_adapters)) {
            emit operationFailed(QStringLiteral("Failed to decode Adapters response"));
            return;
        }
        emit adaptersRefreshed();
        return;
    }
    if (correlationId == QLatin1String(kInstancesCorrelation)) {
        QList<DecodedTransportInstance> live;
        if (!NodeApiCodec::decodeTransportInstances(responseCbor, &live)) {
            emit operationFailed(QStringLiteral("Failed to decode TransportInstances response"));
            return;
        }
        if (cache() != nullptr) {
            const qint64 now = QDateTime::currentMSecsSinceEpoch();
            QSet<QString> keep;
            for (const DecodedTransportInstance& i : live) {
                CachedTransportInstanceRow row;
                row.transport = i.transport;
                row.family = i.family;
                row.displayName = i.displayName;
                row.connection = i.connection;
                row.presence = i.presence;
                row.boundProfile = i.boundProfile;
                row.updatedAtMs = now;
                cache()->upsertTransportInstance(row);
                keep.insert(i.transport);
            }
            // Prune accounts the live list no longer reports (disconnected/removed).
            for (const CachedTransportInstanceRow& existing : cache()->transportInstances()) {
                if (!keep.contains(existing.transport)) {
                    cache()->deleteTransportInstance(existing.transport);
                }
            }
        }
        emit instancesRefreshed();
        return;
    }
    if (correlationId.startsWith(QLatin1String(kConvPrefix))) {
        const QString transport = correlationId.mid(int(qstrlen(kConvPrefix)));
        QList<DecodedConversation> live;
        if (!NodeApiCodec::decodeConversations(responseCbor, &live)) {
            emit operationFailed(QStringLiteral("Failed to decode Conversations response"));
            return;
        }
        if (cache() != nullptr) {
            const qint64 now = QDateTime::currentMSecsSinceEpoch();
            QSet<QString> keep;
            for (const DecodedConversation& c : live) {
                CachedConversationRow row;
                row.transport = transport; // key by the requested transport for consistent pruning
                row.convId = c.id;
                row.kind = c.kind;
                row.title = c.title;
                row.topic = c.topic;
                row.updatedAtMs = now;
                cache()->upsertConversation(row);
                keep.insert(c.id);
            }
            for (const CachedConversationRow& existing : cache()->conversations(transport)) {
                if (!keep.contains(existing.convId)) {
                    cache()->deleteConversation(transport, existing.convId);
                }
            }
        }
        emit conversationsRefreshed(transport);
        return;
    }
}

void TransportRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kAdaptersCorrelation) ||
        correlationId == QLatin1String(kInstancesCorrelation) ||
        correlationId.startsWith(QLatin1String(kConvPrefix))) {
        emit operationFailed(message);
    }
}

} // namespace daemonapp::daemon
