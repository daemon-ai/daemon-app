// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/repositories.h"

#include "daemon/node_api_codec.h"

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
    // A fresh refresh restarts the page loop: drop anything a superseded loop accumulated.
    m_rosterLoop.reset();
    client()->sendRequest(
        NodeApiCodec::encodeSessionsQueryRequest(m_sentRosterDelta, m_sentRosterSinceRev),
        QLatin1String(kSessionsCorrelation));
}

void SessionRepository::refreshSessionsByProfile(const QString& profileId) {
    if (client() == nullptr) {
        emit refreshFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    if (profileId.isEmpty()) {
        return;
    }
    m_byProfileId = profileId;
    m_byProfileLoop.reset();
    client()->sendRequest(
        NodeApiCodec::encodeSessionsQueryRequest(/*hasSinceRev=*/false, /*sinceRev=*/0, profileId),
        QLatin1String(kByProfileCorrelation));
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

void SessionRepository::updateSessionMeta(const QString& sessionId, std::optional<bool> pinned,
                                          std::optional<bool> archived,
                                          std::optional<QString> title) {
    if (sessionId.isEmpty() || (!pinned && !archived && !title)) {
        return; // nothing to patch
    }
    if (client() == nullptr) {
        // Offline / no client: pin/archive/title are node-owned, so there is no local write to
        // fall back on. Surface it rather than silently swallowing the intent.
        emit metaUpdateFailed(sessionId, tr("Not connected to a daemon"));
        return;
    }
    m_pendingMetaSession = sessionId;
    client()->sendRequest(
        NodeApiCodec::encodeSessionUpdateMetaRequest(sessionId, pinned, archived, std::move(title)),
        QLatin1String(kUpdateMetaCorrelation));
}

void SessionRepository::handleResponse(const QString& correlationId,
                                       const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kSessionsCorrelation)) {
        applySessionPage(responseCbor);
    } else if (correlationId == QLatin1String(kByProfileCorrelation)) {
        applyByProfilePage(responseCbor);
    } else if (correlationId == QLatin1String(kCreateCorrelation)) {
        applySessionCreated(responseCbor);
    } else if (correlationId == QLatin1String(kUpdateMetaCorrelation)) {
        applyMetaUpdate(responseCbor);
    }
}

void SessionRepository::applyMetaUpdate(const QByteArray& responseCbor) {
    const QString sessionId = m_pendingMetaSession;
    m_pendingMetaSession.clear();
    if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Ok) {
        // Node-authoritative: the row is now updated node-side (and
        // RosterChanged/SessionMetaChanged will fire). Issue the immediate, non-debounced refetch
        // so the roster projects the node's stored pin/archive/title without waiting on the feed
        // debounce - race-free because the node persisted + bumped the roster rev before replying
        // Ok (mirrors applySessionCreated).
        refreshSessions();
        return;
    }
    DecodedApiError err;
    const QString msg = NodeApiCodec::decodeError(responseCbor, &err)
                            ? err.message
                            : tr("SessionUpdateMeta failed");
    emit metaUpdateFailed(sessionId, msg);
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
    // Upsert a minimal cached row NOW so the All Sessions list / Fleet leaves show the new
    // session immediately instead of waiting on the debounced RosterChanged refetch (whose
    // full-replace form would otherwise be the first time the row exists client-side). The
    // authoritative row (node-bound default profile, state, role) lands via the immediate
    // refetch below; the node's title is seeded later by the first message.
    CachedSessionRow row;
    row.sessionId = sessionId;
    // A blank node-created session is un-run: "Ready" is its wire state (and the schema's
    // daemon_sessions.state is NOT NULL, so the placeholder must carry one).
    row.state = QStringLiteral("Ready");
    row.profileRef = profileId; // empty when the node bound its active default
    row.updatedAtMs = QDateTime::currentMSecsSinceEpoch();
    upsertCachedSession(row);
    emit sessionsRefreshed(); // drives CachedSessionStore::reload() -> changed() -> UI rebuild
    // The node also emits RosterChanged (subscription_manager refetches roster+tree); this signal
    // is the event-driven hook the orchestrator/sidebar auto-select on.
    emit sessionCreated(sessionId, profileId);
    // Immediate (non-debounced) authoritative refetch. Request ordering makes this race-free: the
    // node registered the session and bumped the roster rev before replying SessionCreated, so
    // this SessionsQuery always includes the new row (a full replace cannot prune it).
    refreshSessions();
}

