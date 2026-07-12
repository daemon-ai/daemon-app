// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/repositories.h"

#include "daemon/node_api_codec.h"
#include "uuidv7.h"

#include <algorithm>
#include <array>
#include <QDateTime>
#include <QFile>
#include <QHash>
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

void SessionRepository::startTurn(const QString& sessionId, const QString& text) {
    if (client() == nullptr || sessionId.isEmpty() || text.isEmpty()) {
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeSubmitStartTurnRequest(sessionId, text),
                          QLatin1String(kSubmitPrefix) + sessionId);
}

void SessionRepository::steer(const QString& sessionId, const QString& text) {
    if (client() == nullptr || sessionId.isEmpty() || text.isEmpty()) {
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeSubmitSteerRequest(sessionId, text),
                          QLatin1String(kSubmitPrefix) + sessionId);
}

void SessionRepository::interrupt(const QString& sessionId) {
    if (client() == nullptr || sessionId.isEmpty()) {
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeSubmitInterruptRequest(sessionId),
                          QLatin1String(kSubmitPrefix) + sessionId);
}

void SessionRepository::createSession(const QString& profileId) {
    if (client() == nullptr) {
        emit refreshFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    m_pendingCreateProfile = profileId;
    // Node-authority: send no session id so the node MINTS it; bind the chosen profile (empty = the
    // node's active default).
    client()->sendRequest(NodeApiCodec::encodeSessionCreateRequest(QString(), profileId),
                          QLatin1String(kCreateCorrelation));
}

void SessionRepository::getSessionDetail(const QString& sessionId) {
    if (sessionId.isEmpty() || client() == nullptr) {
        return;
    }
    if (m_detailInFlight.contains(sessionId)) {
        return; // dedupe: a SessionGet for this id is already outstanding
    }
    m_detailInFlight.insert(sessionId);
    client()->sendRequest(NodeApiCodec::encodeSessionGetRequest(sessionId),
                          QLatin1String(kDetailPrefix) + sessionId);
}

bool SessionRepository::cachedDetail(const QString& id, DecodedSessionDetail* out) const {
    const auto it = m_details.constFind(id);
    if (it == m_details.constEnd()) {
        return false;
    }
    if (out != nullptr) {
        *out = it.value();
    }
    return true;
}

void SessionRepository::applySessionDetail(const QString& sessionId,
                                           const QByteArray& responseCbor) {
    m_detailInFlight.remove(sessionId);
    if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Error) {
        DecodedApiError err;
        const QString msg = NodeApiCodec::decodeError(responseCbor, &err) && !err.message.isEmpty()
                                ? err.message
                                : tr("The session detail request was rejected");
        emit detailFailed(sessionId, msg);
        return;
    }
    DecodedSessionDetail detail;
    bool found = false;
    if (!NodeApiCodec::decodeSessionDetail(responseCbor, &detail, &found)) {
        emit detailFailed(sessionId, tr("Failed to decode SessionDetail response"));
        return;
    }
    // The null arm (unknown session) still resolves the fetch: cache an empty detail so a repeated
    // read does not re-issue forever, and fire the loaded signal so consumers stop waiting.
    m_details.insert(sessionId, detail);
    emit sessionDetailLoaded(sessionId);
}

void SessionRepository::handleResponse(const QString& correlationId,
                                       const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kCreateCorrelation)) {
        applySessionCreated(responseCbor);
    } else if (correlationId.startsWith(QLatin1String(kDetailPrefix))) {
        applySessionDetail(correlationId.mid(int(qstrlen(kDetailPrefix))), responseCbor);
    } else if (correlationId.startsWith(QLatin1String(kSubmitPrefix))) {
        applySubmitReply(correlationId.mid(int(qstrlen(kSubmitPrefix))), responseCbor);
    }
}

void SessionRepository::applySubmitReply(const QString& sessionId, const QByteArray& responseCbor) {
    // Any non-error acknowledgement means the node accepted the command (mirrors the turn
    // engine's submit handling); only an explicit Error is a rejection.
    if (NodeApiCodec::responseKind(responseCbor) != ApiResponseKind::Error) {
        emit submitted(sessionId);
        return;
    }
    DecodedApiError err;
    const QString msg = NodeApiCodec::decodeError(responseCbor, &err) && !err.message.isEmpty()
                            ? err.message
                            : tr("The session rejected the command");
    emit submitFailed(sessionId, msg);
}

void SessionRepository::applySessionCreated(const QByteArray& responseCbor) {
    QString sessionId;
    if (!NodeApiCodec::decodeSessionCreated(responseCbor, &sessionId)) {
        // A failed create must not vanish silently: surface the node's ApiError (or a generic
        // message) through the repo's error signal and drop the dead pending-create state.
        m_pendingCreateProfile.clear();
        DecodedApiError err;
        const QString msg = NodeApiCodec::decodeError(responseCbor, &err)
                                ? err.message
                                : tr("SessionCreate failed");
        emit refreshFailed(msg);
        return;
    }
    const QString profileId = m_pendingCreateProfile;
    m_pendingCreateProfile.clear();
    // The node also emits RosterChanged (the mirror ingestor's policy arm refetches the roster
    // into the `sessions` table — the authoritative row lands there); this signal is the
    // event-driven hook the orchestrator/sidebar auto-select on (AD: the legacy cache placeholder
    // write + immediate cache refetch died with the cache feed).
    emit sessionCreated(sessionId, profileId);
}

void SessionRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kCreateCorrelation)) {
        // A transport-level create failure (timeout / dropped socket): the pending create is
        // dead — clear its state and surface the error instead of vanishing silently.
        m_pendingCreateProfile.clear();
        emit refreshFailed(message);
        return;
    }
    if (correlationId.startsWith(QLatin1String(kSubmitPrefix))) {
        // A transport-level operator-submit failure: surface it so a steer/cancel is never
        // silently mistaken for delivered.
        emit submitFailed(correlationId.mid(int(qstrlen(kSubmitPrefix))), message);
        return;
    }
    if (correlationId.startsWith(QLatin1String(kDetailPrefix))) {
        // A transport-level SessionGet failure: clear the in-flight guard and surface it so a
        // detail read is retriable rather than wedged.
        const QString sessionId = correlationId.mid(int(qstrlen(kDetailPrefix)));
        m_detailInFlight.remove(sessionId);
        emit detailFailed(sessionId, message);
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

// [acct-mgmt] wire v35: persist the credential label. One request; on Ok re-fetch the redacted list
// (the node emits no event for a label set) so the new label renders — never optimistic.
void CredentialRepository::setCredentialLabel(const QString& profile, bool hasLabel,
                                              const QString& label) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeCredentialSetLabelRequest(profile, hasLabel, label),
                          QLatin1String(kSetLabelCorrelation));
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
    // [acct-mgmt] wire v35: set-label joins the set/remove re-fetch-on-Ok path.
    if (correlationId == QLatin1String(kSetCorrelation) ||
        correlationId == QLatin1String(kRemoveCorrelation) ||
        correlationId == QLatin1String(kSetLabelCorrelation)) {
        const ApiResponseKind kind = NodeApiCodec::responseKind(responseCbor);
        if (kind == ApiResponseKind::Ok) {
            refreshList(); // reflect the mutation in the redacted view
            return;
        }
        DecodedApiError err;
        if (kind == ApiResponseKind::Error && NodeApiCodec::decodeError(responseCbor, &err)) {
            emit operationFailed(err.message);
        } else {
            emit operationFailed(tr("Credential operation failed"));
        }
        return;
    }
}

bool CredentialRepository::isOwnCorrelation(const QString& correlationId) {
    const std::array keys = {kListCorrelation, kSetCorrelation, kRemoveCorrelation,
                             kSetLabelCorrelation};
    return std::ranges::any_of(
        keys, [&](const char* key) { return correlationId == QLatin1String(key); });
}

void CredentialRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (isOwnCorrelation(correlationId)) {
        emit operationFailed(message);
    }
}

// --- ProviderRepository (provider/model discovery) ------------------------------------------

ProviderRepository::ProviderRepository(NodeApiClient* client, DaemonCacheStore* cache,
                                       QObject* parent)
    : RepositoryBase(client, cache, parent) {
    if (this->client() != nullptr) {
        connect(this->client(), &NodeApiClient::responseReady, this,
                &ProviderRepository::handleResponse);
        connect(this->client(), &NodeApiClient::failed, this, &ProviderRepository::handleFailure);
    }
}

void ProviderRepository::refreshProviders() {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeProviderCatalogRequest(),
                          QLatin1String(kCatalogCorrelation));
}

void ProviderRepository::refreshProviderModels(const QString& provider,
                                               const QString& credentialRef,
                                               const QString& transientKey) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    if (provider.isEmpty()) {
        return;
    }
    // A fresh refresh restarts the provider's page loop; the continuations re-issue the same
    // credentials verbatim (the loop state keeps them).
    ProviderModelsLoop& state = m_modelsLoops[provider];
    state.loop.reset();
    state.credentialRef = credentialRef;
    state.transientKey = transientKey;
    // Tag the correlation with the provider id so the response handler knows which list to fill.
    client()->sendRequest(
        NodeApiCodec::encodeProviderModelsRequest(provider, credentialRef, transientKey),
        QString::fromLatin1(kModelsPrefix) + provider);
}

void ProviderRepository::refreshCustomProviders() {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeCustomProviderListRequest(),
                          QLatin1String(kCustomListCorrelation));
}

void ProviderRepository::setCustomProvider(const DecodedCustomProvider& provider) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeCustomProviderSetRequest(provider),
                          QLatin1String(kCustomSetCorrelation));
}

void ProviderRepository::removeCustomProvider(const QString& id) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    if (id.isEmpty()) {
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeCustomProviderRemoveRequest(id),
                          QLatin1String(kCustomRemoveCorrelation));
}

void ProviderRepository::handleResponse(const QString& correlationId,
                                        const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kCatalogCorrelation)) {
        const bool decoded = NodeApiCodec::decodeProviderCatalog(responseCbor, &m_providers);
        if (!decoded) {
            emit operationFailed(QStringLiteral("Failed to decode ProviderCatalog response"));
            return;
        }
        emit providersRefreshed();
        return;
    }
    if (correlationId == QLatin1String(kCustomListCorrelation)) {
        if (!NodeApiCodec::decodeCustomProviders(responseCbor, &m_customProviders)) {
            emit operationFailed(QStringLiteral("Failed to decode CustomProviders response"));
            return;
        }
        emit customProvidersRefreshed();
        return;
    }
    if (correlationId == QLatin1String(kCustomSetCorrelation) ||
        correlationId == QLatin1String(kCustomRemoveCorrelation)) {
        // The node acked the mutation (a failure would arrive via handleFailure). Re-fetch the
        // editor list AND the merged catalog so both the management view and the picker update.
        refreshCustomProviders();
        refreshProviders();
        return;
    }
    if (correlationId.startsWith(QLatin1String(kModelsPrefix))) {
        const QString provider = correlationId.mid(int(qstrlen(kModelsPrefix)));
        QList<DecodedModelDescriptor> models;
        QString next;
        if (!NodeApiCodec::decodeProviderModels(responseCbor, &models, &next)) {
            m_modelsLoops.remove(provider);
            emit operationFailed(QStringLiteral("Failed to decode ProviderModels response"));
            return;
        }
        // Page loop: accumulate and re-issue with the cursor until it clears, then publish the
        // complete listing ONCE (the discovery consumers replace on the signal).
        ProviderModelsLoop& state = m_modelsLoops[provider];
        state.loop.items.append(models);
        if (!next.isEmpty() && state.loop.guard(next) && client() != nullptr) {
            client()->sendRequest(NodeApiCodec::encodeProviderModelsRequest(
                                      provider, state.credentialRef, state.transientKey, next),
                                  QString::fromLatin1(kModelsPrefix) + provider);
            return;
        }
        m_models.insert(provider, state.loop.items);
        m_modelsLoops.remove(provider);
        emit providerModelsRefreshed(provider);
    }
}

void ProviderRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kCatalogCorrelation) ||
        correlationId.startsWith(QLatin1String(kModelsPrefix)) ||
        correlationId == QLatin1String(kCustomListCorrelation) ||
        correlationId == QLatin1String(kCustomSetCorrelation) ||
        correlationId == QLatin1String(kCustomRemoveCorrelation)) {
        if (correlationId.startsWith(QLatin1String(kModelsPrefix))) {
            // A dead page loop must not leak its partial accumulation into the next refresh.
            m_modelsLoops.remove(correlationId.mid(int(qstrlen(kModelsPrefix))));
        }
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
    // A fresh refresh restarts the page loop: drop anything a superseded loop accumulated.
    m_modelsLoop.reset();
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
    // A fresh search restarts paging: page 0 replaces the hit list; searchMore() appends.
    m_searchText = text;
    m_searchEngine = engine;
    m_searchSort = sort;
    m_searchAppending = false;
    client()->sendRequest(NodeApiCodec::encodeModelSearchRequest(text, engine, sort),
                          QLatin1String(kSearchCorrelation));
}

