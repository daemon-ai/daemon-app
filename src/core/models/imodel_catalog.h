// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "models/provider_filter.h"
#include "uimodels/variant_list_model.h"

#include <algorithm>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace models {

// Model-catalog seam backing the Models hub (Discover / Downloads / Installed /
// Providers). The mock simulates HF-style search and download progress; a daemon
// adapter later maps these to model_search / model_files / model_download /
// model_catalog / model_activate / model_delete over IConnectionService.
//
// The three list surfaces are exposed as live VariantListModel* so QML ListViews
// bind directly and update in place (download progress, install/remove). Each row
// is a QVariantMap; see the mock for the field shapes.
class IModelCatalog : public QObject {
    Q_OBJECT
    // Search results for the current query (Discover tab). In the local track these are repos.
    Q_PROPERTY(QObject* discover READ discover CONSTANT)
    // The loadable files (grouped quant rows) of the repo the quant picker is open on.
    Q_PROPERTY(QObject* files READ files CONSTANT)
    // Active/queued/finished download jobs (Downloads tab).
    Q_PROPERTY(QObject* downloads READ downloads CONSTANT)
    // Locally installed models (Installed tab).
    Q_PROPERTY(QObject* installed READ installed CONSTANT)
    // The currently-active default model id ("" if none).
    Q_PROPERTY(QString currentModelId READ currentModelId NOTIFY currentChanged)
    // The most recent model-operation failure (HF network error, gated repo, failed activate...),
    // "" when none. The Models pages + the wizard discover dialog render it inline; a new
    // user-initiated operation clears it.
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    // Whether another Discover search page is likely available (the last search page came back
    // full); searchMore() appends it.
    Q_PROPERTY(bool searchHasMore READ searchHasMore NOTIFY searchHasMoreChanged)

public:
    using QObject::QObject;
    ~IModelCatalog() override = default;

    [[nodiscard]] virtual QObject* discover() const = 0;
    [[nodiscard]] virtual QObject* files() const = 0;
    [[nodiscard]] virtual QObject* downloads() const = 0;
    [[nodiscard]] virtual QObject* installed() const = 0;
    [[nodiscard]] virtual QString currentModelId() const = 0;
    // The repo the files() surface currently holds ("" if none loaded).
    [[nodiscard]] virtual QString filesRepo() const = 0;

    // The ids of the installed models, for QML dropdowns (profile model, routing
    // targets, ...) that need a plain string list rather than the row model.
    [[nodiscard]] Q_INVOKABLE virtual QStringList installedIds() const = 0;

    // The installed model ids served by `provider` (a ProviderSelector wire string), for the
    // profile editor's provider-filtered model combo. Falls back to ALL installed ids when the
    // provider is empty or matches nothing installed (remote providers like daemon_api/genai often
    // have no local catalog rows, and the mock tags rows with vendor names), so the combo is never
    // wrongly empty.
    [[nodiscard]] Q_INVOKABLE QStringList installedIdsForProvider(const QString& provider) const {
        auto* model = qobject_cast<uimodels::VariantListModel*>(installed());
        const QStringList filtered = model != nullptr
                                         ? filterInstalledIdsByProvider(model->rows(), provider)
                                         : QStringList{};
        return filtered.isEmpty() ? installedIds() : filtered;
    }

    // Providers are static-ish metadata (id, name, kind, configured) returned as a
    // list of maps for the Providers tab.
    [[nodiscard]] Q_INVOKABLE virtual QVariantList providers() const = 0;

    [[nodiscard]] QString lastError() const { return m_lastError; }

    // Whether `id` names a LOCALLY installed model (vs a ready-to-use cloud descriptor) — the ids
    // ModelActivate accepts. Default: scan the installed() rows for `id` with `local == true`.
    [[nodiscard]] Q_INVOKABLE virtual bool isLocalInstalled(const QString& id) const {
        auto* model = qobject_cast<uimodels::VariantListModel*>(installed());
        if (model == nullptr) {
            return false;
        }
        const QList<QVariantMap> rows = model->rows();
        return std::any_of(rows.cbegin(), rows.cend(), [&id](const QVariantMap& row) {
            return row.value(QStringLiteral("id")).toString() == id &&
                   row.value(QStringLiteral("local")).toBool();
        });
    }

    // Re-run discovery; results land in discover(). `sizeFilter` is "" (any) or a
    // params bucket like "<=8B" / ">8B".
    Q_INVOKABLE virtual void search(const QString& query, const QString& sizeFilter = {}) = 0;

    [[nodiscard]] virtual bool searchHasMore() const { return false; }
    // Fetch + APPEND the next Discover page for the current query (no-op when searchHasMore is
    // false or the backing catalog does not page).
    Q_INVOKABLE virtual void searchMore() {}

