#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

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
    // Search results for the current query (Discover tab).
    Q_PROPERTY(QObject* discover READ discover CONSTANT)
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
    [[nodiscard]] virtual QObject* downloads() const = 0;
    [[nodiscard]] virtual QObject* installed() const = 0;
    [[nodiscard]] virtual QString currentModelId() const = 0;

    // The ids of the installed models, for QML dropdowns (profile model, routing
    // targets, ...) that need a plain string list rather than the row model.
    [[nodiscard]] Q_INVOKABLE virtual QStringList installedIds() const = 0;

    // Providers are static-ish metadata (id, name, kind, configured) returned as a
    // list of maps for the Providers tab.
    [[nodiscard]] Q_INVOKABLE virtual QVariantList providers() const = 0;

    // Re-run discovery; results land in discover(). `sizeFilter` is "" (any) or a
    // params bucket like "<=8B" / ">8B".
    Q_INVOKABLE virtual void search(const QString& query, const QString& sizeFilter = {}) = 0;

    // Start a download for a discover-row model id; progress streams via the
    // downloads() model, and on completion the model lands in installed().
    Q_INVOKABLE virtual void download(const QString& modelId) = 0;
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
};

} // namespace models