void ModelRepository::searchMore() {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    if (!m_searchHasMore || m_searchAppending) {
        return; // no further page, or a continuation is already in flight
    }
    m_searchAppending = true;
    client()->sendRequest(NodeApiCodec::encodeModelSearchRequest(m_searchText, m_searchEngine,
                                                                 m_searchSort, m_searchPage + 1),
                          QLatin1String(kSearchCorrelation));
}

void ModelRepository::requestFiles(const QString& repo, const QString& engine) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    m_pendingFilesRepo = repo;
    m_pendingFilesEngine = engine;
    // A fresh request restarts the page loop: drop anything a superseded loop accumulated.
    m_filesLoop.reset();
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

void ModelRepository::applyDownloadProgress(quint64 id, const QString& state,
                                            quint64 downloadedBytes, quint64 totalBytes) {
    // Patch the matching job row in place (or insert a fresh one), so the Downloads view tracks
    // progress from the push feed instead of a poll. The event carries the REAL byte counters
    // (wire v26); a zero total (Hub reported no sizes) keeps any previously-known total.
    bool found = false;
    for (DecodedDownloadStatus& s : m_downloads) {
        if (s.id == id) {
            s.state = state;
            s.downloadedBytes = downloadedBytes;
            if (totalBytes > 0) {
                s.totalBytes = totalBytes;
            }
            found = true;
            break;
        }
    }
    if (!found) {
        DecodedDownloadStatus s;
        s.id = id;
        s.state = state;
        s.downloadedBytes = downloadedBytes;
        s.totalBytes = totalBytes;
        m_downloads.append(s);
    }
    bool newlyCompleted = false;
    if (state == QStringLiteral("Completed") && !m_completedDownloads.contains(id)) {
        m_completedDownloads.insert(id);
        newlyCompleted = true;
    }
    emit downloadsChanged();
    // The event carries only id/state/bytes: a fresh insert (a job started by another client
    // instance) lacks repo/file/name, and file-completion counts (files x/y) go stale on the
    // in-place patch — refetch the authoritative row set on those transitions (event-driven, no
    // polling; the placeholder row above keeps progress visible until the reply lands).
    if (!found || newlyCompleted) {
        refreshDownloads();
    }
    if (newlyCompleted) {
        refreshCatalog();
    }
}

void ModelRepository::handleResponse(const QString& correlationId, const QByteArray& responseCbor) {
    using Handler = void (ModelRepository::*)(const QByteArray&);
    static const QHash<QString, Handler> kHandlers = {
        {QString::fromLatin1(kModelsCorrelation), &ModelRepository::handleModelsResponse},
        {QString::fromLatin1(kCurrentCorrelation), &ModelRepository::handleModelCurrentResponse},
        {QString::fromLatin1(kSetModelCorrelation),
         &ModelRepository::handleSetSessionModelResponse},
        {QString::fromLatin1(kSearchCorrelation), &ModelRepository::handleModelSearchResponse},
        {QString::fromLatin1(kFilesCorrelation), &ModelRepository::handleModelFilesResponse},
        {QString::fromLatin1(kRecommendCorrelation),
         &ModelRepository::handleModelRecommendResponse},
        {QString::fromLatin1(kDownloadCorrelation), &ModelRepository::handleModelDownloadResponse},
        {QString::fromLatin1(kDownloadsCorrelation),
         &ModelRepository::handleModelDownloadsResponse},
        {QString::fromLatin1(kCatalogCorrelation), &ModelRepository::handleModelCatalogResponse},
        {QString::fromLatin1(kDeleteCorrelation), &ModelRepository::handleModelDeleteResponse},
        {QString::fromLatin1(kActivateCorrelation), &ModelRepository::handleModelActivateResponse},
        {QString::fromLatin1(kLifecycleCorrelation),
         &ModelRepository::handleModelLifecycleResponse},
        {QString::fromLatin1(kQuantizeCorrelation), &ModelRepository::handleModelQuantizeResponse},
        {QString::fromLatin1(kQuantizesCorrelation),
         &ModelRepository::handleModelQuantizesResponse},
    };
    const auto it = kHandlers.constFind(correlationId);
    if (it != kHandlers.constEnd()) {
        (this->*it.value())(responseCbor);
        return;
    }
    // A6 ghost probe: the inspected id rides the correlation tail (prefix route).
    if (correlationId.startsWith(QLatin1String(kInspectPrefix))) {
        const QString id = correlationId.mid(QLatin1String(kInspectPrefix).size());
        DecodedGgufInfo info;
        if (NodeApiCodec::decodeModelInspect(responseCbor, &info)) {
            emit inspected(id, info);
            return;
        }
        // An Error reply on a cataloged id IS the ghost signal (missing/unreadable artifact).
        DecodedApiError err;
        if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Error &&
            NodeApiCodec::decodeError(responseCbor, &err)) {
            emit inspectFailed(id, err.message);
        } else {
            emit inspectFailed(id, tr("Model inspection failed"));
        }
    }
}

void ModelRepository::emitOperationError(const QByteArray& responseCbor, const QString& fallback) {
    DecodedApiError err;
    if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Error &&
        NodeApiCodec::decodeError(responseCbor, &err)) {
        emit operationFailed(err.message);
    } else {
        emit operationFailed(fallback);
    }
}

void ModelRepository::handleModelsResponse(const QByteArray& responseCbor) {
    QList<DecodedModelDescriptor> models;
    QString next;
    if (!NodeApiCodec::decodeModels(responseCbor, &models, &next)) {
        m_modelsLoop.reset();
        emit operationFailed(QStringLiteral("Failed to decode Models response"));
        return;
    }
    // Page loop: accumulate and re-issue with the cursor until it clears, then publish the
    // complete catalog ONCE (the picker consumers replace on modelsRefreshed).
    m_modelsLoop.items.append(models);
    if (!next.isEmpty() && m_modelsLoop.guard(next) && client() != nullptr) {
        client()->sendRequest(NodeApiCodec::encodeModelsRequest(next),
                              QLatin1String(kModelsCorrelation));
        return;
    }
    m_models = m_modelsLoop.items;
    m_modelsLoop.reset();
    emit modelsRefreshed();
}

void ModelRepository::handleModelCurrentResponse(const QByteArray& responseCbor) {
    if (!NodeApiCodec::decodeModelCurrent(responseCbor, &m_current, &m_hasCurrent)) {
        emit operationFailed(QStringLiteral("Failed to decode ModelCurrent response"));
        return;
    }
    emit currentRefreshed();
}

void ModelRepository::handleSetSessionModelResponse(const QByteArray& responseCbor) {
    if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Ok) {
        emit modelSet();
        return;
    }
    emitOperationError(responseCbor, tr("Set-model operation failed"));
}

void ModelRepository::handleModelSearchResponse(const QByteArray& responseCbor) {
    QList<DecodedSearchHit> hits;
    bool hasMore = false;
    quint32 page = 0;
    if (!NodeApiCodec::decodeModelSearch(responseCbor, &hits, &hasMore, &page)) {
        m_searchAppending = false;
        emit operationFailed(QStringLiteral("Failed to decode ModelSearch response"));
        return;
    }
    if (m_searchAppending) {
        m_searchHits.append(hits); // a searchMore() continuation accumulates
    } else {
        m_searchHits = hits;
    }
    m_searchAppending = false;
    m_searchPage = page;
    m_searchHasMore = hasMore;
    emit searchHitsChanged();
}

void ModelRepository::handleModelFilesResponse(const QByteArray& responseCbor) {
    QList<DecodedModelFile> files;
    QString next;
    if (!NodeApiCodec::decodeModelFiles(responseCbor, &files, &next)) {
        m_filesLoop.reset();
        emit operationFailed(QStringLiteral("Failed to decode ModelFiles response"));
        return;
    }
    // Page loop: accumulate and re-issue with the cursor until it clears, then publish the repo's
    // complete file list ONCE (the quant picker replaces on filesLoaded).
    m_filesLoop.items.append(files);
    if (!next.isEmpty() && m_filesLoop.guard(next) && client() != nullptr) {
        client()->sendRequest(NodeApiCodec::encodeModelFilesRequest(
                                  m_pendingFilesRepo, m_pendingFilesEngine, QString(), next),
                              QLatin1String(kFilesCorrelation));
        return;
    }
    m_files = m_filesLoop.items;
    m_filesLoop.reset();
    m_filesRepo = m_pendingFilesRepo;
    emit filesLoaded(m_filesRepo);
}

void ModelRepository::handleModelRecommendResponse(const QByteArray& responseCbor) {
    if (!NodeApiCodec::decodeModelRecommend(responseCbor, &m_recommendation)) {
        emit operationFailed(QStringLiteral("Failed to decode ModelRecommend response"));
        return;
    }
    m_hasRecommendation = true;
    emit recommendLoaded(m_pendingRecommendRepo);
}

void ModelRepository::handleModelDownloadResponse(const QByteArray& responseCbor) {
    quint64 id = 0;
    if (NodeApiCodec::decodeModelDownloadStarted(responseCbor, &id)) {
        emit downloadStarted(id);
        refreshDownloads(); // fetch the initial snapshot; live progress arrives via the L3 feed
        return;
    }
    emitOperationError(responseCbor, tr("Download could not be started"));
}

void ModelRepository::handleModelDownloadsResponse(const QByteArray& responseCbor) {
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
}

void ModelRepository::handleModelCatalogResponse(const QByteArray& responseCbor) {
    if (!NodeApiCodec::decodeModelCatalog(responseCbor, &m_installed)) {
        emit operationFailed(QStringLiteral("Failed to decode ModelCatalog response"));
        return;
    }
    emit catalogChanged();
}

void ModelRepository::handleModelDeleteResponse(const QByteArray& responseCbor) {
    if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Ok) {
        refreshCatalog();
        return;
    }
    emit operationFailed(tr("Delete failed"));
}

void ModelRepository::handleModelActivateResponse(const QByteArray& responseCbor) {
    if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Ok) {
        emit modelActivated();
        return;
    }
    emitOperationError(responseCbor, tr("Activate failed"));
}

void ModelRepository::handleModelLifecycleResponse(const QByteArray& /*responseCbor*/) {
    // Cancel/Pause/Resume Ok (or Error) -> re-poll so the new state surfaces.
    refreshDownloads();
}

// --- Local re-quantization (A5) + ghost probe (A6); app-wizard-auth stream ----------------------

void ModelRepository::quantize(const QString& repo, const QString& targetQuant,
                               const QString& sourceFile) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeModelQuantizeRequest(repo, targetQuant, sourceFile),
                          QLatin1String(kQuantizeCorrelation));
}

void ModelRepository::refreshQuantizes() {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeModelQuantizesRequest(),
                          QLatin1String(kQuantizesCorrelation));
}

