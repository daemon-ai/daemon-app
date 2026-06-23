#pragma once

#include "automation/icron_store.h"

#include "uimodels/variant_list_model.h"

namespace automation {

class MockCronStore : public ICronStore {
    Q_OBJECT

public:
    explicit MockCronStore(QObject* parent = nullptr);

    [[nodiscard]] QObject* jobs() const override;
    [[nodiscard]] QVariantMap job(const QString& id) const override;

    QString createJob(const QString& name, const QString& schedule,
                      const QString& profile) override;
    void updateJob(const QString& id, const QVariantMap& fields) override;
    void setEnabled(const QString& id, bool enabled) override;
    void runNow(const QString& id) override;
    void remove(const QString& id) override;

private:
    // Persist the job rows + next-id to the last-known on-disk cache.
    void save() const;

    uimodels::VariantListModel* m_jobs = nullptr;
    int m_nextId = 1;
};

} // namespace automation
