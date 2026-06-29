// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace uimodels {
class VariantListModel;
}

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

    // Providers are static-ish metadata (id, name, kind, configured) returned as a
    // list of maps for the Providers tab.
    [[nodiscard]] Q_INVOKABLE virtual QVariantList providers() const = 0;

    // Re-run discovery; results land in discover(). `sizeFilter` is "" (any) or a
    // params bucket like "<=8B" / ">8B".
    Q_INVOKABLE virtual void search(const QString& query, const QString& sizeFilter = {}) = 0;

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

    // Make an installed model the active default / remove it.
    Q_INVOKABLE virtual void activate(const QString& modelId) = 0;
    Q_INVOKABLE virtual void remove(const QString& modelId) = 0;

signals:
    void currentChanged();
    // Emitted when a download finishes (modelId now installed).
    void downloadFinished(const QString& modelId);
    // The files() surface now holds `repo`'s quant rows.
    void filesChanged(const QString& repo);
    // A recommendation for `repo` is now available via recommendation().
    void recommendChanged(const QString& repo);
};

} // namespace models
