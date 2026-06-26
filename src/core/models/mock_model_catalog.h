#pragma once

#include "models/imodel_catalog.h"
#include "uimodels/variant_list_model.h"

#include <QHash>
#include <QList>
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
    [[nodiscard]] QObject* downloads() const override;
    [[nodiscard]] QObject* installed() const override;
    [[nodiscard]] QString currentModelId() const override { return m_currentId; }

    [[nodiscard]] QStringList installedIds() const override;

    [[nodiscard]] QVariantList providers() const override;

    void search(const QString& query, const QString& sizeFilter) override;
    void download(const QString& modelId) override;
    void pauseDownload(const QString& jobId) override;
    void resumeDownload(const QString& jobId) override;
    void cancelDownload(const QString& jobId) override;
    void activate(const QString& modelId) override;
    void remove(const QString& modelId) override;

private:
    void tick();
    [[nodiscard]] QVariantMap catalogEntry(const QString& modelId) const;
    // Persist the installed set + active model id to the last-known on-disk cache.
    void save() const;

    uimodels::VariantListModel* m_discover = nullptr;
    uimodels::VariantListModel* m_downloads = nullptr;
    uimodels::VariantListModel* m_installed = nullptr;
    QTimer* m_timer = nullptr;
    QString m_currentId;

    QList<QVariantMap> m_catalog;         // the full fake universe
    QHash<QString, double> m_jobProgress; // jobId -> 0..1
    QHash<QString, bool> m_jobPaused;     // jobId -> paused
    QHash<QString, QString> m_jobModel;   // jobId -> modelId
    int m_nextJob = 1;
};

} // namespace models
