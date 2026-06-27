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

// Daemon-backed model catalog. Two tracks, both real:
//   - Cloud (CON-6): the node's `Models` op + `ModelCurrent` surface ready-to-use cloud models in
//     installed() (no download needed); activate() records the pick / triggers ModelActivate.
//   - Local (Phase 2): `ModelSearch` repos fill discover(); selecting one loads its quant files
//     (`ModelFiles` + `ModelRecommend`) into files(); downloadFile() runs `ModelDownload` and the
//     repo's poll loop streams `ModelDownloads` into downloads(); `ModelCatalog` installed models
//     join the installed() set; activate()/remove() map to `ModelActivate`/`ModelDelete`.
//
// Lives under src/core/daemon (not models/) because it depends on the daemon ModelRepository, which
// links the models interface - keeping it here avoids a library cycle.
class DaemonModelCatalog : public IModelCatalog {
    Q_OBJECT

public:
    DaemonModelCatalog(daemonapp::daemon::ModelRepository* models, QObject* parent = nullptr);

    [[nodiscard]] QObject* discover() const override;
    [[nodiscard]] QObject* files() const override;
    [[nodiscard]] QObject* downloads() const override;
    [[nodiscard]] QObject* installed() const override;
    [[nodiscard]] QString currentModelId() const override;
    [[nodiscard]] QString filesRepo() const override;

    [[nodiscard]] QStringList installedIds() const override;
    [[nodiscard]] QVariantList providers() const override;

    void search(const QString& query, const QString& sizeFilter = {}) override;
    void repoFiles(const QString& repo) override;
    void recommend(const QString& repo) override;
    [[nodiscard]] QVariantMap recommendation() const override;
    void download(const QString& modelId) override;
    void downloadFile(const QString& repo, const QString& file,
                      const QString& engine = QStringLiteral("llama")) override;
    void pauseDownload(const QString& jobId) override;
    void resumeDownload(const QString& jobId) override;
    void cancelDownload(const QString& jobId) override;
    void activate(const QString& modelId) override;
    void remove(const QString& modelId) override;

private:
    void rebuildDiscover();
    void rebuildFiles();
    void rebuildDownloads();
    void rebuildInstalled();
    // True when `id` names a locally-installed model (vs a cloud descriptor).
    [[nodiscard]] bool isLocalInstalled(const QString& id) const;

    daemonapp::daemon::ModelRepository* m_models = nullptr;
    uimodels::VariantListModel* m_discover = nullptr;
    uimodels::VariantListModel* m_files = nullptr;
    uimodels::VariantListModel* m_downloads = nullptr;
    uimodels::VariantListModel* m_installed = nullptr;
    QString m_pickedId; // the user's explicit selection, overrides the daemon's resolved current
};

} // namespace models
