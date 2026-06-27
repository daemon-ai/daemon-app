#include "daemon/repositories.h"

#include "daemon/node_api_codec.h"

#include <QDateTime>

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

bool SessionRepository::appendCachedLog(const CachedLogRow& row) {
    return cache() != nullptr && cache()->appendSessionLog(row);
}

QList<CachedLogRow> SessionRepository::cachedLog(const QString& sessionId, quint64 afterSeq,
                                                 int limit) const {
    return cache() != nullptr ? cache()->sessionLog(sessionId, afterSeq, limit)
                              : QList<CachedLogRow>{};
}

bool SessionRepository::setLogCursor(const QString& sessionId, quint64 seq) {
    return cache() != nullptr &&
           cache()->setCursor(QStringLiteral("session-log/%1").arg(sessionId), QString::number(seq),
                              QDateTime::currentMSecsSinceEpoch());
}

quint64 SessionRepository::logCursor(const QString& sessionId) const {
    return cache() != nullptr
               ? cache()->cursor(QStringLiteral("session-log/%1").arg(sessionId)).toULongLong()
               : 0;
}

void SessionRepository::refreshSessions() {
    if (client() == nullptr) {
        emit refreshFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeSessionsQueryRequest(),
                          QLatin1String(kSessionsCorrelation));
}

void SessionRepository::subscribe(const QString& sessionId) {
    if (client() == nullptr) {
        emit refreshFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    // Resume from the persisted cursor so each poll only pulls new entries.
    constexpr quint32 kMaxEntries = 256;
    client()->sendRequest(
        NodeApiCodec::encodeSubscribeRequest(sessionId, logCursor(sessionId), kMaxEntries),
        subscribeCorrelation(sessionId));
}

QString SessionRepository::subscribeCorrelation(const QString& sessionId) {
    return QLatin1String(kSubscribePrefix) + sessionId;
}

void SessionRepository::handleResponse(const QString& correlationId,
                                       const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kSessionsCorrelation)) {
        QList<CachedSessionRow> rows;
        QString nextCursor;
        if (!NodeApiCodec::decodeSessionPage(responseCbor, &rows, &nextCursor)) {
            emit refreshFailed(QStringLiteral("Failed to decode SessionPage response"));
            return;
        }
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        for (CachedSessionRow& row : rows) {
            row.updatedAtMs = now;
            upsertCachedSession(row);
        }
        if (!nextCursor.isEmpty() && cache() != nullptr) {
            cache()->setCursor(QStringLiteral("sessions-query/cursor"), nextCursor, now);
        }
        emit sessionsRefreshed();
        return;
    }

    if (correlationId.startsWith(QLatin1String(kSubscribePrefix))) {
        const QString sessionId = correlationId.mid(int(qstrlen(kSubscribePrefix)));
        QList<CachedLogRow> rows;
        quint64 nextSeq = 0;
        if (!NodeApiCodec::decodeLogPage(responseCbor, sessionId, &rows, &nextSeq)) {
            emit refreshFailed(QStringLiteral("Failed to decode LogPage response"));
            return;
        }
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        for (CachedLogRow& row : rows) {
            row.updatedAtMs = now;
            appendCachedLog(row);
        }
        if (nextSeq > 0) {
            setLogCursor(sessionId, nextSeq);
        }
        emit logUpdated(sessionId);
        return;
    }
}

void SessionRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kSessionsCorrelation) ||
        correlationId.startsWith(QLatin1String(kSubscribePrefix))) {
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
}

void ModelRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kModelsCorrelation) ||
        correlationId == QLatin1String(kCurrentCorrelation) ||
        correlationId == QLatin1String(kSetModelCorrelation)) {
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
    }
}

bool ProfileRepository::upsertCachedProfile(const CachedProfileRow& row) {
    return cache() != nullptr && cache()->upsertProfile(row);
}

QList<CachedProfileRow> ProfileRepository::cachedProfiles() const {
    return cache() != nullptr ? cache()->profiles() : QList<CachedProfileRow>{};
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

void ProfileRepository::handleResponse(const QString& correlationId,
                                       const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kProfilesCorrelation)) {
        if (!NodeApiCodec::decodeProfiles(responseCbor, &m_profiles)) {
            emit refreshFailed(QStringLiteral("Failed to decode Profiles response"));
            return;
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
            emit profileSpecLoaded(id);
        }
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
               correlationId.startsWith(QLatin1String(kGetPrefix))) {
        emit operationFailed(message);
    }
}

bool FsRepository::upsertCachedEntry(const CachedFsEntryRow& row) {
    return cache() != nullptr && cache()->upsertFsEntry(row);
}

QList<CachedFsEntryRow> FsRepository::cachedEntries(const QString& rootId) const {
    return cache() != nullptr ? cache()->fsEntries(rootId) : QList<CachedFsEntryRow>{};
}

bool ApprovalRepository::upsertCachedApproval(const CachedApprovalRow& row) {
    return cache() != nullptr && cache()->upsertApproval(row);
}

QList<CachedApprovalRow> ApprovalRepository::cachedApprovals() const {
    return cache() != nullptr ? cache()->approvals() : QList<CachedApprovalRow>{};
}

} // namespace daemonapp::daemon