void SessionRepository::applyByProfilePage(const QByteArray& responseCbor) {
    QList<CachedSessionRow> rows;
    QString nextCursor;
    if (!NodeApiCodec::decodeSessionPage(responseCbor, &rows, &nextCursor, nullptr, nullptr)) {
        emit refreshFailed(QStringLiteral("Failed to decode ByProfile SessionPage response"));
        return;
    }
    // Additive merge (no prune): a scoped subset must not clobber the shared roster cache. The
    // rows carry bound_profile, so the client-side ByProfile filter (CachedSessionStore) projects
    // them under their agent. Because it never prunes, each page can merge incrementally; the
    // page loop below just keeps fetching until next_cursor clears (guard-only PageLoop use).
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (CachedSessionRow& row : rows) {
        row.updatedAtMs = now;
        upsertCachedSession(row);
    }
    if (!nextCursor.isEmpty() && !m_byProfileId.isEmpty() && m_byProfileLoop.guard(nextCursor) &&
        client() != nullptr) {
        client()->sendRequest(NodeApiCodec::encodeSessionsQueryRequest(
                                  /*hasSinceRev=*/false, /*sinceRev=*/0, m_byProfileId, nextCursor),
                              QLatin1String(kByProfileCorrelation));
    } else {
        m_byProfileLoop.reset();
    }
    emit sessionsRefreshed();
}

void SessionRepository::applySessionPage(const QByteArray& responseCbor) {
    QList<CachedSessionRow> rows;
    QString nextCursor;
    quint64 rev = 0;
    QStringList removed;
    if (!NodeApiCodec::decodeSessionPage(responseCbor, &rows, &nextCursor, &rev, &removed)) {
        m_rosterLoop.reset();
        emit refreshFailed(QStringLiteral("Failed to decode SessionPage response"));
        return;
    }
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    // L4: a full page replaces the roster; a delta merges. A delta whose returned rev went
    // *backwards* from what we asked for is a daemon-reset fallback (the server returned a full
    // page because our since_rev was unservable) -> treat it as a replace too.
    const bool fallbackFull = m_sentRosterDelta && rev < m_sentRosterSinceRev;
    const bool replace = !m_sentRosterDelta || fallbackFull;
    if (replace && !nextCursor.isEmpty() && m_rosterLoop.guard(nextCursor) && client() != nullptr) {
        // Page loop (full reads only): accumulate and query the next page; the replace + prune
        // below must see the WHOLE roster, or every session past page one would be dropped.
        // The continuation of a full read is itself a full read (no since_rev — a delta fallback
        // has already resolved to full pages). Delta reads stay single-page: their persisted rev
        // catches the tail up on the next refresh.
        m_rosterLoop.items.append(rows);
        m_sentRosterDelta = false;
        client()->sendRequest(
            NodeApiCodec::encodeSessionsQueryRequest(false, 0, QString(), nextCursor),
            QLatin1String(kSessionsCorrelation));
        return;
    }
    if (replace) {
        // The final page: run the merge over everything the loop accumulated.
        rows = m_rosterLoop.items + rows;
    }
    m_rosterLoop.reset();
    if (replace) {
        // Prune any cached session the full listing no longer contains (closes the "removed
        // sessions never purged" gap on a cold/fallback resync).
        pruneSessionsMissingFrom(rows);
    }
    for (CachedSessionRow& row : rows) {
        row.updatedAtMs = now;
        upsertCachedSession(row);
    }
    if (!replace) {
        // L4 delta prune: drop sessions the server reported removed (hard-removed or left scope).
        pruneRemovedSessions(removed);
    }
    if (cache() != nullptr) {
        // Persist the roster revision so the next refresh resumes as a delta.
        cache()->setCursor(QLatin1String(kRosterRevScope), QString::number(rev), now);
    }
    emit sessionsRefreshed();
}

