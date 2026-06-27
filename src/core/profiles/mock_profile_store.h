#pragma once

#include "profiles/iprofile_store.h"
#include "uimodels/variant_list_model.h"

namespace profiles {

// In-memory IProfileStore seeded with a few believable agent profiles plus skill
// and tool catalogs, so the editor + curator are fully interactive.
class MockProfileStore : public IProfileStore {
    Q_OBJECT

public:
    explicit MockProfileStore(QObject* parent = nullptr);

    [[nodiscard]] QObject* profiles() const override;
    [[nodiscard]] QString defaultProfileId() const override { return m_defaultId; }

    [[nodiscard]] QVariantMap profile(const QString& id) const override;
    [[nodiscard]] QStringList profileNames() const override;
    [[nodiscard]] QVariantList availableSkills() const override;
    [[nodiscard]] QVariantList availableTools() const override;

    QString createProfile(const QString& name) override;
    QString cloneProfile(const QString& source, const QString& newId) override;
    void updateProfile(const QString& id, const QVariantMap& fields) override;
    void remove(const QString& id) override;
    void setDefault(const QString& id) override;

private:
    // Persist the profile rows + default/next-id to the last-known on-disk cache.
    void save() const;

    uimodels::VariantListModel* m_profiles = nullptr;
    QString m_defaultId;
    int m_nextId = 1;
};

} // namespace profiles