void ModelRepository::inspect(const QString& id) {
    if (client() == nullptr) {
        emit inspectFailed(id, QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeModelInspectRequest(id),
                          QLatin1String(kInspectPrefix) + id);
}

void ModelRepository::handleModelQuantizeResponse(const QByteArray& responseCbor) {
    quint64 id = 0;
    if (!NodeApiCodec::decodeModelQuantizeStarted(responseCbor, &id)) {
        emitOperationError(responseCbor, tr("Quantize failed to start"));
        return;
    }
    emit quantizeStarted(id);
    refreshQuantizes(); // seed the jobs view; the poll loop keeps it live until settle
}

void ModelRepository::handleModelQuantizesResponse(const QByteArray& responseCbor) {
    if (!NodeApiCodec::decodeModelQuantizes(responseCbor, &m_quantizes)) {
        emit operationFailed(QStringLiteral("Failed to decode ModelQuantizes response"));
        return;
    }
    bool active = false;
    bool newlyCompleted = false;
    for (const DecodedQuantizeStatus& job : m_quantizes) {
        if (job.state == QStringLiteral("Queued") || job.state == QStringLiteral("Preparing") ||
            job.state == QStringLiteral("Quantizing")) {
            active = true;
        } else if (job.state == QStringLiteral("Completed") &&
                   !m_completedQuantizes.contains(job.id)) {
            m_completedQuantizes.insert(job.id);
            newlyCompleted = true;
        }
    }
    emit quantizesChanged();
    if (newlyCompleted) {
        refreshCatalog(); // the produced model was cataloged; surface it
    }
    // Quantize jobs have no L3 push event (unlike DownloadProgress), so a modest poll runs while
    // any job is active and stops on settle — the same shape the retired downloads poll had.
    if (active) {
        if (m_quantizePoll == nullptr) {
            m_quantizePoll = new QTimer(this);
            m_quantizePoll->setInterval(1000);
            connect(m_quantizePoll, &QTimer::timeout, this, &ModelRepository::refreshQuantizes);
        }
        if (!m_quantizePoll->isActive()) {
            m_quantizePoll->start();
        }
    } else if (m_quantizePoll != nullptr) {
        m_quantizePoll->stop();
    }
}

void ModelRepository::handleFailure(const QString& correlationId, const QString& message) {
    // A6: a transport-dead inspect must surface per-id (the probe caller keys on it).
    if (correlationId.startsWith(QLatin1String(kInspectPrefix))) {
        emit inspectFailed(correlationId.mid(QLatin1String(kInspectPrefix).size()), message);
        return;
    }
    static const QSet<QString> kOurs = {
        QString::fromLatin1(kModelsCorrelation),   QString::fromLatin1(kCurrentCorrelation),
        QString::fromLatin1(kSetModelCorrelation), QString::fromLatin1(kSearchCorrelation),
        QString::fromLatin1(kFilesCorrelation),    QString::fromLatin1(kRecommendCorrelation),
        QString::fromLatin1(kDownloadCorrelation), QString::fromLatin1(kDownloadsCorrelation),
        QString::fromLatin1(kCatalogCorrelation),  QString::fromLatin1(kDeleteCorrelation),
        QString::fromLatin1(kActivateCorrelation), QString::fromLatin1(kLifecycleCorrelation),
        QString::fromLatin1(kQuantizeCorrelation), QString::fromLatin1(kQuantizesCorrelation),
    };
    if (kOurs.contains(correlationId)) {
        // A dead page loop must not leak its partial accumulation into the next refresh.
        if (correlationId == QLatin1String(kModelsCorrelation)) {
            m_modelsLoop.reset();
        } else if (correlationId == QLatin1String(kFilesCorrelation)) {
            m_filesLoop.reset();
        } else if (correlationId == QLatin1String(kSearchCorrelation)) {
            m_searchAppending = false; // a dead continuation must not make the next reply append
        } else if (correlationId == QLatin1String(kQuantizesCorrelation) &&
                   m_quantizePoll != nullptr) {
            m_quantizePoll->stop(); // a dead transport must not keep the settle poll hammering
        }
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

QString ProfileRepository::cachedSoul(const QString& id) const {
    return m_souls.value(id);
}

void ProfileRepository::requestSoul(const QString& id) {
    if (client() == nullptr || id.isEmpty()) {
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeSoulGetRequest(id),
                          QLatin1String(kSoulGetPrefix) + id);
}

void ProfileRepository::setSoul(const QString& id, const QString& text) {
    if (client() == nullptr) {
        emit soulOpFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    if (id.isEmpty()) {
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeSoulSetRequest(id, text),
                          QLatin1String(kSoulSetPrefix) + id);
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
    // A fresh fetch restarts the page loop: drop anything a superseded loop accumulated.
    m_historyLoops[id].reset();
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
    if (dispatchExactResponse(correlationId, responseCbor)) {
        return;
    }
    dispatchPrefixedResponse(correlationId, responseCbor);
}

bool ProfileRepository::dispatchExactResponse(const QString& correlationId,
                                              const QByteArray& responseCbor) {
    using Handler = void (ProfileRepository::*)(const QByteArray&);
    static const QHash<QString, Handler> kHandlers = {
        {QString::fromLatin1(kProfilesCorrelation), &ProfileRepository::handleProfilesResponse},
        {QString::fromLatin1(kImportCorrelation), &ProfileRepository::handleProfileImportResponse},
        // Select/Delete/Update/Create/Clone all share the "re-list on success" mutation handler.
        {QString::fromLatin1(kSelectCorrelation),
         &ProfileRepository::handleProfileMutationResponse},
        {QString::fromLatin1(kDeleteCorrelation),
         &ProfileRepository::handleProfileMutationResponse},
        {QString::fromLatin1(kUpdateCorrelation),
         &ProfileRepository::handleProfileMutationResponse},
        {QString::fromLatin1(kCreateCorrelation),
         &ProfileRepository::handleProfileMutationResponse},
        {QString::fromLatin1(kCloneCorrelation), &ProfileRepository::handleProfileMutationResponse},
    };
    const auto it = kHandlers.constFind(correlationId);
    if (it == kHandlers.constEnd()) {
        return false;
    }
    (this->*it.value())(responseCbor);
    return true;
}

void ProfileRepository::dispatchPrefixedResponse(const QString& correlationId,
                                                 const QByteArray& responseCbor) {
    struct PrefixRoute {
        const char* prefix;
        void (ProfileRepository::*handler)(const QString&, const QByteArray&);
    };
    static const std::array<PrefixRoute, 6> kRoutes = {{
        {kGetPrefix, &ProfileRepository::handleProfileGetResponse},
        {kSoulGetPrefix, &ProfileRepository::handleSoulGetResponse},
        {kSoulSetPrefix, &ProfileRepository::handleSoulSetResponse},
        {kExportPrefix, &ProfileRepository::handleProfileExportResponse},
        {kHistoryPrefix, &ProfileRepository::handleProfileHistoryResponse},
        {kRevertPrefix, &ProfileRepository::handleProfileRevertResponse},
    }};
    for (const PrefixRoute& route : kRoutes) {
        if (correlationId.startsWith(QLatin1String(route.prefix))) {
            (this->*route.handler)(correlationId.mid(int(qstrlen(route.prefix))), responseCbor);
            return;
        }
    }
}

void ProfileRepository::handleProfilesResponse(const QByteArray& responseCbor) {
    if (!NodeApiCodec::decodeProfiles(responseCbor, &m_profiles)) {
        emit refreshFailed(QStringLiteral("Failed to decode Profiles response"));
        return;
    }
    // Offline-first prune: drop any cached profile the live list no longer carries (a deleted
    // agent) so the cache mirrors truth. Surviving rows keep their spec_cbor; the per-profile
    // ProfileGet that the store kicks next refreshes each row's spec + active flag.
    pruneStaleProfiles();
    emit profilesRefreshed();
}

void ProfileRepository::pruneStaleProfiles() {
    if (cache() == nullptr) {
        return;
    }
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

void ProfileRepository::handleProfileGetResponse(const QString& id,
                                                 const QByteArray& responseCbor) {
    DecodedProfileSpec spec;
    bool found = false;
    if (!(NodeApiCodec::decodeProfile(responseCbor, &spec, &found) && found)) {
        return;
    }
    m_specs.insert(id, spec);
    // Offline-first: persist the full profile (raw ProfileGet bytes + the active flag from the
    // list) so the Profiles UI renders this agent from cache on a later offline start.
    persistFetchedProfile(id, responseCbor);
    emit profileSpecLoaded(id);
}

void ProfileRepository::handleSoulGetResponse(const QString& id, const QByteArray& responseCbor) {
    QString text;
    if (!NodeApiCodec::decodeSoulText(responseCbor, &text)) {
        // An ApiError (unknown profile / no persona store) or a decode miss: a read gap the
        // persona editor already gates on (foreign profiles hide the surface). Do NOT toast off a
        // read error — the contract makes editor visibility a design decision, not a read outcome.
        return;
    }
    m_souls.insert(id, text);
    emit soulLoaded(id);
}

void ProfileRepository::handleSoulSetResponse(const QString& id, const QByteArray& responseCbor) {
    if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Error) {
        DecodedApiError err;
        emit soulOpFailed(NodeApiCodec::decodeError(responseCbor, &err)
                              ? err.message
                              : tr("Persona save failed"));
        return;
    }
    // Ok: the node validated/scanned/capped and stored the persona (and emits ProfilesChanged for
    // thin clients). Re-fetch the authoritative stored text via SoulGet rather than echoing the
    // submitted string — soulLoaded fires once when that answer lands (one signal per fresh read).
    requestSoul(id);
}

void ProfileRepository::persistFetchedProfile(const QString& id, const QByteArray& responseCbor) {
    if (cache() == nullptr) {
        return;
    }
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

void ProfileRepository::handleProfileExportResponse(const QString& id,
                                                    const QByteArray& responseCbor) {
    DecodedDistribution dist;
    if (!NodeApiCodec::decodeDistribution(responseCbor, &dist)) {
        emit operationFailed(QStringLiteral("Failed to decode Distribution"));
        return;
    }
    // Hand the store the raw bytes (the portable artifact) + the decoded form.
    emit distributionExported(id, responseCbor, dist);
}

void ProfileRepository::handleProfileImportResponse(const QByteArray& responseCbor) {
    QString newId;
    if (!NodeApiCodec::decodeProfileId(responseCbor, &newId)) {
        DecodedApiError err;
        emit operationFailed(NodeApiCodec::decodeError(responseCbor, &err)
                                 ? err.message
                                 : tr("Profile import failed"));
        return;
    }
    refreshProfiles();
    emit imported(newId);
}

void ProfileRepository::handleProfileHistoryResponse(const QString& id,
                                                     const QByteArray& responseCbor) {
    // A daemon with no revision log answers Unsupported("versioning not available"); that is a
    // capability gap, not a decode failure - surface it as historyUnavailable so the UI can hide
    // the panel honestly rather than toast a misleading parse error.
    if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Error) {
        m_historyLoops.remove(id);
        DecodedApiError err;
        emit historyUnavailable(id, NodeApiCodec::decodeError(responseCbor, &err)
                                        ? err.message
                                        : tr("Version history unavailable"));
        return;
    }
    QList<DecodedRevision> revs;
    QString next;
    if (!NodeApiCodec::decodeRevisions(responseCbor, &revs, &next)) {
        m_historyLoops.remove(id);
        emit operationFailed(QStringLiteral("Failed to decode Revisions"));
        return;
    }
    // Page loop: accumulate and re-issue with the cursor until it clears, then emit the full
    // history ONCE (the panel replaces on historyLoaded).
    PageLoop<DecodedRevision>& loop = m_historyLoops[id];
    loop.items.append(revs);
    if (!next.isEmpty() && loop.guard(next) && client() != nullptr) {
        client()->sendRequest(NodeApiCodec::encodeProfileHistoryRequest(id, next),
                              QLatin1String(kHistoryPrefix) + id);
        return;
    }
    const QList<DecodedRevision> history = loop.items;
    m_historyLoops.remove(id);
    emit historyLoaded(id, history);
}

void ProfileRepository::handleProfileRevertResponse(const QString& id,
                                                    const QByteArray& responseCbor) {
    if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Error) {
        DecodedApiError err;
        emit operationFailed(NodeApiCodec::decodeError(responseCbor, &err)
                                 ? err.message
                                 : tr("Profile revert failed"));
        return;
    }
    // Rolled forward: refresh the spec (re-get) + the list, then notify.
    getProfile(id);
    refreshProfiles();
    emit reverted(id);
}

void ProfileRepository::handleProfileMutationResponse(const QByteArray& responseCbor) {
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
        emit operationFailed(tr("Profile operation failed"));
    }
}

bool ProfileRepository::isProfileOperation(const QString& correlationId) {
    const std::array exact = {kSelectCorrelation, kDeleteCorrelation, kUpdateCorrelation,
                              kCreateCorrelation, kCloneCorrelation,  kImportCorrelation};
    if (std::ranges::any_of(exact,
                            [&](const char* key) { return correlationId == QLatin1String(key); })) {
        return true;
    }
    const std::array prefixes = {kGetPrefix, kExportPrefix, kHistoryPrefix, kRevertPrefix};
    return std::ranges::any_of(prefixes, [&](const char* prefix) {
        return correlationId.startsWith(QLatin1String(prefix));
    });
}

void ProfileRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kProfilesCorrelation)) {
        emit refreshFailed(message);
        return;
    }
    // A SoulSet that never reached the node (transport failure) is a failed save -> surface it.
    // A SoulGet transport failure is a silent read gap (same rationale as its ApiError arm).
    if (correlationId.startsWith(QLatin1String(kSoulSetPrefix))) {
        emit soulOpFailed(message);
        return;
    }
    if (correlationId.startsWith(QLatin1String(kSoulGetPrefix))) {
        return;
    }
    if (isProfileOperation(correlationId)) {
        // A dead history page loop must not leak its partial accumulation into the next fetch.
        if (correlationId.startsWith(QLatin1String(kHistoryPrefix))) {
            m_historyLoops.remove(correlationId.mid(int(qstrlen(kHistoryPrefix))));
        }
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
    // A fresh refresh restarts the page loop: drop anything a superseded loop accumulated.
    m_pendingLoop.reset();
    client()->sendRequest(NodeApiCodec::encodeApprovalsPendingRequest(sessionId),
                          QLatin1String(kPendingCorrelation));
}

