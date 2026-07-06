// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "models/imodel_catalog.h"

#include <QSet>
#include <QString>

namespace uimodels {
class VariantListModel;
}
namespace daemonapp::daemon {
class CredentialRepository;
class ModelRepository;
class ProfileRepository;
class ProviderRepository;
} // namespace daemonapp::daemon

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
    // `providers`/`credentials`/`profiles` (optional) back the Providers tab with the node's real
    // ProviderCatalog + credential presence; null falls back to an empty provider list.
    explicit DaemonModelCatalog(daemonapp::daemon::ModelRepository* models,
                                daemonapp::daemon::ProviderRepository* providers = nullptr,
                                daemonapp::daemon::CredentialRepository* credentials = nullptr,
                                daemonapp::daemon::ProfileRepository* profiles = nullptr,
                                QObject* parent = nullptr);

    [[nodiscard]] QObject* discover() const override;
    [[nodiscard]] QObject* files() const override;
    [[nodiscard]] QObject* downloads() const override;
    [[nodiscard]] QObject* installed() const override;
    [[nodiscard]] QString currentModelId() const override;
    [[nodiscard]] QString filesRepo() const override;

    [[nodiscard]] QStringList installedIds() const override;
    [[nodiscard]] QVariantList providers() const override;

    void search(const QString& query, const QString& sizeFilter = {}) override;
    [[nodiscard]] bool searchHasMore() const override;
    void searchMore() override;
    void repoFiles(const QString& repo) override;
    void recommend(const QString& repo) override;
    [[nodiscard]] QVariantMap recommendation() const override;
    void download(const QString& modelId) override;
    void downloadFile(const QString& repo, const QString& file,
                      const QString& engine = QStringLiteral("llama")) override;
    void pauseDownload(const QString& jobId) override;
    void resumeDownload(const QString& jobId) override;
    void cancelDownload(const QString& jobId) override;
    // Matches the repository's DECODED installed models by download source (repo + file, with a
    // repo-only fallback for file-less jobs whose row name is the repo itself).
    [[nodiscard]] QString installedIdFor(const QString& repo, const QString& file) const override;
    // Only a terminal-state row is dismissible; the id joins a client-side hidden set that
    // rebuildDownloads() filters on every downloadsChanged rebuild.
    void dismissDownload(const QString& jobId) override;
    void activate(const QString& modelId) override;
    void activateForProfile(const QString& modelId, const QString& profileId) override;
    void remove(const QString& modelId) override;
    // A5: local re-quantization over ModelQuantize/ModelQuantizes (jobs mirror downloads()).
    [[nodiscard]] QObject* quantizeJobs() const override;
    void quantizeModel(const QString& repo, const QString& targetQuant,
                       const QString& sourceFile = {}) override;
    // A6: ModelInspect-backed ghost probe; a failed probe marks the installed row `missing`.
    void verifyInstalled(const QString& modelId) override;

    // True when `id` names a locally-installed model (vs a cloud descriptor) — authoritative from
    // the repository's decoded installed set (not the projected rows).
    [[nodiscard]] bool isLocalInstalled(const QString& id) const override;

private:
    void rebuildDiscover();
    void rebuildFiles();
    void rebuildDownloads();
    void rebuildInstalled();
    void rebuildQuantizes();
    // Shared activate body: optional explicit profile ("" = the node's default local profile).
    void activateImpl(const QString& modelId, const QString& profileId);

    daemonapp::daemon::ModelRepository* m_models = nullptr;
    daemonapp::daemon::ProviderRepository* m_providers = nullptr;
    daemonapp::daemon::CredentialRepository* m_credentials = nullptr;
    daemonapp::daemon::ProfileRepository* m_profiles = nullptr;
    uimodels::VariantListModel* m_discover = nullptr;
    uimodels::VariantListModel* m_files = nullptr;
    uimodels::VariantListModel* m_downloads = nullptr;
    uimodels::VariantListModel* m_installed = nullptr;
    QString m_pickedId; // the user's explicit selection, overrides the daemon's resolved current
    // The locally-installed ids of the previous rebuild, so a rebuild that observes the set GROW
    // emits downloadFinished(id) (the signal that un-stales the provider picker's offered models).
    QSet<QString> m_knownInstalled;
    // Dismissed (terminal) download job ids, hidden from downloads() rebuilds. Client-side only:
    // the node retains its job history forever and the wire has no clear op.
    QSet<QString> m_dismissedDownloads;
    // A5: the quantize-job rows (mirrors m_downloads).
    uimodels::VariantListModel* m_quantizeJobs = nullptr;
    // A6: installed ids whose last ModelInspect probe FAILED (ghost artifacts); the rebuild
    // marks their rows `missing`. Cleared per-id when a later probe succeeds.
    QSet<QString> m_missingIds;
};

} // namespace models