void SessionRepository::pruneSessionsMissingFrom(const QList<CachedSessionRow>& rows) {
    if (cache() == nullptr) {
        return;
    }
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

void SessionRepository::pruneRemovedSessions(const QList<QString>& removed) {
    if (cache() == nullptr) {
        return;
    }
    for (const QString& id : removed) {
        cache()->deleteSession(id);
    }
}

void SessionRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kUpdateMetaCorrelation)) {
        // A transport-level meta-update failure (timeout / dropped socket): surface it so the
        // pin/archive/rename is not silently mistaken for applied.
        const QString sessionId = m_pendingMetaSession;
        m_pendingMetaSession.clear();
        emit metaUpdateFailed(sessionId, message);
        return;
    }
    if (correlationId == QLatin1String(kCreateCorrelation)) {
        // A transport-level create failure (timeout / dropped socket): the pending create is
        // dead — clear its state and surface the error instead of vanishing silently.
        m_pendingCreateProfile.clear();
        emit refreshFailed(message);
        return;
    }
    if (correlationId == QLatin1String(kSessionsCorrelation) ||
        correlationId == QLatin1String(kByProfileCorrelation)) {
        // A dead page loop must not leak its partial accumulation into the next refresh.
        m_rosterLoop.reset();
        m_byProfileLoop.reset();
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
            emit operationFailed(tr("Credential operation failed"));
        }
        return;
    }
}