void ApprovalRepository::decide(const QString& sessionId, const QString& requestId, bool allow,
                                bool allowPermanent, const QString& reason) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    // [wave2:app-approvals-safety] D3: thread the optional deny reason (wire v29) to the encoder.
    client()->sendRequest(NodeApiCodec::encodeApprovalDecideRequest(sessionId, requestId, allow,
                                                                    allowPermanent, reason),
                          QLatin1String(kDecideCorrelation));
}

void ApprovalRepository::handleResponse(const QString& correlationId,
                                        const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kPendingCorrelation)) {
        QList<DecodedApprovalInfo> pending;
        QString next;
        if (!NodeApiCodec::decodeApprovals(responseCbor, &pending, &next)) {
            m_pendingLoop.reset();
            emit operationFailed(QStringLiteral("Failed to decode Approvals response"));
            return;
        }
        // Page loop: accumulate and re-issue with the cursor until it clears, then swap the
        // complete inbox in ONCE (the badge/list replace on pendingRefreshed).
        m_pendingLoop.items.append(pending);
        if (!next.isEmpty() && m_pendingLoop.guard(next) && client() != nullptr) {
            client()->sendRequest(NodeApiCodec::encodeApprovalsPendingRequest(m_lastSession, next),
                                  QLatin1String(kPendingCorrelation));
            return;
        }
        m_pending = m_pendingLoop.items;
        m_pendingLoop.reset();
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
            emit operationFailed(tr("Approval decision failed"));
        }
        return;
    }
}

void ApprovalRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kPendingCorrelation) ||
        correlationId == QLatin1String(kDecideCorrelation)) {
        // A dead page loop must not leak its partial accumulation into the next refresh.
        m_pendingLoop.reset();
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
    if (correlationId == QLatin1String(kControlCorrelation)) {
        handleUnitControlResponse(responseCbor);
    }
}

void FleetRepository::handleUnitControlResponse(const QByteArray& responseCbor) {
    // Pause/Resume/Scale answer Ok or an ApiError (e.g. Unsupported on an engine-leaf unit ->
    // surface it, do not silently swallow). AD: the ack no longer refetches the legacy cache
    // tree — controlAcked is bridged to the ingestor's Tree refetch (MirrorFleetTree), so the
    // post-ack re-render reaches the ONE mirror read path.
    if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Error) {
        DecodedApiError err;
        NodeApiCodec::decodeError(responseCbor, &err);
        emit controlFailed(err.message.isEmpty() ? tr("Unit control failed") : err.message);
    } else {
        emit controlAcked();
    }
}

void FleetRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kControlCorrelation)) {
        emit controlFailed(message);
    }
}

// [wave2:app-delegation] F7/DEL-7: the read-only delegation guardrail ceilings.
CapsRepository::CapsRepository(NodeApiClient* client, QObject* parent)
    : RepositoryBase(client, nullptr, parent) {
    if (this->client() != nullptr) {
        connect(this->client(), &NodeApiClient::responseReady, this,
                &CapsRepository::handleResponse);
        connect(this->client(), &NodeApiClient::failed, this, &CapsRepository::handleFailure);
    }
}

void CapsRepository::refresh() {
    if (client() == nullptr) {
        emit refreshFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeCapsRequest(), QLatin1String(kCapsCorrelation));
}

void CapsRepository::handleResponse(const QString& correlationId, const QByteArray& responseCbor) {
    if (correlationId != QLatin1String(kCapsCorrelation)) {
        return;
    }
    if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Error) {
        DecodedApiError err;
        NodeApiCodec::decodeError(responseCbor, &err);
        emit refreshFailed(err.message.isEmpty() ? tr("Failed to read delegation limits")
                                                 : err.message);
        return;
    }
    if (!NodeApiCodec::decodeCaps(responseCbor, &m_caps)) {
        emit refreshFailed(tr("Failed to decode delegation limits"));
        return;
    }
    m_loaded = true;
    emit capsRefreshed();
}

void CapsRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kCapsCorrelation)) {
        emit refreshFailed(message);
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
    // A fresh refresh restarts the transport's page loop.
    m_convLoops[transport].reset();
    client()->sendRequest(NodeApiCodec::encodeConvListRequest(transport),
                          QLatin1String(kConvPrefix) + transport);
}

// [waveB:app-v30] D2: seed every cached account's ConvList once (connect-ready baseline). Uses the
// offline-first cached instance list so it works the moment the cache is warm; freshly-discovered
// accounts get seeded by their TransportChanged(connected) event.
void TransportRepository::refreshAllConversations() {
    if (client() == nullptr || cache() == nullptr) {
        return;
    }
    for (const CachedTransportInstanceRow& row : cache()->transportInstances()) {
        refreshConversations(row.transport);
    }
}

// [waveB:app-v30] D1: teardown intents. One request; on Ok re-list so the client renders the
// node's reported outcome (state, or the account's disappearance) rather than a local mutation.
void TransportRepository::disconnect(const QString& transport) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeTransportDisconnectRequest(transport),
                          QLatin1String(kDisconnectCorrelation));
}

void TransportRepository::remove(const QString& transport) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeTransportRemoveRequest(transport),
                          QLatin1String(kRemoveCorrelation));
}

// [acct-mgmt] wire v35: reversible lifecycle + persisted metadata. One request; on Ok re-list so
// the client renders the node's reported state (the node also emits TransportChanged) — never
// optimistic.
void TransportRepository::connectTransport(const QString& transport) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeTransportConnectRequest(transport),
                          QLatin1String(kConnectCorrelation));
}

void TransportRepository::setEnabled(const QString& transport, bool enabled) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeTransportSetEnabledRequest(transport, enabled),
                          QLatin1String(kSetEnabledCorrelation));
}

void TransportRepository::setTransportLabel(const QString& transport, bool hasLabel,
                                            const QString& label) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeTransportSetLabelRequest(transport, hasLabel, label),
                          QLatin1String(kSetLabelCorrelation));
}

void TransportRepository::handleResponse(const QString& correlationId,
                                         const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kAdaptersCorrelation)) {
        handleAdaptersResponse(responseCbor);
        return;
    }
    if (correlationId == QLatin1String(kInstancesCorrelation)) {
        handleInstancesResponse(responseCbor);
        return;
    }
    // [waveB:app-v30] D1: disconnect/remove both re-list on Ok; surface the node's error otherwise.
    // [acct-mgmt] wire v35: connect/set-enabled/set-label join the same re-list-on-Ok path.
    if (correlationId == QLatin1String(kDisconnectCorrelation) ||
        correlationId == QLatin1String(kRemoveCorrelation) ||
        correlationId == QLatin1String(kConnectCorrelation) ||
        correlationId == QLatin1String(kSetEnabledCorrelation) ||
        correlationId == QLatin1String(kSetLabelCorrelation)) {
        const ApiResponseKind kind = NodeApiCodec::responseKind(responseCbor);
        if (kind == ApiResponseKind::Ok) {
            refreshInstances();
            return;
        }
        DecodedApiError err;
        if (kind == ApiResponseKind::Error && NodeApiCodec::decodeError(responseCbor, &err)) {
            emit operationFailed(err.message);
        } else if (correlationId == QLatin1String(kRemoveCorrelation)) {
            emit operationFailed(tr("Failed to remove the account"));
        } else if (correlationId == QLatin1String(kConnectCorrelation)) {
            emit operationFailed(tr("Failed to connect the account"));
        } else if (correlationId == QLatin1String(kSetEnabledCorrelation)) {
            emit operationFailed(tr("Failed to change the account's enabled state"));
        } else if (correlationId == QLatin1String(kSetLabelCorrelation)) {
            emit operationFailed(tr("Failed to rename the account"));
        } else {
            emit operationFailed(tr("Failed to disconnect the account"));
        }
        return;
    }
    if (correlationId.startsWith(QLatin1String(kConvPrefix))) {
        handleConversationsResponse(correlationId.mid(int(qstrlen(kConvPrefix))), responseCbor);
        return;
    }
    // [acct-mgmt] Room lifecycle + member ops. The transport (+ conv, split on \x1f) rides the
    // tail.
    const auto tailOf = [&](const char* prefix) { return correlationId.mid(int(qstrlen(prefix))); };
    const auto splitTwo = [](const QString& tail, QString* conv) {
        const qsizetype us = tail.indexOf(QChar(0x1f));
        if (us < 0) {
            *conv = QString();
            return tail;
        }
        *conv = tail.mid(us + 1);
        return tail.left(us);
    };
    if (correlationId.startsWith(QLatin1String(kJoinDetailsPrefix))) {
        handleJoinDetailsResponse(tailOf(kJoinDetailsPrefix), responseCbor);
    } else if (correlationId.startsWith(QLatin1String(kCreateDetailsPrefix))) {
        handleCreateDetailsResponse(tailOf(kCreateDetailsPrefix), responseCbor);
    } else if (correlationId.startsWith(QLatin1String(kJoinPrefix))) {
        handleRoomOkResponse(tailOf(kJoinPrefix), responseCbor, tr("Failed to join the room"));
    } else if (correlationId.startsWith(QLatin1String(kCreatePrefix))) {
        handleRoomOkResponse(tailOf(kCreatePrefix), responseCbor, tr("Failed to create the room"));
    } else if (correlationId.startsWith(QLatin1String(kLeavePrefix))) {
        handleRoomOkResponse(tailOf(kLeavePrefix), responseCbor, tr("Failed to leave the room"));
    } else if (correlationId.startsWith(QLatin1String(kDeletePrefix))) {
        handleRoomOkResponse(tailOf(kDeletePrefix), responseCbor, tr("Failed to delete the room"));
    } else if (correlationId.startsWith(QLatin1String(kConvGetPrefix))) {
        QString conv;
        const QString transport = splitTwo(tailOf(kConvGetPrefix), &conv);
        handleConversationGetResponse(transport, conv, responseCbor);
    } else if (correlationId.startsWith(QLatin1String(kMemberOpPrefix))) {
        QString conv;
        const QString transport = splitTwo(tailOf(kMemberOpPrefix), &conv);
        handleMemberOpResponse(transport, conv, responseCbor);
    } else if (correlationId.startsWith(QLatin1String(kSettingsPrefix))) {
        // [integrations wire v38] TransportSettings read-back (the transport rides the tail).
        handleSettingsResponse(tailOf(kSettingsPrefix), responseCbor);
    } else if (correlationId.startsWith(QLatin1String(kConfigurePrefix))) {
        // [integrations wire v38] TransportConfigure Ok -> re-read the settings (authoritative).
        handleConfigureResponse(tailOf(kConfigurePrefix), responseCbor);
    }
}

void TransportRepository::handleAdaptersResponse(const QByteArray& responseCbor) {
    if (!NodeApiCodec::decodeAdapters(responseCbor, &m_adapters)) {
        emit operationFailed(QStringLiteral("Failed to decode Adapters response"));
        return;
    }
    emit adaptersRefreshed();
}

void TransportRepository::handleInstancesResponse(const QByteArray& responseCbor) {
    QList<DecodedTransportInstance> live;
    if (!NodeApiCodec::decodeTransportInstances(responseCbor, &live)) {
        emit operationFailed(QStringLiteral("Failed to decode TransportInstances response"));
        return;
    }
    syncTransportInstances(live);
    emit instancesRefreshed();
}

void TransportRepository::syncTransportInstances(const QList<DecodedTransportInstance>& live) {
    if (cache() == nullptr) {
        return;
    }
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
        // [waveB:app-v30] D1: node-reported disconnect provenance (offline-first).
        row.connectionReason = i.reason;
        row.connectionMessage = i.message;
        row.fatal = i.fatal;
        // [acct-mgmt] wire v35: node-persisted desired state + display-label override.
        row.enabled = i.enabled;
        row.label = i.label;
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

// --- CheckpointRepository (E4/TOOL-9) --------------------------------------------------------

CheckpointRepository::CheckpointRepository(NodeApiClient* client, DaemonCacheStore* cache,
                                           QObject* parent)
    : RepositoryBase(client, cache, parent) {
    if (this->client() != nullptr) {
        connect(this->client(), &NodeApiClient::responseReady, this,
                &CheckpointRepository::handleResponse);
        connect(this->client(), &NodeApiClient::failed, this, &CheckpointRepository::handleFailure);
    }
}

void CheckpointRepository::refresh(const QString& sessionId) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    m_lastSession = sessionId;
    // A fresh refresh restarts the page loop: drop anything a superseded loop accumulated.
    m_listLoop.reset();
    client()->sendRequest(NodeApiCodec::encodeCheckpointListRequest(sessionId),
                          QLatin1String(kListCorrelation));
}

