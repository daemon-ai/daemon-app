#pragma once

#include "profiles/iprofile_store.h"

#include <QString>

namespace uimodels {
class VariantListModel;
}
namespace daemonapp::daemon {
class ProfileRepository;
}

namespace profiles {

// Daemon-backed profile store (PRO-1 / PRO-5): the profile list is projected from the node's
// ProfileList (via ProfileRepository), setDefault switches the active profile (ProfileSelect), and
// remove deletes one (ProfileDelete). Create/update are the durable ProfileCreate/ProfileUpdate
// editors (PRO-2/PRO-3), deferred in this slice.
//
// Lives under src/core/daemon (not profiles/) because it depends on the daemon ProfileRepository,
// which links the profiles interface - keeping it here avoids a library cycle.
class DaemonProfileStore : public IProfileStore {
    Q_OBJECT

public:
    DaemonProfileStore(daemonapp::daemon::ProfileRepository* repo, QObject* parent = nullptr);

    [[nodiscard]] QObject* profiles() const override;
    [[nodiscard]] QString defaultProfileId() const override { return m_defaultId; }

    [[nodiscard]] QVariantMap profile(const QString& id) const override;
    [[nodiscard]] QStringList profileNames() const override;
    [[nodiscard]] QVariantList availableSkills() const override { return {}; }
    [[nodiscard]] QVariantList availableTools() const override { return {}; }

    QString createProfile(const QString& name) override;                       // deferred (PRO-2)
    void updateProfile(const QString& id, const QVariantMap& fields) override; // deferred (PRO-3)
    void remove(const QString& id) override;     // ProfileDelete (PRO-4)
    void setDefault(const QString& id) override; // ProfileSelect (PRO-5)

private:
    void rebuild();

    daemonapp::daemon::ProfileRepository* m_repo = nullptr;
    uimodels::VariantListModel* m_profiles = nullptr;
    QString m_defaultId;
};

} // namespace profiles