bool CredentialRepository::isOwnCorrelation(const QString& correlationId) {
    const std::array keys = {kListCorrelation, kSetCorrelation, kRemoveCorrelation};
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
        correlationId.startsWith(QLatin1String(kModelsPrefix))) {
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
    static const std::array<PrefixRoute, 4> kRoutes = {{
        {kGetPrefix, &ProfileRepository::handleProfileGetResponse},
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
                                bool allowPermanent) {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(
        NodeApiCodec::encodeApprovalDecideRequest(sessionId, requestId, allow, allowPermanent),
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

QList<CachedFleetUnitRow> FleetRepository::cachedUnits() const {
    return cache() != nullptr ? cache()->fleetUnits() : QList<CachedFleetUnitRow>{};
}

void FleetRepository::refreshTree(const QString& source) {
    if (client() == nullptr) {
        emit refreshFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    // A fresh refresh restarts the page loop: drop anything a superseded loop accumulated.
    m_treeLoop.reset();
    m_treeRoot.clear();
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
        handleTreeResponse(responseCbor);
        return;
    }
    if (correlationId == QLatin1String(kControlCorrelation)) {
        handleUnitControlResponse(responseCbor);
    }
}

void FleetRepository::handleTreeResponse(const QByteArray& responseCbor) {
    QList<DecodedUnitNode> nodes;
    QString root;
    QString next;
    if (!NodeApiCodec::decodeTreeReport(responseCbor, &nodes, &root, &next)) {
        m_treeLoop.reset();
        m_treeRoot.clear();
        emit refreshFailed(QStringLiteral("Failed to decode Tree response"));
        return;
    }
    // Page loop: accumulate the raw nodes and re-issue with the cursor until it clears (`root`
    // rides every page; the first page fixes it), then flatten + sync ONCE over the whole union —
    // the id-linked DFS needs every node, and pruning against one page would drop the rest.
    if (m_treeLoop.pages == 0) {
        m_treeRoot = root;
    }
    m_treeLoop.items.append(nodes);
    if (!next.isEmpty() && m_treeLoop.guard(next) && client() != nullptr) {
        client()->sendRequest(NodeApiCodec::encodeTreeRequest(next),
                              QLatin1String(kTreeCorrelation));
        return;
    }
    const QList<DecodedUnitNode> flat =
        NodeApiCodec::flattenTreeNodes(m_treeLoop.items, m_treeRoot);
    m_treeLoop.reset();
    m_treeRoot.clear();
    syncFleetUnits(flat);
    emit treeRefreshed();
}

void FleetRepository::syncFleetUnits(const QList<DecodedUnitNode>& flat) {
    if (cache() == nullptr) {
        return;
    }
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

void FleetRepository::handleUnitControlResponse(const QByteArray& responseCbor) {
    // Pause/Resume/Scale answer Ok (re-fetch the tree) or an ApiError (e.g. Unsupported on an
    // engine-leaf unit -> surface it, do not silently swallow).
    if (NodeApiCodec::responseKind(responseCbor) == ApiResponseKind::Error) {
        DecodedApiError err;
        NodeApiCodec::decodeError(responseCbor, &err);
        emit controlFailed(err.message.isEmpty() ? tr("Unit control failed") : err.message);
    } else {
        refreshTree(QStringLiteral("control"));
    }
}

void FleetRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kTreeCorrelation)) {
        // A dead page loop must not leak its partial accumulation into the next refresh.
        m_treeLoop.reset();
        m_treeRoot.clear();
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
    // A fresh refresh restarts the transport's page loop.
    m_convLoops[transport].reset();
    client()->sendRequest(NodeApiCodec::encodeConvListRequest(transport),
                          QLatin1String(kConvPrefix) + transport);
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
    if (correlationId.startsWith(QLatin1String(kConvPrefix))) {
        handleConversationsResponse(correlationId.mid(int(qstrlen(kConvPrefix))), responseCbor);
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

bool TransportRepository::isOwnCorrelation(const QString& correlationId) {
    const std::array keys = {kAdaptersCorrelation, kInstancesCorrelation};
    return std::ranges::any_of(
               keys, [&](const char* key) { return correlationId == QLatin1String(key); }) ||
           correlationId.startsWith(QLatin1String(kConvPrefix));
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

// --- AcpRepository (foreign engines; wire v23) ------------------------------------------------

AcpRepository::AcpRepository(NodeApiClient* client, DaemonCacheStore* cache, QObject* parent)
    : RepositoryBase(client, cache, parent) {
    if (this->client() != nullptr) {
        connect(this->client(), &NodeApiClient::responseReady, this,
                &AcpRepository::handleResponse);
        connect(this->client(), &NodeApiClient::failed, this, &AcpRepository::handleFailure);
    }
}

QVariantList AcpRepository::agents() const {
    QVariantList rows;
    for (const DecodedAcpAgentEntry& e : m_entries) {
        QVariantMap row;
        row[QStringLiteral("name")] = e.name;
        row[QStringLiteral("source")] = e.source;
        row[QStringLiteral("installed")] = e.installed;
        row[QStringLiteral("version")] = e.version;
        rows.append(row);
    }
    return rows;
}

void AcpRepository::refreshCatalog() {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeAcpCatalogRequest(),
                          QLatin1String(kCatalogCorrelation));
}

void AcpRepository::discover() {
    if (client() == nullptr) {
        emit operationFailed(QStringLiteral("No NodeApi client configured"));
        return;
    }
    // The discovery reply reuses the AcpCatalog response shape (fresh scan merged with the
    // durable catalog), so it routes through the same decode + catalogRefreshed path.
    client()->sendRequest(NodeApiCodec::encodeAcpDiscoverRequest(),
                          QLatin1String(kDiscoverCorrelation));
}

void AcpRepository::handleResponse(const QString& correlationId, const QByteArray& responseCbor) {
    if (correlationId != QLatin1String(kCatalogCorrelation) &&
        correlationId != QLatin1String(kDiscoverCorrelation)) {
        return;
    }
    if (!NodeApiCodec::decodeAcpCatalog(responseCbor, &m_entries)) {
        emit operationFailed(QStringLiteral("Failed to decode AcpCatalog response"));
        return;
    }
    emit catalogRefreshed();
}

void AcpRepository::handleFailure(const QString& correlationId, const QString& message) {
    if (correlationId == QLatin1String(kCatalogCorrelation) ||
        correlationId == QLatin1String(kDiscoverCorrelation)) {
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

void AuthRepository::complete(const QString& flowId, const QString& callback) {
    if (client() == nullptr) {
        emit failed(QStringLiteral("complete"), QStringLiteral("No NodeApi client configured"));
        return;
    }
    client()->sendRequest(NodeApiCodec::encodeAuthCompleteRequest(flowId, callback),
                          QLatin1String(kCompleteCorrelation));
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
    if (correlationId == QLatin1String(kCompleteCorrelation)) {
        DecodedAuthCompleteResponse response;
        if (!NodeApiCodec::decodeAuthCompleted(responseCbor, &response)) {
            emitPhaseError(QStringLiteral("complete"), responseCbor,
                           tr("The sign-in could not be completed"));
            return;
        }
        emit completed(response);
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
    } else if (correlationId == QLatin1String(kCompleteCorrelation)) {
        emit failed(QStringLiteral("complete"), message);
    }
    // kCancelCorrelation: silent (best-effort).
}

} // namespace daemonapp::daemon