void CheckpointRepository::rewind(const QString& sessionId, const QString& checkpointId) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    m_pendingRewind = sessionId;
    client()->sendRequest(NodeApiCodec::encodeCheckpointRewindRequest(sessionId, checkpointId),
                          QLatin1String(kRewindCorrelation));
}

void CheckpointRepository::handleResponse(const QString& correlationId,
                                          const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kListCorrelation)) {
        QList<DecodedCheckpointInfo> page;
        QString next;
        if (!NodeApiCodec::decodeCheckpoints(responseCbor, &page, &next)) {
            m_listLoop.reset();
            emit operationFailed(QStringLiteral("Failed to decode Checkpoints response"));
            return;
        }
        // Page loop: accumulate and re-issue with the cursor until it clears, then swap the
        // complete timeline in ONCE.
        m_listLoop.items.append(page);
        if (!next.isEmpty() && m_listLoop.guard(next) && client() != nullptr) {
            client()->sendRequest(NodeApiCodec::encodeCheckpointListRequest(m_lastSession, next),
                                  QLatin1String(kListCorrelation));
            return;
        }
        m_checkpoints = m_listLoop.items;
        m_listLoop.reset();
        emit checkpointsRefreshed(m_lastSession);
        return;
    }
    if (correlationId == QLatin1String(kRewindCorrelation)) {
        const ApiResponseKind kind = NodeApiCodec::responseKind(responseCbor);
        if (kind == ApiResponseKind::Ok) {
            emit rewound(m_pendingRewind);
            // Re-sync: the node prunes checkpoints past the restore point.
            refresh(m_pendingRewind.isEmpty() ? m_lastSession : m_pendingRewind);
            return;
        }
        DecodedApiError err;
        if (kind == ApiResponseKind::Error && NodeApiCodec::decodeError(responseCbor, &err)) {
            emit operationFailed(err.message);
        } else {
            emit operationFailed(tr("Checkpoint rewind failed"));
        }
        return;
    }
}

void CheckpointRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kListCorrelation) ||
        correlationId == QLatin1String(kRewindCorrelation)) {
        // A dead page loop must not leak its partial accumulation into the next refresh.
        m_listLoop.reset();
        emit operationFailed(message);
    }
}

// --- RoutingRepository (B6/ROU) ---------------------------------------------------------------

RoutingRepository::RoutingRepository(NodeApiClient* client, DaemonCacheStore* cache,
                                     QObject* parent)
    : RepositoryBase(client, cache, parent) {
    if (this->client() != nullptr) {
        connect(this->client(), &NodeApiClient::responseReady, this,
                &RoutingRepository::handleResponse);
        connect(this->client(), &NodeApiClient::failed, this, &RoutingRepository::handleFailure);
    }
}

namespace {
// domain::Origin -> the codec's flattened DecodedOrigin (what the Routing* encoders take). The
// inverse of the mirror map's origin decode; kept local to the routing mutation seam.
DecodedOrigin toDecodedOrigin(const domain::Origin& o) {
    DecodedOrigin d;
    d.transport = o.transport.toString();
    switch (o.scope.kind) {
    case domain::OriginScopeKind::Dm:
        d.scopeKind = QStringLiteral("dm");
        d.user = o.scope.user;
        break;
    case domain::OriginScopeKind::Group:
        d.scopeKind = QStringLiteral("group");
        d.chat = o.scope.chat;
        d.hasThread = !o.scope.thread.isEmpty();
        d.thread = o.scope.thread;
        break;
    case domain::OriginScopeKind::Api:
        d.scopeKind = QStringLiteral("api");
        d.apiKey = o.scope.apiKey;
        break;
    case domain::OriginScopeKind::Internal:
        d.scopeKind = QStringLiteral("internal");
        break;
    }
    return d;
}
} // namespace

void RoutingRepository::routingBindChat(const domain::Origin& origin,
                                        const domain::SessionId& session,
                                        const domain::ProfileRef& profile) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeRoutingBindChatRequest(
                              toDecodedOrigin(origin), session.toString(), profile.toString()),
                          QLatin1String(kMutateCorrelation));
}

void RoutingRepository::routingUnbindChat(const domain::Origin& origin) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeRoutingUnbindChatRequest(toDecodedOrigin(origin)),
                          QLatin1String(kMutateCorrelation));
}

void RoutingRepository::handleMutationResponse(const QByteArray& responseCbor) {
    if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Ok) {
        // Node-authoritative: announce the ack — the graph bridges it to the ingestor's
        // RoutingListChats refetch so the MIRROR pin table renders the stored state (AD: the
        // repo's dead in-memory re-list died; the mirror is the only read path).
        emit mutationApplied();
        return;
    }
    DecodedApiError err;
    if (NodeApiCodec::decodeError(responseCbor, &err)) {
        emit operationFailed(err.message);
    } else {
        emit operationFailed(tr("Routing change failed"));
    }
}

void RoutingRepository::handleResponse(const QString& correlationId,
                                       const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kMutateCorrelation)) {
        handleMutationResponse(responseCbor);
    }
}

void RoutingRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kMutateCorrelation)) {
        emit operationFailed(message);
    }
}

void TransportRepository::handleConversationsResponse(const QString& transport,
                                                      const QByteArray& responseCbor) {
    QList<DecodedConversation> live;
    QString next;
    if (!NodeApiCodec::decodeConversations(responseCbor, &live, &next)) {
        m_convLoops.remove(transport);
        emit operationFailed(QStringLiteral("Failed to decode Conversations response"));
        return;
    }
    // Page loop: accumulate and re-issue with the cursor until it clears, then sync (replace +
    // prune) ONCE over the transport's whole listing, like today's single-shot path did.
    PageLoop<DecodedConversation>& loop = m_convLoops[transport];
    loop.items.append(live);
    if (!next.isEmpty() && loop.guard(next) && client() != nullptr) {
        client()->sendRequest(NodeApiCodec::encodeConvListRequest(transport, next),
                              QLatin1String(kConvPrefix) + transport);
        return;
    }
    const QList<DecodedConversation> all = loop.items;
    m_convLoops.remove(transport);
    syncConversations(transport, all);
    emit conversationsRefreshed(transport);
}

void TransportRepository::syncConversations(const QString& transport,
                                            const QList<DecodedConversation>& live) {
    if (cache() == nullptr) {
        return;
    }
    // [wave2:app-channels-liveness] B2: the FIRST refresh of this transport in the session
    // establishes the baseline (everything present is "already seen", no badges) - this avoids
    // restart noise regardless of the offline-first cache contents. On SUBSEQUENT refreshes, a
    // live id absent from the known set is a genuinely newly-surfaced room (e.g. an auto-accepted
    // invite) -> badge it. Membership itself is the node's; this only tracks what the operator has
    // already seen.
    const bool firstSync = !m_knownConversations.contains(transport);
    QSet<QString>& known = m_knownConversations[transport]; // inserts an empty set on first sync
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    QSet<QString> keep;
    for (const DecodedConversation& c : live) {
        CachedConversationRow row;
        row.transport = transport; // key by the requested transport for consistent pruning
        row.convId = c.id;
        row.kind = c.kind;
        row.title = c.title;
        row.topic = c.topic;
        // [integrations wire v38] persist the containing space id (empty = a root) so the
        // integrations tree's hierarchy renders offline-first.
        row.parent = c.hasParent ? c.parent : QString();
        row.updatedAtMs = now;
        cache()->upsertConversation(row);
        keep.insert(c.id);
        if (!firstSync && !known.contains(c.id)) {
            m_newConversations[transport].insert(c.id);
        }
        known.insert(c.id);
    }
    for (const CachedConversationRow& existing : cache()->conversations(transport)) {
        if (!keep.contains(existing.convId)) {
            cache()->deleteConversation(transport, existing.convId);
            // A room that left drops its known/new marks (a later re-join badges it fresh).
            known.remove(existing.convId);
            m_newConversations[transport].remove(existing.convId);
        }
    }
}

void TransportRepository::applyTransportChanged(const QString& transport, const QString& connection,
                                                const QString& presence, bool hasPresence,
                                                const QString& reason, bool hasReason,
                                                const QString& message, bool hasMessage,
                                                bool fatal) {
    if (cache() == nullptr || transport.isEmpty()) {
        return;
    }
    // Patch the cached row in place, preserving the fields the event does not carry. If the
    // transport is not cached yet (a brand-new account before its first TransportInstances), fall
    // back to an authoritative full refetch instead of inventing a partial row.
    bool found = false;
    for (const CachedTransportInstanceRow& existing : cache()->transportInstances()) {
        if (existing.transport != transport) {
            continue;
        }
        CachedTransportInstanceRow row = existing;
        row.connection = connection;
        if (hasPresence) {
            row.presence = presence;
        }
        // [waveB:app-v30] D1: patch the disconnect provenance the event carried. The optionals are
        // authoritative-when-present: a non-error transition clears reason/message; `fatal` always
        // rides the event.
        row.connectionReason = hasReason ? reason : QString();
        row.connectionMessage = hasMessage ? message : QString();
        row.fatal = fatal;
        row.updatedAtMs = QDateTime::currentMSecsSinceEpoch();
        cache()->upsertTransportInstance(row);
        found = true;
        break;
    }
    if (!found) {
        refreshInstances();
        return;
    }
    // The mirror TransportChanged patch renders the same state on the account rows;
    // the GUI/TUI Channels status dots re-read reactively (no poll).
    emit instancesRefreshed();
}

bool TransportRepository::isNewConversation(const QString& transport, const QString& conv) const {
    const auto it = m_newConversations.constFind(transport);
    return it != m_newConversations.constEnd() && it->contains(conv);
}

void TransportRepository::markConversationSeen(const QString& transport, const QString& conv) {
    const auto it = m_newConversations.find(transport);
    if (it != m_newConversations.end()) {
        it->remove(conv);
    }
}

// --- [acct-mgmt] Room lifecycle + member management -------------------------------------------

namespace {

// A key->value QMap<QString,QString> pulled from a QVariantMap "extras" child.
QMap<QString, QString> extrasFromForm(const QVariantMap& form) {
    QMap<QString, QString> out;
    const QVariantMap extras = form.value(QStringLiteral("extras")).toMap();
    for (auto it = extras.constBegin(); it != extras.constEnd(); ++it) {
        out.insert(it.key(), it.value().toString());
    }
    return out;
}

// Fold a DecodedChannelJoinDetails into the QVariantMap descriptor the dialogs render.
QVariantMap joinDetailsToVariant(const DecodedChannelJoinDetails& d) {
    QVariantMap m;
    m[QStringLiteral("nameMaxLength")] = d.hasNameMaxLength ? int(d.nameMaxLength) : 0;
    m[QStringLiteral("nicknameSupported")] = d.nicknameSupported;
    m[QStringLiteral("nicknameMaxLength")] = d.hasNicknameMaxLength ? int(d.nicknameMaxLength) : 0;
    m[QStringLiteral("passwordSupported")] = d.passwordSupported;
    m[QStringLiteral("passwordMaxLength")] = d.hasPasswordMaxLength ? int(d.passwordMaxLength) : 0;
    QVariantList schema;
    for (const DecodedSettingsField& f : d.extrasSchema) {
        QVariantMap fm;
        fm[QStringLiteral("key")] = f.key;
        fm[QStringLiteral("label")] = f.label;
        fm[QStringLiteral("required")] = f.required;
        schema.append(fm);
    }
    m[QStringLiteral("extras")] = schema;
    return m;
}

QVariantMap createDetailsToVariant(const DecodedCreateConversationDetails& d) {
    QVariantMap m;
    m[QStringLiteral("maxParticipants")] = d.hasMaxParticipants ? int(d.maxParticipants) : 0;
    QVariantList schema;
    for (const DecodedSettingsField& f : d.extrasSchema) {
        QVariantMap fm;
        fm[QStringLiteral("key")] = f.key;
        fm[QStringLiteral("label")] = f.label;
        fm[QStringLiteral("required")] = f.required;
        schema.append(fm);
    }
    m[QStringLiteral("extras")] = schema;
    return m;
}

QVariantList membersToVariant(const QList<DecodedConversationMember>& members) {
    QVariantList out;
    for (const DecodedConversationMember& m : members) {
        QVariantMap row;
        row[QStringLiteral("contactId")] = m.contactId;
        row[QStringLiteral("displayName")] = m.displayName;
        row[QStringLiteral("presence")] = m.presence;
        row[QStringLiteral("permission")] = m.permission;
        row[QStringLiteral("alias")] = m.alias;
        row[QStringLiteral("nickname")] = m.nickname;
        row[QStringLiteral("typing")] = m.typing;
        row[QStringLiteral("role")] = m.role;
        row[QStringLiteral("session")] = m.session;
        out.append(row);
    }
    return out;
}

} // namespace

