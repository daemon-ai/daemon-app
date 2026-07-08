// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_model_catalog.h"

#include "daemon/repositories.h"
#include "models/model_format.h"
#include "uimodels/variant_list_model.h"

#include <algorithm>
#include <QHash>
#include <QSet>

namespace models {

using daemonapp::daemon::DecodedDownloadStatus;
using daemonapp::daemon::DecodedGgufInfo;
using daemonapp::daemon::DecodedInstalledModel;
using daemonapp::daemon::DecodedModelDescriptor;
using daemonapp::daemon::DecodedModelFile;
using daemonapp::daemon::DecodedQuantizeStatus;
using daemonapp::daemon::DecodedSearchHit;
using daemonapp::daemon::ModelRepository;

namespace {
// Normalize a wire DownloadState to the lowercase tokens the Downloads view + mock both use.
QString downloadStateToken(const QString& wire) {
    if (wire == QStringLiteral("Completed")) {
        return QStringLiteral("done");
    }
    if (wire == QStringLiteral("Paused")) {
        return QStringLiteral("paused");
    }
    if (wire == QStringLiteral("Failed")) {
        return QStringLiteral("failed");
    }
    if (wire == QStringLiteral("Cancelled")) {
        return QStringLiteral("cancelled");
    }
    if (wire == QStringLiteral("Queued")) {
        return QStringLiteral("queued");
    }
    return QStringLiteral("downloading");
}

// Whether a normalized state token names a settled job (dismissible; no longer progressing).
bool isTerminalStateToken(const QString& token) {
    return token == QStringLiteral("done") || token == QStringLiteral("failed") ||
           token == QStringLiteral("cancelled");
}

// Whether a filename carries the vision-projector (mmproj) marker ("mm-proj" normalizes).
bool isMmprojName(const QString& name) {
    const QString norm =
        name.toLower().replace(QStringLiteral("mm-proj"), QStringLiteral("mmproj"));
    return norm.contains(QStringLiteral("mmproj")) || norm.contains(QStringLiteral("projector"));
}

// Whether an installed record is a vision-projector artifact: the authoritative `arch == clip`
// GGUF metadata, or the mmproj filename hint on the repo file / on-disk name. Mirrors the node's
// classification so the UI badges exactly what the node refuses to activate.
bool isProjectorRecord(const DecodedInstalledModel& m) {
    if (m.arch.compare(QStringLiteral("clip"), Qt::CaseInsensitive) == 0) {
        return true;
    }
    const QString localName = m.localPath.section(QLatin1Char('/'), -1);
    return isMmprojName(m.file) || isMmprojName(localName);
}
} // namespace

DaemonModelCatalog::DaemonModelCatalog(ModelRepository* models,
                                       daemonapp::daemon::ProviderRepository* providers,
                                       daemonapp::daemon::CredentialRepository* credentials,
                                       daemonapp::daemon::ProfileRepository* profiles,
                                       daemonapp::daemon::SessionRepository* sessions,
                                       QObject* parent)
    : IModelCatalog(parent), m_models(models), m_providers(providers), m_credentials(credentials),
      m_profiles(profiles), m_sessions(sessions), m_discover(new uimodels::VariantListModel(this)),
      m_files(new uimodels::VariantListModel(this)),
      m_downloads(new uimodels::VariantListModel(this)),
      m_installed(new uimodels::VariantListModel(this)),
      m_quantizeJobs(new uimodels::VariantListModel(this)) {
    if (m_models != nullptr) {
        connect(m_models, &ModelRepository::modelsRefreshed, this,
                &DaemonModelCatalog::rebuildInstalled);
        connect(m_models, &ModelRepository::catalogChanged, this,
                &DaemonModelCatalog::rebuildInstalled);
        connect(m_models, &ModelRepository::currentRefreshed, this, [this] {
            rebuildInstalled();
            emit currentChanged();
            // A6: the ACTIVE local model is the one whose silent absence strands the user —
            // probe it whenever the current selection resolves (connect-ready / activation),
            // so a deleted artifact surfaces as an explicit missing state instead of a
            // first-send failure. Bounded: one ModelInspect per current-refresh, local only.
            const QString current = currentModelId();
            if (!current.isEmpty() && isLocalInstalled(current)) {
                m_models->inspect(current);
            }
        });
        connect(m_models, &ModelRepository::modelActivated, this, [this] {
            // The node applied the activation; re-resolve current + catalog so the marker moves.
            m_models->refreshCurrent();
            m_models->refreshCatalog();
            emit currentChanged();
        });
        connect(m_models, &ModelRepository::searchHitsChanged, this, [this] {
            rebuildDiscover();
            emit searchHasMoreChanged();
        });
        connect(m_models, &ModelRepository::filesLoaded, this, [this](const QString& repo) {
            rebuildFiles();
            emit filesChanged(repo);
        });
        connect(m_models, &ModelRepository::recommendLoaded, this, [this](const QString& repo) {
            rebuildFiles(); // re-mark the recommended row
            emit recommendChanged(repo);
        });
        connect(m_models, &ModelRepository::downloadsChanged, this,
                &DaemonModelCatalog::rebuildDownloads);
        // Bridge the job-accepted signal so QML (quant picker / discover dialog) reacts without
        // reaching into the repository layer.
        connect(m_models, &ModelRepository::downloadStarted, this,
                [this](quint64 id) { emit downloadStarted(QString::number(id)); });
        // Surface model-op failures (HF network errors, gated repos, failed activations...) on the
        // facade so the Models pages + wizard render them instead of failing silently.
        connect(m_models, &ModelRepository::operationFailed, this,
                [this](const QString& message) { setLastError(message); });
        // A5: quantize jobs mirror the downloads plumbing (rows + accepted signal).
        connect(m_models, &ModelRepository::quantizesChanged, this,
                &DaemonModelCatalog::rebuildQuantizes);
        connect(m_models, &ModelRepository::quantizeStarted, this,
                [this](quint64 id) { emit quantizeStarted(QString::number(id)); });
        // A6 ghost probe resolutions: a failed ModelInspect on a cataloged id marks the row
        // `missing` (an explicit state, never a silent failure); a success clears it.
        connect(m_models, &ModelRepository::inspected, this,
                [this](const QString& id, const DecodedGgufInfo& /*info*/) {
                    if (m_missingIds.remove(id)) {
                        rebuildInstalled();
                    }
                    emit modelVerified(id);
                });
        connect(m_models, &ModelRepository::inspectFailed, this,
                [this](const QString& id, const QString& message) {
                    if (!m_missingIds.contains(id)) {
                        m_missingIds.insert(id);
                        rebuildInstalled();
                    }
                    setLastError(tr("Model %1 is missing on disk: %2").arg(id, message));
                    emit modelMissing(id, message);
                });
        // Seed the cloud catalog + installed set + current selection on construction.
        m_models->refreshModels();
        m_models->refreshCatalog();
        m_models->refreshCurrent();
        // And the download-job snapshot: the daemon outlives the GUI, so a job started by a
        // previous app instance must render immediately, not only on the next progress event.
        m_models->refreshDownloads();
    }
    // Foreign-session live model selection (Phase E): re-emit the selector-changed signal whenever
    // a session's detail (re)hydrates, so the composer re-projects sessionBackend()/selector.
    if (m_sessions != nullptr) {
        connect(m_sessions, &daemonapp::daemon::SessionRepository::sessionDetailLoaded, this,
                [this](const QString& id) { emit sessionModelSelectorChanged(id); });
    }
}

QObject* DaemonModelCatalog::discover() const {
    return m_discover;
}
QObject* DaemonModelCatalog::files() const {
    return m_files;
}
QObject* DaemonModelCatalog::downloads() const {
    return m_downloads;
}
QObject* DaemonModelCatalog::installed() const {
    return m_installed;
}

QString DaemonModelCatalog::filesRepo() const {
    return m_models != nullptr ? m_models->filesRepo() : QString();
}

QString DaemonModelCatalog::currentModelId() const {
    if (!m_pickedId.isEmpty()) {
        return m_pickedId;
    }
    return (m_models != nullptr && m_models->hasCurrent()) ? m_models->currentModel().id
                                                           : QString();
}

bool DaemonModelCatalog::isLocalInstalled(const QString& id) const {
    if (m_models == nullptr) {
        return false;
    }
    const auto& list = m_models->installed();
    return std::ranges::any_of(list, [&id](const DecodedInstalledModel& m) { return m.id == id; });
}

QString DaemonModelCatalog::sessionBackend(const QString& sessionId) const {
    daemonapp::daemon::DecodedSessionDetail detail;
    if (m_sessions == nullptr || sessionId.isEmpty() ||
        !m_sessions->cachedDetail(sessionId, &detail) || !detail.hasForeignBackend) {
        return {}; // native / unknown / not yet hydrated: no foreign backend
    }
    return detail.foreignBackend.kind; // "AgentNative" | "NodeProvider"
}

QVariantMap DaemonModelCatalog::sessionModelSelector(const QString& sessionId) const {
    daemonapp::daemon::DecodedSessionDetail detail;
    if (m_sessions == nullptr || sessionId.isEmpty() ||
        !m_sessions->cachedDetail(sessionId, &detail) || !detail.hasModelSelector) {
        return {QVariantMap{{QStringLiteral("hasSelector"), false}}};
    }
    QVariantList choices;
    choices.reserve(detail.modelSelector.choices.size());
    for (const auto& c : detail.modelSelector.choices) {
        choices.append(QVariantMap{
            {QStringLiteral("id"), c.id},
            {QStringLiteral("label"), c.label.isEmpty() ? c.id : c.label},
        });
    }
    return QVariantMap{
        {QStringLiteral("hasSelector"), true},
        {QStringLiteral("current"), detail.modelSelector.current},
        {QStringLiteral("choices"), choices},
    };
}

void DaemonModelCatalog::ensureSessionDetail(const QString& sessionId) {
    // Hydrate once: getSessionDetail dedupes an in-flight fetch, and a cached detail (even the null
    // arm) means no re-issue. Live refresh on change rides SessionMetaChanged
    // (SubscriptionManager).
    if (m_sessions == nullptr || sessionId.isEmpty() ||
        m_sessions->cachedDetail(sessionId, nullptr)) {
        return;
    }
    m_sessions->getSessionDetail(sessionId);
}

void DaemonModelCatalog::setSessionModel(const QString& sessionId, const QString& model) {
    if (m_models == nullptr || sessionId.isEmpty()) {
        return;
    }
    // Foreign-session model set: model only, NEVER a provider (the node rejects `provider` for a
    // foreign session; AgentNative matches over ACP, NodeProvider rebinds the gateway token).
    m_models->setSessionModel(sessionId, model);
}

QStringList DaemonModelCatalog::installedIds() const {
    QStringList out;
    if (m_models != nullptr) {
        for (const DecodedInstalledModel& m : m_models->installed()) {
            out << m.id;
        }
        for (const DecodedModelDescriptor& d : m_models->models()) {
            if (!out.contains(d.id)) {
                out << d.id;
            }
        }
    }
    return out;
}

QVariantList DaemonModelCatalog::providers() const {
    // The node's real ProviderCatalog (never a hardcoded list). `configured` approximates
    // credential presence: a keyless provider is usable as-is; a key-requiring one is "configured"
    // when some profile bound to its wire selector has a stored credential. (Genai vendors share
    // one selector, so a key stored for any genai vendor marks them all — the redacted
    // CredentialList carries no vendor dimension to be more precise with.)
    QVariantList out;
    if (m_providers == nullptr) {
        return out;
    }
    // The wire selectors that hold a credential, resolved profile -> provider via the reflected
    // ProfileList + the redacted CredentialList.
    QSet<QString> credentialedSelectors;
    if (m_credentials != nullptr && m_profiles != nullptr) {
        QHash<QString, QString> profileSelector; // profile id -> provider wire selector
        for (const daemonapp::daemon::DecodedProfileInfo& p : m_profiles->profiles()) {
            profileSelector.insert(p.id, p.provider);
        }
        for (const daemonapp::daemon::DecodedCredentialInfo& c : m_credentials->credentials()) {
            if (c.present && profileSelector.contains(c.profile)) {
                credentialedSelectors.insert(profileSelector.value(c.profile));
            }
        }
    }
    for (const daemonapp::daemon::DecodedProviderDescriptor& d : m_providers->providers()) {
        if (d.id == QStringLiteral("mock") || d.wireSelector == QStringLiteral("mock")) {
            continue; // never offer Mock (matches the provider-picker seam)
        }
        QVariantMap m;
        m[QStringLiteral("id")] = d.id;
        m[QStringLiteral("name")] = d.displayName.isEmpty() ? d.id : d.displayName;
        m[QStringLiteral("kind")] =
            d.kind == QStringLiteral("local") ? QStringLiteral("local") : QStringLiteral("remote");
        m[QStringLiteral("configured")] =
            !d.requiresKey || credentialedSelectors.contains(d.wireSelector);
        out.append(QVariant(m));
    }
    return out;
}

void DaemonModelCatalog::search(const QString& query, const QString& sizeFilter) {
    Q_UNUSED(sizeFilter)
    if (m_models == nullptr) {
        return;
    }
    setLastError({});
    // Local-track discovery is repo search; an empty query still asks for trending repos.
    m_models->search(query);
}

bool DaemonModelCatalog::searchHasMore() const {
    return m_models != nullptr && m_models->searchHasMore();
}

void DaemonModelCatalog::searchMore() {
    if (m_models == nullptr) {
        return;
    }
    setLastError({});
    m_models->searchMore();
}

void DaemonModelCatalog::repoFiles(const QString& repo) {
    if (m_models == nullptr || repo.isEmpty()) {
        return;
    }
    setLastError({});
    m_files->setRows({});
    m_models->requestFiles(repo);
    m_models->recommend(repo); // auto-detect the budget node-side
}

void DaemonModelCatalog::recommend(const QString& repo) {
    if (m_models != nullptr && !repo.isEmpty()) {
        m_models->recommend(repo);
    }
}

QVariantMap DaemonModelCatalog::recommendation() const {
    QVariantMap out;
    if (m_models == nullptr || !m_models->hasRecommendation()) {
        return out;
    }
    const auto& r = m_models->recommendation();
    out[QStringLiteral("repo")] = r.repo;
    out[QStringLiteral("quant")] = r.quant;
    out[QStringLiteral("file")] = r.file;
    out[QStringLiteral("sizeBytes")] = static_cast<double>(r.sizeBytes);
    out[QStringLiteral("sizeLabel")] = r.hasSizeBytes ? formatBytes(r.sizeBytes) : QString();
    out[QStringLiteral("fits")] = r.fits;
    out[QStringLiteral("reason")] = r.reason;
    return out;
}

void DaemonModelCatalog::download(const QString& modelId) {
    Q_UNUSED(modelId) // legacy/cloud entrypoint: cloud models are usable without a download
}

void DaemonModelCatalog::downloadFile(const QString& repo, const QString& file,
                                      const QString& engine) {
    if (m_models != nullptr && !repo.isEmpty() && !file.isEmpty()) {
        setLastError({});
        m_models->download(repo, file, engine);
    }
}

void DaemonModelCatalog::pauseDownload(const QString& jobId) {
    if (m_models != nullptr) {
        m_models->pauseDownload(jobId.toULongLong());
    }
}
void DaemonModelCatalog::resumeDownload(const QString& jobId) {
    if (m_models != nullptr) {
        m_models->resumeDownload(jobId.toULongLong());
    }
}
void DaemonModelCatalog::cancelDownload(const QString& jobId) {
    if (m_models != nullptr) {
        m_models->cancelDownload(jobId.toULongLong());
    }
}

QString DaemonModelCatalog::installedIdFor(const QString& repo, const QString& file) const {
    if (m_models == nullptr || repo.isEmpty()) {
        return {};
    }
    // Exact source match first (several quants of one repo may be installed side by side)...
    for (const DecodedInstalledModel& m : m_models->installed()) {
        if (m.repo == repo && m.file == file) {
            return m.id;
        }
    }
    // ...then a repo-only fallback: a file-less download row names itself after the repo, so its
    // "file" argument never matches the installed row's real file.
    for (const DecodedInstalledModel& m : m_models->installed()) {
        if (m.repo == repo) {
            return m.id;
        }
    }
    return {};
}

void DaemonModelCatalog::dismissDownload(const QString& jobId) {
    if (m_models == nullptr || m_dismissedDownloads.contains(jobId)) {
        return;
    }
    const quint64 id = jobId.toULongLong();
    for (const DecodedDownloadStatus& s : m_models->downloads()) {
        if (s.id != id) {
            continue;
        }
        // Only a settled job may be hidden; an active row keeps rendering (pause/cancel it
        // instead), so live progress can never silently disappear.
        if (isTerminalStateToken(downloadStateToken(s.state))) {
            m_dismissedDownloads.insert(jobId);
            rebuildDownloads();
        }
        return;
    }
}

void DaemonModelCatalog::activate(const QString& modelId) {
    activateImpl(modelId, QString());
}

void DaemonModelCatalog::activateForProfile(const QString& modelId, const QString& profileId) {
    activateImpl(modelId, profileId);
}

void DaemonModelCatalog::activateImpl(const QString& modelId, const QString& profileId) {
    if (m_pickedId == modelId && (m_models == nullptr || !isLocalInstalled(modelId))) {
        return;
    }
    m_pickedId = modelId;
    if (m_models != nullptr && isLocalInstalled(modelId)) {
        // ModelActivate -> the node loads this model, bound to the EXPLICIT profile when given
        // (the node otherwise defaults to its launch profile name).
        m_models->activate(modelId, profileId);
    }
    rebuildInstalled();
    emit currentChanged();
}

void DaemonModelCatalog::remove(const QString& modelId) {
    if (m_models != nullptr && isLocalInstalled(modelId)) {
        m_models->deleteModel(modelId);
        if (m_pickedId == modelId) {
            m_pickedId.clear();
            emit currentChanged();
        }
    }
}

QObject* DaemonModelCatalog::quantizeJobs() const {
    return m_quantizeJobs;
}

void DaemonModelCatalog::quantizeModel(const QString& repo, const QString& targetQuant,
                                       const QString& sourceFile) {
    if (m_models == nullptr || repo.isEmpty() || targetQuant.isEmpty()) {
        return;
    }
    setLastError(QString()); // a fresh user-initiated operation clears the stale banner
    m_models->quantize(repo, targetQuant, sourceFile);
}

void DaemonModelCatalog::verifyInstalled(const QString& modelId) {
    if (m_models == nullptr || modelId.isEmpty() || !isLocalInstalled(modelId)) {
        return; // cloud descriptors have no on-disk artifact to probe
    }
    m_models->inspect(modelId);
}

void DaemonModelCatalog::rebuildQuantizes() {
    if (m_models == nullptr) {
        return;
    }
    QList<QVariantMap> rows;
    for (const DecodedQuantizeStatus& q : m_models->quantizes()) {
        QVariantMap row;
        row[QStringLiteral("id")] = QString::number(q.id);
        row[QStringLiteral("repo")] = q.repo;
        row[QStringLiteral("name")] =
            QStringLiteral("%1 \u2192 %2").arg(q.sourceFile, q.targetQuant);
        row[QStringLiteral("sourceFile")] = q.sourceFile;
        row[QStringLiteral("targetQuant")] = q.targetQuant;
        // Reuse the downloads panel's lowercase state tokens so one row idiom renders both.
        QString state = QStringLiteral("queued");
        if (q.state == QStringLiteral("Preparing") || q.state == QStringLiteral("Quantizing")) {
            state = QStringLiteral("downloading");
        } else if (q.state == QStringLiteral("Completed")) {
            state = QStringLiteral("done");
        } else if (q.state == QStringLiteral("Failed")) {
            state = QStringLiteral("failed");
        }
        row[QStringLiteral("state")] = state;
        row[QStringLiteral("error")] = q.error;
        row[QStringLiteral("outputPath")] = q.outputPath;
        rows.append(row);
    }
    m_quantizeJobs->setRows(rows);
}

void DaemonModelCatalog::rebuildDiscover() {
    if (m_models == nullptr) {
        return;
    }
    QList<QVariantMap> rows;
    for (const DecodedSearchHit& h : m_models->searchHits()) {
        QVariantMap row;
        row[QStringLiteral("id")] = h.repo; // VariantListModel keys on "id"
        row[QStringLiteral("repo")] = h.repo;
        row[QStringLiteral("name")] = h.repo;
        row[QStringLiteral("author")] = h.author;
        row[QStringLiteral("downloads")] = static_cast<double>(h.downloads);
        row[QStringLiteral("likes")] = static_cast<double>(h.likes);
        row[QStringLiteral("params")] =
            h.hasNumParameters ? formatParams(h.numParameters) : QString();
        row[QStringLiteral("paramsB")] =
            h.hasNumParameters ? static_cast<double>(h.numParameters) / 1e9 : 0.0;
        row[QStringLiteral("pipelineTag")] = h.pipelineTag;
        row[QStringLiteral("blurb")] = h.pipelineTag;
        row[QStringLiteral("gated")] = h.gated;
        rows.append(row);
    }
    m_discover->setRows(rows);
}

void DaemonModelCatalog::rebuildFiles() {
    if (m_models == nullptr) {
        return;
    }
    const QVariantMap rec = recommendation();
    const QString recFile = rec.value(QStringLiteral("file")).toString();
    const QString recQuant = rec.value(QStringLiteral("quant")).toString();
    QList<QVariantMap> rows;
    for (const DecodedModelFile& f : m_models->files()) {
        // Split sets: only surface the first shard as the downloadable row (it names the set).
        if (f.isSplit && !f.isFirstShard) {
            continue;
        }
        QVariantMap row;
        row[QStringLiteral("id")] = f.path; // keyed on the file path
        row[QStringLiteral("path")] = f.path;
        row[QStringLiteral("quant")] = f.quant;
        row[QStringLiteral("sizeBytes")] = static_cast<double>(f.sizeBytes);
        row[QStringLiteral("sizeLabel")] = formatBytes(f.sizeBytes);
        row[QStringLiteral("isSplit")] = f.isSplit;
        // Projector companions stay listed + downloadable (the picker badges them), but the
        // "Recommended" targeting must never land on one (the node's recommend already skips
        // them; this guards a stale/older recommendation too).
        row[QStringLiteral("isMmproj")] = f.isMmproj;
        const bool recommended =
            !f.isMmproj && ((!recFile.isEmpty() && f.path == recFile) ||
                            (recFile.isEmpty() && !recQuant.isEmpty() && f.quant == recQuant));
        row[QStringLiteral("recommended")] = recommended;
        rows.append(row);
    }
    m_files->setRows(rows);
}

void DaemonModelCatalog::rebuildDownloads() {
    if (m_models == nullptr) {
        return;
    }
    QList<QVariantMap> rows;
    for (const DecodedDownloadStatus& s : m_models->downloads()) {
        if (m_dismissedDownloads.contains(QString::number(s.id))) {
            continue; // dismissed terminal rows stay hidden across every rebuild
        }
        QVariantMap row;
        row[QStringLiteral("id")] = QString::number(s.id);
        row[QStringLiteral("modelId")] = s.modelFile.isEmpty() ? s.modelRepo : s.modelFile;
        row[QStringLiteral("name")] = s.modelFile.isEmpty() ? s.modelRepo : s.modelFile;
        row[QStringLiteral("repo")] = s.modelRepo;
        const double progress =
            s.totalBytes > 0
                ? static_cast<double>(s.downloadedBytes) / static_cast<double>(s.totalBytes)
                : (s.state == QStringLiteral("Completed") ? 1.0 : 0.0);
        row[QStringLiteral("progress")] = progress;
        row[QStringLiteral("state")] = downloadStateToken(s.state);
        row[QStringLiteral("sizeLabel")] = formatBytes(s.totalBytes);
        row[QStringLiteral("downloadedLabel")] = formatBytes(s.downloadedBytes);
        // Multi-file jobs (split GGUF sets): the panel shows "file x/y" when filesTotal > 1.
        row[QStringLiteral("filesDone")] = static_cast<int>(s.filesDone);
        row[QStringLiteral("filesTotal")] = static_cast<int>(s.filesTotal);
        row[QStringLiteral("error")] = s.error;
        rows.append(row);
    }
    m_downloads->setRows(rows);
}

void DaemonModelCatalog::rebuildInstalled() {
    if (m_models == nullptr) {
        return;
    }
    // Detect growth of the locally-installed set: each newly-observed id is a finished download
    // (or the first reflection of one), announced as downloadFinished so the provider catalog
    // re-offers local models and pickers un-stale without re-selecting the provider.
    QSet<QString> localIds;
    for (const DecodedInstalledModel& m : m_models->installed()) {
        localIds.insert(m.id);
    }
    const QSet<QString> newlyInstalled = localIds - m_knownInstalled;
    m_knownInstalled = localIds;

    const QString current = currentModelId();
    QList<QVariantMap> rows;
    // Local installed models first (they carry a quant + on-disk size).
    for (const DecodedInstalledModel& m : m_models->installed()) {
        QVariantMap row;
        row[QStringLiteral("id")] = m.id;
        row[QStringLiteral("name")] = m.displayName.isEmpty() ? m.id : m.displayName;
        row[QStringLiteral("provider")] = m.engine; // "llama" | "mistral_rs"
        row[QStringLiteral("repo")] = m.repo;
        row[QStringLiteral("file")] = m.file;
        row[QStringLiteral("quant")] = m.quant;
        row[QStringLiteral("sizeGiB")] =
            static_cast<double>(m.sizeBytes) / (1024.0 * 1024.0 * 1024.0);
        row[QStringLiteral("sizeLabel")] = formatBytes(m.sizeBytes);
        row[QStringLiteral("localPath")] = m.localPath;
        row[QStringLiteral("arch")] = m.arch;
        row[QStringLiteral("params")] = QString();
        row[QStringLiteral("local")] = true;
        row[QStringLiteral("installed")] = true;
        row[QStringLiteral("active")] = (m.id == current);
        // Vision-projector companions: badged, never activatable, excluded from chat offers
        // (provider_model_compose filters on this key). Text models with a paired projector
        // carry its path so the installed list can show a "Vision" marker.
        row[QStringLiteral("projector")] = isProjectorRecord(m);
        row[QStringLiteral("mmprojPath")] = m.mmprojPath;
        // A6 ghost state: the last ModelInspect probe on this id failed — the artifact is
        // missing/unreadable on disk (the node keeps the record). Lists render the explicit
        // missing state with a re-download affordance instead of a silent failure.
        row[QStringLiteral("missing")] = m_missingIds.contains(m.id);
        rows.append(row);
    }
    // Cloud descriptors join as ready-to-use (no download): the composer picker binds these too.
    for (const DecodedModelDescriptor& d : m_models->models()) {
        QVariantMap row;
        row[QStringLiteral("id")] = d.id;
        row[QStringLiteral("name")] = d.id;
        row[QStringLiteral("provider")] = d.provider;
        row[QStringLiteral("quant")] = QString();
        row[QStringLiteral("sizeGiB")] = 0.0;
        row[QStringLiteral("sizeLabel")] = QString();
        row[QStringLiteral("params")] = QString();
        row[QStringLiteral("local")] = false;
        row[QStringLiteral("installed")] = true;
        row[QStringLiteral("active")] = (d.id == current);
        rows.append(row);
    }
    m_installed->setRows(rows);
    // After the rows are live, so a downloadFinished consumer reading installed() sees the model.
    for (const QString& id : newlyInstalled) {
        emit downloadFinished(id);
    }
}

} // namespace models
