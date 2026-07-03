// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "models/imodel_catalog.h"
#include "uimodels/variant_list_model.h"

#include <QHash>
#include <QList>
#include <QVariantList>
#include <QVariantMap>

class QTimer;

namespace models {

// In-memory IModelCatalog: a seeded fake HF catalog with simulated, ticking
// download progress so the Models hub is fully interactive without a daemon.
class MockModelCatalog : public IModelCatalog {
    Q_OBJECT

public:
    explicit MockModelCatalog(QObject* parent = nullptr);

    [[nodiscard]] QObject* discover() const override;
    [[nodiscard]] QObject* files() const override;
    [[nodiscard]] QObject* downloads() const override;
    [[nodiscard]] QObject* installed() const override;
    [[nodiscard]] QString currentModelId() const override { return m_currentId; }
    [[nodiscard]] QString filesRepo() const override { return m_filesRepo; }

    [[nodiscard]] QStringList installedIds() const override;

    [[nodiscard]] QVariantList providers() const override;

    void search(const QString& query, const QString& sizeFilter) override;
    void repoFiles(const QString& repo) override;
    void recommend(const QString& repo) override;
    [[nodiscard]] QVariantMap recommendation() const override { return m_recommendation; }
    void download(const QString& modelId) override;
    void downloadFile(const QString& repo, const QString& file,
                      const QString& engine = QStringLiteral("llama")) override;
    void pauseDownload(const QString& jobId) override;
    void resumeDownload(const QString& jobId) override;
    void cancelDownload(const QString& jobId) override;
    // Trivial mapping: the mock's "repo" IS the catalog model id it installs under.
    [[nodiscard]] QString installedIdFor(const QString& repo, const QString& file) const override;
    // Drops a TERMINAL download row (the mock already auto-removes completed rows).
    void dismissDownload(const QString& jobId) override;
    void activate(const QString& modelId) override;
    void remove(const QString& modelId) override;

    // TEST-ONLY deterministic seeding (tst_downloads_panel): replace the download rows verbatim
    // (each element a full row map) and freeze the simulation — the tick timer stops and job
    // bookkeeping is dropped, so seeded states (queued/downloading/paused/done/failed/cancelled)
    // never mutate underneath a test. The app itself never calls this.
    Q_INVOKABLE void seedDownloadRows(const QVariantList& rows);

private:
    void tick();
    [[nodiscard]] QVariantMap catalogEntry(const QString& modelId) const;
    // Persist the installed set + active model id to the last-known on-disk cache.
    void save() const;

    uimodels::VariantListModel* m_discover = nullptr;
    uimodels::VariantListModel* m_files = nullptr;
    uimodels::VariantListModel* m_downloads = nullptr;
    uimodels::VariantListModel* m_installed = nullptr;
    QTimer* m_timer = nullptr;
    QString m_currentId;
    QString m_filesRepo;          // the model id the synthetic quant picker is open on
    QVariantMap m_recommendation; // the synthetic recommendation for m_filesRepo

    QList<QVariantMap> m_catalog;         // the full fake universe
    QHash<QString, double> m_jobProgress; // jobId -> 0..1
    QHash<QString, bool> m_jobPaused;     // jobId -> paused
    QHash<QString, QString> m_jobModel;   // jobId -> modelId
    int m_nextJob = 1;
};

} // namespace models