void TransportRepository::conversationJoinDetails(const QString& transport) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeConvJoinDetailsRequest(transport),
                          QLatin1String(kJoinDetailsPrefix) + transport);
}

void TransportRepository::conversationCreateDetails(const QString& transport) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeConvCreateDetailsRequest(transport),
                          QLatin1String(kCreateDetailsPrefix) + transport);
}

void TransportRepository::joinRoom(const QString& transport, const QVariantMap& form) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    ConvJoinForm f;
    f.name = form.value(QStringLiteral("name")).toString();
    const QString nickname = form.value(QStringLiteral("nickname")).toString();
    f.hasNickname = !nickname.isEmpty();
    f.nickname = nickname;
    const QString password = form.value(QStringLiteral("password")).toString();
    f.hasPassword = !password.isEmpty();
    f.password = password;
    f.extras = extrasFromForm(form);
    // [api/39 §10.3 rung 3] a client-minted op-id makes the direct join retry-safe (the node
    // dedups on (principal, op_id) — a resent form cannot join twice).
    client()->sendRequest(
        NodeApiCodec::encodeConvJoinRequest(transport, f, mirror::generateUuidV7()),
        QLatin1String(kJoinPrefix) + transport);
}

void TransportRepository::createRoom(const QString& transport, const QVariantMap& form) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    ConvCreateForm f;
    if (form.contains(QStringLiteral("maxParticipants"))) {
        const int mx = form.value(QStringLiteral("maxParticipants")).toInt();
        // 0 = unlimited: still send it explicitly so the node's default is not assumed.
        f.hasMaxParticipants = mx >= 0;
        f.maxParticipants = static_cast<quint32>(qMax(0, mx));
    }
    for (const QVariant& p : form.value(QStringLiteral("participants")).toList()) {
        const QString id = p.toString();
        if (!id.isEmpty()) {
            f.participants.append(id);
        }
    }
    f.extras = extrasFromForm(form);
    // [api/39 §10.3 rung 3] retry-safe direct create — no duplicate room on resend.
    client()->sendRequest(
        NodeApiCodec::encodeConvCreateRequest(transport, f, mirror::generateUuidV7()),
        QLatin1String(kCreatePrefix) + transport);
}

void TransportRepository::leaveRoom(const QString& transport, const QString& conversation) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeConvLeaveRequest(transport, conversation),
                          QLatin1String(kLeavePrefix) + transport);
}

void TransportRepository::deleteRoom(const QString& transport, const QString& conversation) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeConvDeleteRequest(transport, conversation),
                          QLatin1String(kDeletePrefix) + transport);
}

void TransportRepository::conversationMembers(const QString& transport,
                                              const QString& conversation) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeConvGetRequest(transport, conversation),
                          QLatin1String(kConvGetPrefix) + transport + QChar(0x1f) + conversation);
}

void TransportRepository::memberInvite(const QString& transport, const QString& conversation,
                                       const QString& contactId) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeMemberInviteRequest(
                              transport, conversation, contactId, mirror::generateUuidV7()),
                          QLatin1String(kMemberOpPrefix) + transport + QChar(0x1f) + conversation);
}

void TransportRepository::memberKick(const QString& transport, const QString& conversation,
                                     const QString& contactId) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeMemberRemoveRequest(transport, conversation,
                                                                  contactId, QString(),
                                                                  mirror::generateUuidV7()),
                          QLatin1String(kMemberOpPrefix) + transport + QChar(0x1f) + conversation);
}

void TransportRepository::memberBan(const QString& transport, const QString& conversation,
                                    const QString& contactId) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeMemberBanRequest(transport, conversation, contactId,
                                                               QString(), mirror::generateUuidV7()),
                          QLatin1String(kMemberOpPrefix) + transport + QChar(0x1f) + conversation);
}

void TransportRepository::memberSetRole(const QString& transport, const QString& conversation,
                                        const QString& contactId, const QString& role) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeMemberSetRoleRequest(
                              transport, conversation, contactId, role, mirror::generateUuidV7()),
                          QLatin1String(kMemberOpPrefix) + transport + QChar(0x1f) + conversation);
}

void TransportRepository::handleJoinDetailsResponse(const QString& transport,
                                                    const QByteArray& responseCbor) {
    DecodedChannelJoinDetails d;
    if (!NodeApiCodec::decodeConvJoinDetails(responseCbor, &d)) {
        DecodedApiError err;
        if (NodeApiCodec::decodeError(responseCbor, &err)) {
            emit operationFailed(err.message);
        } else {
            emit operationFailed(tr("Failed to load the join form"));
        }
        return;
    }
    emit joinDetailsReady(transport, joinDetailsToVariant(d));
}

void TransportRepository::handleCreateDetailsResponse(const QString& transport,
                                                      const QByteArray& responseCbor) {
    DecodedCreateConversationDetails d;
    if (!NodeApiCodec::decodeConvCreateDetails(responseCbor, &d)) {
        DecodedApiError err;
        if (NodeApiCodec::decodeError(responseCbor, &err)) {
            emit operationFailed(err.message);
        } else {
            emit operationFailed(tr("Failed to load the new-room form"));
        }
        return;
    }
    emit createDetailsReady(transport, createDetailsToVariant(d));
}

void TransportRepository::handleRoomOkResponse(const QString& transport,
                                               const QByteArray& responseCbor,
                                               const QString& failMessage) {
    const ApiResponseKind kind = NodeApiCodec::responseKind(responseCbor);
    if (kind == ApiResponseKind::Ok) {
        // The node's ConversationsChanged event also refetches, but refresh now so the room
        // list reflects the mutation without waiting for the feed.
        refreshConversations(transport);
        return;
    }
    DecodedApiError err;
    if (kind == ApiResponseKind::Error && NodeApiCodec::decodeError(responseCbor, &err)) {
        emit operationFailed(err.message);
    } else {
        emit operationFailed(failMessage);
    }
}

void TransportRepository::handleConversationGetResponse(const QString& transport,
                                                        const QString& conv,
                                                        const QByteArray& responseCbor) {
    DecodedConversation d;
    bool found = false;
    if (!NodeApiCodec::decodeConversation(responseCbor, &d, &found)) {
        DecodedApiError err;
        if (NodeApiCodec::decodeError(responseCbor, &err)) {
            emit operationFailed(err.message);
        } else {
            emit operationFailed(tr("Failed to load the room members"));
        }
        return;
    }
    emit membersReady(transport, conv, found ? membersToVariant(d.members) : QVariantList{});
}

void TransportRepository::handleMemberOpResponse(const QString& transport, const QString& conv,
                                                 const QByteArray& responseCbor) {
    const ApiResponseKind kind = NodeApiCodec::responseKind(responseCbor);
    if (kind == ApiResponseKind::Ok) {
        // Re-hydrate the member palette (the MembershipChanged event refetches the ConvList).
        conversationMembers(transport, conv);
        return;
    }
    DecodedApiError err;
    if (kind == ApiResponseKind::Error && NodeApiCodec::decodeError(responseCbor, &err)) {
        emit operationFailed(err.message);
    } else {
        emit operationFailed(tr("The membership action was rejected"));
    }
}

bool TransportRepository::isOwnCorrelation(const QString& correlationId) {
    // [waveB:app-v30] D1: +disconnect/remove correlations.
    // [acct-mgmt] wire v35: +connect/set-enabled/set-label correlations.
    const std::array keys = {kAdaptersCorrelation, kInstancesCorrelation, kDisconnectCorrelation,
                             kRemoveCorrelation,   kConnectCorrelation,   kSetEnabledCorrelation,
                             kSetLabelCorrelation};
    if (std::ranges::any_of(keys,
                            [&](const char* key) { return correlationId == QLatin1String(key); })) {
        return true;
    }
    // [acct-mgmt] room lifecycle + member-op correlations are all prefix-routed.
    const std::array prefixes = {kConvPrefix,   kJoinDetailsPrefix, kCreateDetailsPrefix,
                                 kJoinPrefix,   kCreatePrefix,      kLeavePrefix,
                                 kDeletePrefix, kConvGetPrefix,     kMemberOpPrefix};
    return std::ranges::any_of(prefixes, [&](const char* prefix) {
        return correlationId.startsWith(QLatin1String(prefix));
    });
}

void TransportRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (isOwnCorrelation(correlationId)) {
        // A dead conversations page loop must not leak its partial accumulation.
        if (correlationId.startsWith(QLatin1String(kConvPrefix))) {
            m_convLoops.remove(correlationId.mid(int(qstrlen(kConvPrefix))));
        }
        emit operationFailed(message);
    }
}

// --- [acct-mgmt] ContactsRepository (transport contacts / roster; wire v34) -------------------

ContactsRepository::ContactsRepository(NodeApiClient* client, DaemonCacheStore* cache,
                                       QObject* parent)
    : RepositoryBase(client, cache, parent) {
    if (this->client() != nullptr) {
        connect(this->client(), &NodeApiClient::responseReady, this,
                &ContactsRepository::handleResponse);
        connect(this->client(), &NodeApiClient::failed, this, &ContactsRepository::handleFailure);
    }
}

void ContactsRepository::refreshContacts(const QString& transport) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    // A fresh refresh restarts the transport's page loop.
    m_contactLoops[transport].reset();
    client()->sendRequest(NodeApiCodec::encodeRosterListRequest(transport),
                          QLatin1String(kListPrefix) + transport);
}

void ContactsRepository::addContact(const QString& transport, const QString& contactId) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    DecodedContact contact;
    contact.id = contactId;
    client()->sendRequest(NodeApiCodec::encodeRosterAddRequest(transport, contact),
                          QLatin1String(kAddPrefix) + transport);
}

void ContactsRepository::updateContact(const QString& transport, const DecodedContact& contact) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeRosterUpdateRequest(transport, contact),
                          QLatin1String(kUpdatePrefix) + transport);
}

void ContactsRepository::removeContact(const QString& transport, const QString& contactId) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    DecodedContact contact;
    contact.id = contactId;
    client()->sendRequest(NodeApiCodec::encodeRosterRemoveRequest(transport, contact),
                          QLatin1String(kRemovePrefix) + transport);
}

void ContactsRepository::setAlias(const QString& transport, const QString& contactId,
                                  const QString& alias) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    // Empty alias clears the local alias (the optional is omitted node-side).
    client()->sendRequest(
        NodeApiCodec::encodeContactSetAliasRequest(transport, contactId, !alias.isEmpty(), alias),
        QLatin1String(kAliasPrefix) + transport);
}

void ContactsRepository::getProfile(const QString& transport, const QString& contactId) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeContactGetProfileRequest(transport, contactId),
                          QLatin1String(kProfilePrefix) + transport + QChar(0x1f) + contactId);
}

void ContactsRepository::searchDirectory(const QString& transport, const QString& query) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeDirectorySearchRequest(transport, query),
                          QLatin1String(kDirectoryPrefix) + transport);
}

void ContactsRepository::handleResponse(const QString& correlationId,
                                        const QByteArray& responseCbor) {
    const auto tailOf = [&](const char* prefix) { return correlationId.mid(int(qstrlen(prefix))); };
    if (correlationId.startsWith(QLatin1String(kListPrefix))) {
        handleRosterListResponse(tailOf(kListPrefix), responseCbor);
    } else if (correlationId.startsWith(QLatin1String(kDirectoryPrefix))) {
        handleDirectoryResponse(tailOf(kDirectoryPrefix), responseCbor);
    } else if (correlationId.startsWith(QLatin1String(kProfilePrefix))) {
        const QString tail = tailOf(kProfilePrefix);
        const qsizetype us = tail.indexOf(QChar(0x1f));
        const QString transport = us < 0 ? tail : tail.left(us);
        const QString contactId = us < 0 ? QString() : tail.mid(us + 1);
        handleProfileResponse(transport, contactId, responseCbor);
    } else if (correlationId.startsWith(QLatin1String(kAddPrefix))) {
        handleMutationResponse(tailOf(kAddPrefix), responseCbor, tr("Failed to add the contact"));
    } else if (correlationId.startsWith(QLatin1String(kUpdatePrefix))) {
        handleMutationResponse(tailOf(kUpdatePrefix), responseCbor,
                               tr("Failed to update the contact"));
    } else if (correlationId.startsWith(QLatin1String(kRemovePrefix))) {
        handleMutationResponse(tailOf(kRemovePrefix), responseCbor,
                               tr("Failed to remove the contact"));
    } else if (correlationId.startsWith(QLatin1String(kAliasPrefix))) {
        handleMutationResponse(tailOf(kAliasPrefix), responseCbor,
                               tr("Failed to set the contact alias"));
    }
}

