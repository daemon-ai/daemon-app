// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_model_catalog.h"

#include "daemon/repositories.h"
#include "models/model_format.h"
#include "uimodels/variant_list_model.h"

#include <algorithm>

namespace models {

using daemonapp::daemon::DecodedDownloadStatus;
using daemonapp::daemon::DecodedInstalledModel;
using daemonapp::daemon::DecodedModelDescriptor;
using daemonapp::daemon::DecodedModelFile;
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
} // namespace

DaemonModelCatalog::DaemonModelCatalog(ModelRepository* models, QObject* parent)
    : IModelCatalog(parent), m_models(models), m_discover(new uimodels::VariantListModel(this)),
      m_files(new uimodels::VariantListModel(this)),
      m_downloads(new uimodels::VariantListModel(this)),
      m_installed(new uimodels::VariantListModel(this)) {
    if (m_models != nullptr) {
        connect(m_models, &ModelRepository::modelsRefreshed, this,
                &DaemonModelCatalog::rebuildInstalled);
        connect(m_models, &ModelRepository::catalogChanged, this,
                &DaemonModelCatalog::rebuildInstalled);
        connect(m_models, &ModelRepository::currentRefreshed, this, [this] {
            rebuildInstalled();
            emit currentChanged();
        });
        connect(m_models, &ModelRepository::modelActivated, this, [this] {
            // The node applied the activation; re-resolve current + catalog so the marker moves.
            m_models->refreshCurrent();
            m_models->refreshCatalog();
            emit currentChanged();
        });
        connect(m_models, &ModelRepository::searchHitsChanged, this,
                &DaemonModelCatalog::rebuildDiscover);
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
        // Seed the cloud catalog + installed set + current selection on construction.
        m_models->refreshModels();
        m_models->refreshCatalog();
        m_models->refreshCurrent();
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
    const auto mkp = [](const QString& id, const QString& name, const QString& kind) {
        QVariantMap m;
        m[QStringLiteral("id")] = id;
        m[QStringLiteral("name")] = name;
        m[QStringLiteral("kind")] = kind; // "local" | "remote"
        m[QStringLiteral("configured")] = false;
        return QVariant(m);
    };
    return {
        mkp(QStringLiteral("huggingface"), QStringLiteral("Hugging Face"), QStringLiteral("local")),
        mkp(QStringLiteral("anthropic"), QStringLiteral("Anthropic"), QStringLiteral("remote")),
        mkp(QStringLiteral("openai"), QStringLiteral("OpenAI"), QStringLiteral("remote")),
        mkp(QStringLiteral("openrouter"), QStringLiteral("OpenRouter"), QStringLiteral("remote")),
        mkp(QStringLiteral("google"), QStringLiteral("Google"), QStringLiteral("remote")),
    };
}

void DaemonModelCatalog::search(const QString& query, const QString& sizeFilter) {
    Q_UNUSED(sizeFilter)
    if (m_models == nullptr) {
        return;
    }
    // Local-track discovery is repo search; an empty query still asks for trending repos.
    m_models->search(query);
}

void DaemonModelCatalog::repoFiles(const QString& repo) {
    if (m_models == nullptr || repo.isEmpty()) {
        return;
    }
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

void DaemonModelCatalog::activate(const QString& modelId) {
    if (m_pickedId == modelId && (m_models == nullptr || !isLocalInstalled(modelId))) {
        return;
    }
    m_pickedId = modelId;
    if (m_models != nullptr && isLocalInstalled(modelId)) {
        m_models->activate(modelId); // ModelActivate -> the node loads this model by default
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
        const bool recommended = (!recFile.isEmpty() && f.path == recFile) ||
                                 (recFile.isEmpty() && !recQuant.isEmpty() && f.quant == recQuant);
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
        row[QStringLiteral("error")] = s.error;
        rows.append(row);
    }
    m_downloads->setRows(rows);
}

void DaemonModelCatalog::rebuildInstalled() {
    if (m_models == nullptr) {
        return;
    }
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
}

} // namespace models
