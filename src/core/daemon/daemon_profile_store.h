// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "daemon/node_api_codec.h"
#include "profiles/iprofile_store.h"

#include <QHash>
#include <QString>
#include <QUrl>

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
    [[nodiscard]] QString resolveProfileRef(const QString& selector) const override;
    [[nodiscard]] QStringList profileNames() const override;
    [[nodiscard]] QVariantList availableSkills() const override { return {}; }
    [[nodiscard]] QVariantList availableTools() const override { return {}; }

    QString createProfile(const QString& name) override; // ProfileCreate
    // One ProfileCreate carrying the wizard's full spec (provider/model/baseUrl), so no
    // follow-up ProfileUpdate can race the create on the node's concurrent per-Call dispatch.
    QString createProfileWithSpec(const QString& name, const QVariantMap& fields) override;
    QString createForeignProfile(const QString& name,
                                 const QString& agent) override;                // engine=Foreign
    QString cloneProfile(const QString& source, const QString& newId) override; // ProfileClone
    void updateProfile(const QString& id, const QVariantMap& fields) override;  // ProfileUpdate
    void remove(const QString& id) override;     // ProfileDelete (PRO-4)
    void setDefault(const QString& id) override; // ProfileSelect (PRO-5)
    void refresh() override;                     // ProfileList (changed() on the reflection)

    // PRO-7 export/import + PRO-8 history/revert.
    void exportProfileToFile(const QString& id, const QUrl& fileUrl) override;
    void importProfileFromFile(const QUrl& fileUrl, const QString& newId) override;
    [[nodiscard]] QObject* history() const override;
    [[nodiscard]] bool historyAvailable() const override { return m_historyAvailable; }
    void requestHistory(const QString& id) override;
    void revertProfile(const QString& id, double seq) override;

private:
    void rebuild();
    // On a fresh list, fetch each profile's full spec so the editor can show real fields.
    void onProfilesRefreshed();

    daemonapp::daemon::ProfileRepository* m_repo = nullptr;
    uimodels::VariantListModel* m_profiles = nullptr;
    uimodels::VariantListModel* m_history = nullptr; // revisions of the last requestHistory(id)
    QString m_defaultId;
    QString m_historyId;                   // the profile m_history currently holds
    QHash<QString, QUrl> m_pendingExports; // id -> file url awaiting distributionExported
    // Whether the daemon hosts a revision log. Optimistic-true until a ProfileHistory answers
    // Unsupported (no revision log) - or, once wired, set proactively from the Hello capability.
    bool m_historyAvailable = true;
};

} // namespace profiles