void ContactsRepository::handleRosterListResponse(const QString& transport,
                                                  const QByteArray& responseCbor) {
    QList<DecodedContact> page;
    QString next;
    if (!NodeApiCodec::decodeContactPage(responseCbor, &page, &next)) {
        m_contactLoops.remove(transport);
        DecodedApiError err;
        if (NodeApiCodec::decodeError(responseCbor, &err)) {
            emit operationFailed(err.message);
        } else {
            emit operationFailed(QStringLiteral("Failed to decode ContactPage response"));
        }
        return;
    }
    // Page loop: accumulate and re-issue with the cursor until it clears, then swap ONCE.
    PageLoop<DecodedContact>& loop = m_contactLoops[transport];
    loop.items.append(page);
    if (!next.isEmpty() && loop.guard(next) && client() != nullptr) {
        client()->sendRequest(NodeApiCodec::encodeRosterListRequest(transport, next),
                              QLatin1String(kListPrefix) + transport);
        return;
    }
    m_contacts[transport] = loop.items;
    m_contactLoops.remove(transport);
    emit contactsRefreshed(transport);
}

void ContactsRepository::handleDirectoryResponse(const QString& transport,
                                                 const QByteArray& responseCbor) {
    QList<DecodedContact> results;
    if (!NodeApiCodec::decodeContacts(responseCbor, &results)) {
        DecodedApiError err;
        if (NodeApiCodec::decodeError(responseCbor, &err)) {
            emit operationFailed(err.message);
        } else {
            emit operationFailed(tr("Failed to search the directory"));
        }
        return;
    }
    m_directory[transport] = results;
    emit directoryReady(transport);
}

void ContactsRepository::handleProfileResponse(const QString& transport, const QString& contactId,
                                               const QByteArray& responseCbor) {
    QString profile;
    if (!NodeApiCodec::decodeContactProfile(responseCbor, &profile)) {
        DecodedApiError err;
        if (NodeApiCodec::decodeError(responseCbor, &err)) {
            emit operationFailed(err.message);
        } else {
            emit operationFailed(tr("Failed to load the contact profile"));
        }
        return;
    }
    emit profileReady(transport, contactId, profile);
}

void ContactsRepository::handleMutationResponse(const QString& transport,
                                                const QByteArray& responseCbor,
                                                const QString& failMessage) {
    const ApiResponseKind kind = NodeApiCodec::responseKind(responseCbor);
    if (kind == ApiResponseKind::Ok) {
        // The node's ContactsChanged event also refetches, but refresh now so the roster reflects
        // the mutation without waiting for the feed.
        refreshContacts(transport);
        return;
    }
    DecodedApiError err;
    if (kind == ApiResponseKind::Error && NodeApiCodec::decodeError(responseCbor, &err)) {
        emit operationFailed(err.message);
    } else {
        emit operationFailed(failMessage);
    }
}

bool ContactsRepository::isOwnCorrelation(const QString& correlationId) {
    const std::array prefixes = {kListPrefix,  kAddPrefix,     kUpdatePrefix,   kRemovePrefix,
                                 kAliasPrefix, kProfilePrefix, kDirectoryPrefix};
    return std::ranges::any_of(prefixes, [&](const char* prefix) {
        return correlationId.startsWith(QLatin1String(prefix));
    });
}

void ContactsRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (isOwnCorrelation(correlationId)) {
        // A dead roster page loop must not leak its partial accumulation.
        if (correlationId.startsWith(QLatin1String(kListPrefix))) {
            m_contactLoops.remove(correlationId.mid(int(qstrlen(kListPrefix))));
        }
        emit operationFailed(message);
    }
}

// --- AgentRepository (foreign engines; wire v29) ----------------------------------------------

AgentRepository::AgentRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent)
    : RepositoryBase(client, cache, parent) {
    if (this->client() != nullptr) {
        connect(this->client(), &NodeApiClient::responseReady, this,
                &AgentRepository::handleResponse);
        connect(this->client(), &NodeApiClient::failed, this, &AgentRepository::handleFailure);
    }
}

QVariantList AgentRepository::agents() const {
    QVariantList rows;
    for (const DecodedAgentEntry& e : m_entries) {
        QVariantMap row;
        row[QStringLiteral("name")] = e.name;
        row[QStringLiteral("source")] = e.source;
        row[QStringLiteral("protocol")] = e.protocol;
        row[QStringLiteral("installed")] = e.installed;
        row[QStringLiteral("version")] = e.version;
        rows.append(row);
    }
    return rows;
}

void AgentRepository::refreshCatalog() {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeAgentCatalogRequest(),
                          QLatin1String(kCatalogCorrelation));
}

void AgentRepository::discover() {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    // The discovery reply reuses the AgentCatalog response shape (fresh scan merged with the
    // durable catalog), so it routes through the same decode + catalogRefreshed path.
    client()->sendRequest(NodeApiCodec::encodeAgentDiscoverRequest(),
                          QLatin1String(kDiscoverCorrelation));
}

void AgentRepository::registerAgent(const QVariantMap& form) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    DecodedAgentRecipeInput in;
    in.name = form.value(QStringLiteral("name")).toString().trimmed();
    in.protocol = form.value(QStringLiteral("protocol"), QStringLiteral("Acp")).toString();
    in.program = form.value(QStringLiteral("program")).toString().trimmed();
    in.args = form.value(QStringLiteral("args")).toStringList();
    in.env = form.value(QStringLiteral("env")).toMap();
    in.endpoint = form.value(QStringLiteral("endpoint")).toString().trimmed();
    // Defensive: the register form already gates on this; the node also validates + re-probes.
    if (in.name.isEmpty() || (in.program.isEmpty() && in.endpoint.isEmpty())) {
        emit operationFailed(
            QStringLiteral("A foreign agent needs a name and a program or endpoint"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeAgentRegisterRequest(in),
                          QLatin1String(kRegisterCorrelation));
}

void AgentRepository::removeAgent(const QString& name) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    m_lastRemovedName = name;
    client()->sendRequest(NodeApiCodec::encodeAgentRemoveRequest(name),
                          QLatin1String(kRemoveCorrelation));
}

void AgentRepository::handleResponse(const QString& correlationId, const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kCatalogCorrelation) ||
        correlationId == QLatin1String(kDiscoverCorrelation)) {
        if (!NodeApiCodec::decodeAgentCatalog(responseCbor, &m_entries)) {
            emit operationFailed(QStringLiteral("Failed to decode AgentCatalog response"));
            return;
        }
        emit catalogRefreshed();
        return;
    }
    if (correlationId == QLatin1String(kRegisterCorrelation) ||
        correlationId == QLatin1String(kRemoveCorrelation)) {
        // Register/remove reply is a bare Ok/Error. On Error surface the node's message (it names
        // the agent + the reason — e.g. handshake failed / binary not found) for C6.
        if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Error) {
            DecodedApiError err;
            NodeApiCodec::decodeError(responseCbor, &err);
            emit operationFailed(err.message);
            return;
        }
        if (correlationId == QLatin1String(kRegisterCorrelation)) {
            emit agentRegistered();
        } else {
            emit agentRemoved(m_lastRemovedName);
        }
        // Reflect the node's re-probe: refresh the durable catalog + a fresh discovery scan.
        refreshCatalog();
        discover();
    }
}

void AgentRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kCatalogCorrelation) ||
        correlationId == QLatin1String(kDiscoverCorrelation) ||
        correlationId == QLatin1String(kRegisterCorrelation) ||
        correlationId == QLatin1String(kRemoveCorrelation)) {
        emit operationFailed(message);
    }
}

// --- Interactive auth (app-wizard-auth stream) ---------------------------------------------------

AuthRepository::AuthRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent)
    : RepositoryBase(client, cache, parent) {
    if (this->client() != nullptr) {
        connect(this->client(), &NodeApiClient::responseReady, this,
                &AuthRepository::handleResponse);
        connect(this->client(), &NodeApiClient::failed, this, &AuthRepository::handleFailure);
    }
}

void AuthRepository::refreshProviders() {
    if (client() == nullptr) {
        emit failed(QStringLiteral("providers"), QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeAuthProvidersRequest(),
                          QLatin1String(kProvidersCorrelation));
}

void AuthRepository::begin(const QString& family, const QVariantMap& params,
                           const QString& redirectUri, const QString& bindProfile) {
    if (client() == nullptr) {
        emit failed(QStringLiteral("begin"), QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(
        NodeApiCodec::encodeAuthBeginRequest(family, params, redirectUri, bindProfile),
        QLatin1String(kBeginCorrelation));
}

void AuthRepository::step(const QString& flowId, AuthStepInputKind kind, const QVariantMap& fields,
                          const QString& callback) {
    if (client() == nullptr) {
        emit failed(QStringLiteral("step"), QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeAuthStepRequest(flowId, kind, fields, callback),
                          QLatin1String(kStepCorrelation));
}

void AuthRepository::cancel(const QString& flowId) {
    if (client() == nullptr || flowId.isEmpty()) {
        return; // nothing to drop; cancel is best-effort by design
    }
    client()->sendRequest(NodeApiCodec::encodeAuthCancelRequest(flowId),
                          QLatin1String(kCancelCorrelation));
}

void AuthRepository::handleResponse(const QString& correlationId, const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kProvidersCorrelation)) {
        if (!NodeApiCodec::decodeAuthProviders(responseCbor, &m_providers)) {
            emitPhaseError(QStringLiteral("providers"), responseCbor,
                           tr("Failed to read the sign-in provider list"));
            return;
        }
        emit providersRefreshed();
        return;
    }
    if (correlationId == QLatin1String(kBeginCorrelation)) {
        DecodedAuthBeginResponse response;
        if (!NodeApiCodec::decodeAuthBegun(responseCbor, &response)) {
            emitPhaseError(QStringLiteral("begin"), responseCbor,
                           tr("The sign-in flow could not be started"));
            return;
        }
        emit begun(response);
        return;
    }
    if (correlationId == QLatin1String(kStepCorrelation)) {
        DecodedAuthStepResult result;
        if (!NodeApiCodec::decodeAuthStepped(responseCbor, &result)) {
            emitPhaseError(QStringLiteral("step"), responseCbor,
                           tr("The sign-in could not be completed"));
            return;
        }
        emit stepped(result);
        return;
    }
    // kCancelCorrelation: fire-and-forget (Ok and errors are both terminal for an abandoned
    // flow).
}

void AuthRepository::emitPhaseError(const QString& phase, const QByteArray& responseCbor,
                                    const QString& fallback) {
    DecodedApiError err;
    if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Error &&
        NodeApiCodec::decodeError(responseCbor, &err)) {
        emit failed(phase, err.message);
    } else {
        emit failed(phase, fallback);
    }
}

void AuthRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kProvidersCorrelation)) {
        emit failed(QStringLiteral("providers"), message);
    } else if (correlationId == QLatin1String(kBeginCorrelation)) {
        emit failed(QStringLiteral("begin"), message);
    } else if (correlationId == QLatin1String(kStepCorrelation)) {
        emit failed(QStringLiteral("step"), message);
    }
    // kCancelCorrelation: silent (best-effort).
}

// [wave2:app-approvals-safety] --- ToolRepository (D2 tool inventory) --------------------------

ToolRepository::ToolRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent)
    : RepositoryBase(client, cache, parent) {
    if (this->client() != nullptr) {
        connect(this->client(), &NodeApiClient::responseReady, this,
                &ToolRepository::handleResponse);
        connect(this->client(), &NodeApiClient::failed, this, &ToolRepository::handleFailure);
    }
}

void ToolRepository::refreshTools() {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeToolListRequest(), QLatin1String(kToolsCorrelation));
}

// [waveB:app-v30] D4: request the toggle; on Ok re-fetch so the client renders the node's
// authoritative overlay (never an optimistic local flip).
void ToolRepository::setEnabled(const QString& tool, bool enabled) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeToolSetEnabledRequest(tool, enabled),
                          QLatin1String(kSetEnabledCorrelation));
}

