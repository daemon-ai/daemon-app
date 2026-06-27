#pragma once

#include "models/imodel_catalog.h"

#include <QString>

namespace uimodels {
class VariantListModel;
}
namespace daemonapp::daemon {
class ModelRepository;
}

namespace models {

// Daemon-backed model catalog (CON-6): discovery via the node's `Models` op and the active model
// via `ModelCurrent`, both projected into the hub's row models. Cloud models need no download, so
// they surface directly as "installed" (ready to use); the download tabs stay empty in this slice.
// `activate` records the user's choice (the first-send / SetSessionModel applies it once a session
// exists); readiness (CON-7) is satisfied by either an explicit pick or a resolved ModelCurrent.
//
// Lives under src/core/daemon (not models/) because it depends on the daemon ModelRepository, which
// links the models interface - keeping it here avoids a library cycle.
class DaemonModelCatalog : public IModelCatalog {
    Q_OBJECT

public:
    DaemonModelCatalog(daemonapp::daemon::ModelRepository* models, QObject* parent = nullptr);

    [[nodiscard]] QObject* discover() const override;
    [[nodiscard]] QObject* downloads() const override;
    [[nodiscard]] QObject* installed() const override;
    [[nodiscard]] QString currentModelId() const override;

    [[nodiscard]] QStringList installedIds() const override;
    [[nodiscard]] QVariantList providers() const override;

    void search(const QString& query, const QString& sizeFilter = {}) override;
    void download(const QString& modelId) override;
    void pauseDownload(const QString& jobId) override;
    void resumeDownload(const QString& jobId) override;
    void cancelDownload(const QString& jobId) override;
    void activate(const QString& modelId) override;
    void remove(const QString& modelId) override;

private:
    void rebuild();

    daemonapp::daemon::ModelRepository* m_models = nullptr;
    uimodels::VariantListModel* m_discover = nullptr;
    uimodels::VariantListModel* m_downloads = nullptr;
    uimodels::VariantListModel* m_installed = nullptr;
    QString m_pickedId; // the user's explicit selection, overrides the daemon's resolved current
};

} // namespace models