    // Local-track step 2: load a repo's loadable files (grouped quant rows) into files() and
    // request a hardware-aware recommendation. filesChanged(repo) / recommendChanged(repo) fire.
    Q_INVOKABLE virtual void repoFiles(const QString& repo) = 0;
    // Request only the recommendation for a repo (recommendChanged(repo) fires).
    Q_INVOKABLE virtual void recommend(const QString& repo) = 0;
    // The latest quant recommendation as a row map ({quant, file, sizeLabel, reason, fits, repo})
    // or an empty map if none is loaded. The quant picker pre-highlights this entry.
    [[nodiscard]] Q_INVOKABLE virtual QVariantMap recommendation() const = 0;

    // Start a download for a discover-row id (legacy/cloud); the local track uses downloadFile().
    Q_INVOKABLE virtual void download(const QString& modelId) = 0;
    // Local-track step 3: download a specific quant file from a repo. Progress streams via
    // downloads(); on completion the model lands in installed().
    Q_INVOKABLE virtual void downloadFile(const QString& repo, const QString& file,
                                          const QString& engine = QStringLiteral("llama")) = 0;
    Q_INVOKABLE virtual void pauseDownload(const QString& jobId) = 0;
    Q_INVOKABLE virtual void resumeDownload(const QString& jobId) = 0;
    Q_INVOKABLE virtual void cancelDownload(const QString& jobId) = 0;

    // Resolve the installed CATALOG id for a completed download. Download rows carry the source
    // repo + file (NOT the hashed catalog id that offered-model rows / activate / downloadFinished
    // use), so the downloads UI maps a done row through this before selecting it. "" when nothing
    // installed matches.
    [[nodiscard]] Q_INVOKABLE virtual QString installedIdFor(const QString& repo,
                                                             const QString& file) const = 0;
    // Hide a TERMINAL download row (done/failed/cancelled) client-side. The node keeps finished
    // jobs forever and the wire has no clear-history op, so dismissal is a per-session view
    // filter; an ACTIVE row cannot be dismissed (pause or cancel it instead).
    Q_INVOKABLE virtual void dismissDownload(const QString& jobId) = 0;

    // Make an installed model the active default / remove it.
    Q_INVOKABLE virtual void activate(const QString& modelId) = 0;
    Q_INVOKABLE virtual void remove(const QString& modelId) = 0;

    // --- Local re-quantization (A5 / CON-13) + ghost-model probe (A6 / CON-14) ---------------
    // Live quantize-job rows ({id, repo, sourceFile, targetQuant, state, error}), mirroring
    // downloads(). Null when the backing catalog has no quantize capability (the mock).
    [[nodiscard]] virtual QObject* quantizeJobs() const { return nullptr; }
    // Start a local re-quantization of an INSTALLED repo toward `targetQuant` (empty
    // `sourceFile` lets the node pick the best installed source). Progress streams via
    // quantizeJobs(); on completion the produced model joins installed(). Default: no-op (mock).
    Q_INVOKABLE virtual void quantizeModel(const QString& repo, const QString& targetQuant,
                                           const QString& sourceFile = {}) {
        Q_UNUSED(repo)
        Q_UNUSED(targetQuant)
        Q_UNUSED(sourceFile)
    }
    // A6 ghost probe: verify an installed model's artifact is still present + readable on disk
    // (the node keeps catalog records for deleted files). Resolves asynchronously to
    // modelVerified(id) or modelMissing(id, reason); the daemon impl also marks the installed()
    // row `missing` so list surfaces render the explicit state. Detection stays behind this one
    // seam so a future node `on_disk` flag can replace the probe without UI changes. Default:
    // no-op (mock artifacts are not real files).
    Q_INVOKABLE virtual void verifyInstalled(const QString& modelId) { Q_UNUSED(modelId) }

    // Activate an installed model for an EXPLICIT profile id (the wizard / profile-editor apply
    // path): the node's ModelActivate defaults to the launch profile name otherwise, which is not
    // necessarily the profile the user just configured. Default: same as activate() (the mock has
    // one global selection).
    Q_INVOKABLE virtual void activateForProfile(const QString& modelId, const QString& profileId) {
        Q_UNUSED(profileId)
        activate(modelId);
    }

signals:
    void currentChanged();
    // A download job was accepted and is now running (jobId as shown in downloads()).
    void downloadStarted(const QString& jobId);
    // Emitted when a download finishes (modelId now installed).
    void downloadFinished(const QString& modelId);
    // A5: a quantize job was accepted (id as shown in quantizeJobs()).
    void quantizeStarted(const QString& jobId);
    // A6: a verifyInstalled probe resolved — the artifact is present+readable / missing.
    void modelVerified(const QString& modelId);
    void modelMissing(const QString& modelId, const QString& reason);
    // The files() surface now holds `repo`'s quant rows.
    void filesChanged(const QString& repo);
    // A recommendation for `repo` is now available via recommendation().
    void recommendChanged(const QString& repo);
    void lastErrorChanged();
    void searchHasMoreChanged();

protected:
    // Record (or clear, with "") the surfaced model-operation failure; emits lastErrorChanged.
    void setLastError(const QString& message) {
        if (m_lastError == message) {
            return;
        }
        m_lastError = message;
        emit lastErrorChanged();
    }

private:
    QString m_lastError;
};

} // namespace models