void ToolRepository::handleResponse(const QString& correlationId, const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kToolsCorrelation)) {
        QList<DecodedToolInfo> tools;
        if (!NodeApiCodec::decodeTools(responseCbor, &tools)) {
            emit operationFailed(QStringLiteral("Failed to decode Tools response"));
            return;
        }
        m_tools = tools;
        emit toolsRefreshed();
        return;
    }
    // [waveB:app-v30] D4: the toggle's Ok/Error; on Ok re-list the authoritative inventory.
    if (correlationId == QLatin1String(kSetEnabledCorrelation)) {
        const ApiResponseKind kind = NodeApiCodec::responseKind(responseCbor);
        if (kind == ApiResponseKind::Ok) {
            refreshTools();
            return;
        }
        DecodedApiError err;
        if (kind == ApiResponseKind::Error && NodeApiCodec::decodeError(responseCbor, &err)) {
            emit operationFailed(err.message);
        } else {
            emit operationFailed(tr("Failed to update the tool"));
        }
    }
}

void ToolRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kToolsCorrelation) ||
        correlationId == QLatin1String(kSetEnabledCorrelation)) {
        emit operationFailed(message);
    }
}

// [wave2:app-approvals-safety] --- FingerprintRepository (D4 remembered approvals) -------------

FingerprintRepository::FingerprintRepository(NodeApiClient* client, DaemonCacheStore* cache,
                                             QObject* parent)
    : RepositoryBase(client, cache, parent) {
    if (this->client() != nullptr) {
        connect(this->client(), &NodeApiClient::responseReady, this,
                &FingerprintRepository::handleResponse);
        connect(this->client(), &NodeApiClient::failed, this,
                &FingerprintRepository::handleFailure);
    }
}

void FingerprintRepository::refreshFingerprints(const QString& sessionId) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    m_session = sessionId;
    if (sessionId.isEmpty()) {
        // No active session to scope to: present an empty allow-list rather than a wire error.
        m_fingerprints.clear();
        emit fingerprintsRefreshed();
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeFingerprintListRequest(sessionId),
                          QLatin1String(kListCorrelation));
}

void FingerprintRepository::revoke(const QString& sessionId, const QString& fingerprint) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    m_session = sessionId;
    client()->sendRequest(NodeApiCodec::encodeFingerprintRevokeRequest(sessionId, fingerprint),
                          QLatin1String(kRevokeCorrelation));
}

void FingerprintRepository::handleResponse(const QString& correlationId,
                                           const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kListCorrelation)) {
        QList<DecodedRememberedFingerprint> fps;
        if (!NodeApiCodec::decodeFingerprints(responseCbor, &fps)) {
            emit operationFailed(QStringLiteral("Failed to decode Fingerprints response"));
            return;
        }
        m_fingerprints = fps;
        emit fingerprintsRefreshed();
        return;
    }
    if (correlationId == QLatin1String(kRevokeCorrelation)) {
        const ApiResponseKind kind = NodeApiCodec::responseKind(responseCbor);
        if (kind == ApiResponseKind::Ok) {
            refreshFingerprints(m_session); // re-sync the allow-list after a revoke
            return;
        }
        DecodedApiError err;
        if (kind == ApiResponseKind::Error && NodeApiCodec::decodeError(responseCbor, &err)) {
            emit operationFailed(err.message);
        } else {
            emit operationFailed(tr("Fingerprint revoke failed"));
        }
        return;
    }
}

void FingerprintRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kListCorrelation) ||
        correlationId == QLatin1String(kRevokeCorrelation)) {
        emit operationFailed(message);
    }
}

// --- Node OpenAI-compatible gateway (Phase F; wire v32) --------------------------------------
GatewayRepository::GatewayRepository(NodeApiClient* client, QObject* parent)
    : RepositoryBase(client, nullptr, parent) {
    if (this->client() != nullptr) {
        connect(this->client(), &NodeApiClient::responseReady, this,
                &GatewayRepository::handleResponse);
        connect(this->client(), &NodeApiClient::failed, this, &GatewayRepository::handleFailure);
    }
}

void GatewayRepository::refresh() {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeGatewayGetRequest(), QLatin1String(kGetCorrelation));
}

void GatewayRepository::setEnabled(bool enabled, const QString& addr) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeGatewaySetRequest(enabled, addr),
                          QLatin1String(kSetCorrelation));
}

void GatewayRepository::applyStatusReply(const QByteArray& responseCbor, bool isGet) {
    if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Error) {
        DecodedApiError err;
        NodeApiCodec::decodeError(responseCbor, &err);
        // An Unsupported GatewayGet means the peer predates the gateway: mark it so the surfaces
        // hide/disable, rather than surfacing a spurious failure. A GatewaySet error (or any
        // non-Unsupported GatewayGet error) is a real operation failure.
        if (isGet && err.kind == QStringLiteral("Unsupported")) {
            if (m_supported || !m_loaded) {
                m_supported = false;
                m_loaded = true;
                emit statusChanged();
            }
            return;
        }
        emit operationFailed(err.message.isEmpty() ? tr("Gateway operation failed") : err.message);
        return;
    }
    if (!NodeApiCodec::decodeGatewayStatus(responseCbor, &m_status)) {
        emit operationFailed(tr("Failed to decode gateway status"));
        return;
    }
    m_supported = true;
    m_loaded = true;
    emit statusChanged();
}

void GatewayRepository::handleResponse(const QString& correlationId,
                                       const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kGetCorrelation)) {
        applyStatusReply(responseCbor, /*isGet=*/true);
    } else if (correlationId == QLatin1String(kSetCorrelation)) {
        applyStatusReply(responseCbor, /*isGet=*/false);
    }
}

void GatewayRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kGetCorrelation) ||
        correlationId == QLatin1String(kSetCorrelation)) {
        emit operationFailed(message);
    }
}

// --- [integrations wire v38] Transport account settings (read + configure) ----------------------
void TransportRepository::refreshSettings(const QString& transport) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeTransportSettingsRequest(transport),
                          QLatin1String(kSettingsPrefix) + transport);
}

void TransportRepository::configure(const QString& transport,
                                    const QMap<QString, QString>& values) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeTransportConfigureRequest(transport, values),
                          QLatin1String(kConfigurePrefix) + transport);
}

void TransportRepository::handleSettingsResponse(const QString& transport,
                                                 const QByteArray& responseCbor) {
    QMap<QString, QString> vals;
    if (!NodeApiCodec::decodeTransportSettings(responseCbor, &vals)) {
        DecodedApiError err;
        if (NodeApiCodec::decodeError(responseCbor, &err)) {
            emit operationFailed(err.message);
        } else {
            emit operationFailed(QStringLiteral("Failed to decode TransportSettings response"));
        }
        return;
    }
    m_settings.insert(transport, vals);
    emit settingsRefreshed(transport);
}

void TransportRepository::handleConfigureResponse(const QString& transport,
                                                  const QByteArray& responseCbor) {
    const ApiResponseKind kind = NodeApiCodec::responseKind(responseCbor);
    if (kind == ApiResponseKind::Ok) {
        // Authoritative re-read: the node validated + applied (reconnect); render its stored
        // values.
        refreshSettings(transport);
        return;
    }
    DecodedApiError err;
    if (kind == ApiResponseKind::Error && NodeApiCodec::decodeError(responseCbor, &err)) {
        emit operationFailed(err.message);
    } else {
        emit operationFailed(tr("Failed to apply the account settings"));
    }
}

// --- [integrations wire v38] Native chat (ConvHistory / ConvSend) -------------------------------
QString ChatRepository::convKey(const QString& transport, const QString& conv) {
    return transport + QChar(0x1f) + conv;
}

bool ChatRepository::splitConvKey(const QString& tail, QString* transport, QString* conv) {
    const qsizetype us = tail.indexOf(QChar(0x1f));
    if (us < 0) {
        return false;
    }
    if (transport != nullptr) {
        *transport = tail.left(us);
    }
    if (conv != nullptr) {
        *conv = tail.mid(us + 1);
    }
    return true;
}

ChatRepository::ChatRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent)
    : RepositoryBase(client, cache, parent) {
    if (this->client() != nullptr) {
        connect(this->client(), &NodeApiClient::responseReady, this,
                &ChatRepository::handleResponse);
        connect(this->client(), &NodeApiClient::failed, this, &ChatRepository::handleFailure);
    }
}

void ChatRepository::refreshHistory(const QString& transport, const QString& conv) {
    if (client() == nullptr) {
        emit operationFailed(transport, conv, QStringLiteral("No NodeApi client configured"));
        return;
    }
    const QString key = convKey(transport, conv);
    // A fresh refresh restarts the conversation's page loop (paged forward from the start).
    m_historyLoops[key].reset();
    client()->sendRequest(NodeApiCodec::encodeConvHistoryRequest(transport, conv),
                          QLatin1String(kHistoryPrefix) + key);
}

void ChatRepository::send(const QString& transport, const QString& conv, const QString& text) {
    if (client() == nullptr) {
        emit operationFailed(transport, conv, QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeConvSendRequest(transport, conv, text),
                          QLatin1String(kSendPrefix) + convKey(transport, conv));
}

void ChatRepository::applyMessagesChanged(const QString& transport, const QString& conv) {
    // The transcript grew: re-fetch it (the node bounds the page). A later delta could page only
    // the tail past the last known cursor; a full refresh keeps v38 simple and always correct.
    refreshHistory(transport, conv);
}

void ChatRepository::handleResponse(const QString& correlationId, const QByteArray& responseCbor) {
    const auto tailOf = [&](const char* prefix) { return correlationId.mid(int(qstrlen(prefix))); };
    if (correlationId.startsWith(QLatin1String(kHistoryPrefix))) {
        QString transport;
        QString conv;
        if (splitConvKey(tailOf(kHistoryPrefix), &transport, &conv)) {
            handleHistoryResponse(transport, conv, responseCbor);
        }
    } else if (correlationId.startsWith(QLatin1String(kSendPrefix))) {
        QString transport;
        QString conv;
        if (splitConvKey(tailOf(kSendPrefix), &transport, &conv)) {
            handleSendResponse(transport, conv, responseCbor);
        }
    }
}

void ChatRepository::handleFailure(const QString& correlationId, const QString& message) {
    const auto tailOf = [&](const char* prefix) { return correlationId.mid(int(qstrlen(prefix))); };
    QString transport;
    QString conv;
    bool ours = false;
    if (correlationId.startsWith(QLatin1String(kHistoryPrefix))) {
        ours = splitConvKey(tailOf(kHistoryPrefix), &transport, &conv);
    } else if (correlationId.startsWith(QLatin1String(kSendPrefix))) {
        ours = splitConvKey(tailOf(kSendPrefix), &transport, &conv);
    }
    if (ours) {
        emit operationFailed(transport, conv, message);
    }
}

void ChatRepository::handleHistoryResponse(const QString& transport, const QString& conv,
                                           const QByteArray& responseCbor) {
    const QString key = convKey(transport, conv);
    QList<DecodedChatMessage> page;
    quint64 next = 0;
    quint64 head = 0;
    if (!NodeApiCodec::decodeConvHistory(responseCbor, &page, &next, &head)) {
        m_historyLoops.remove(key);
        DecodedApiError err;
        if (NodeApiCodec::decodeError(responseCbor, &err)) {
            emit operationFailed(transport, conv, err.message);
        } else {
            emit operationFailed(transport, conv,
                                 QStringLiteral("Failed to decode ConvHistory response"));
        }
        return;
    }
    // Page loop: accumulate ascending records and re-issue with `after_cursor = next` until the
    // page reaches the head (next >= head), then swap the transcript ONCE.
    PageLoop<DecodedChatMessage>& loop = m_historyLoops[key];
    loop.items.append(page);
    if (next < head && loop.guard(QString::number(next)) && client() != nullptr) {
        client()->sendRequest(
            NodeApiCodec::encodeConvHistoryRequest(transport, conv, /*hasAfter=*/true, next),
            QLatin1String(kHistoryPrefix) + key);
        return;
    }
    m_messages.insert(key, loop.items);
    m_historyLoops.remove(key);
    emit historyRefreshed(transport, conv);
}

void ChatRepository::handleSendResponse(const QString& transport, const QString& conv,
                                        const QByteArray& responseCbor) {
    const ApiResponseKind kind = NodeApiCodec::responseKind(responseCbor);
    if (kind == ApiResponseKind::Ok) {
        // No optimistic echo: the node appended the Chat record + emits MessagesChanged, which the
        // SubscriptionManager turns into an authoritative refetch (applyMessagesChanged).
        emit messageSent(transport, conv);
        return;
    }
    DecodedApiError err;
    if (kind == ApiResponseKind::Error && NodeApiCodec::decodeError(responseCbor, &err)) {
        emit operationFailed(transport, conv, err.message);
    } else {
        emit operationFailed(transport, conv, tr("Failed to send the message"));
    }
}

} // namespace daemonapp::daemon
