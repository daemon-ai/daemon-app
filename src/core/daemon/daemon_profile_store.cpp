#include "daemon/daemon_profile_store.h"

#include "daemon/repositories.h"
#include "uimodels/variant_list_model.h"

namespace profiles {

DaemonProfileStore::DaemonProfileStore(daemonapp::daemon::ProfileRepository* repo, QObject* parent)
    : IProfileStore(parent), m_repo(repo), m_profiles(new uimodels::VariantListModel(this)) {
    if (m_repo != nullptr) {
        connect(m_repo, &daemonapp::daemon::ProfileRepository::profilesRefreshed, this,
                &DaemonProfileStore::rebuild);
    }
}

QObject* DaemonProfileStore::profiles() const {
    return m_profiles;
}

QVariantMap DaemonProfileStore::profile(const QString& id) const {
    const int row = m_profiles->indexOfId(id);
    return row >= 0 ? m_profiles->at(row) : QVariantMap{};
}

QStringList DaemonProfileStore::profileNames() const {
    QStringList out;
    if (m_repo != nullptr) {
        for (const daemonapp::daemon::DecodedProfileInfo& p : m_repo->profiles()) {
            out << p.id;
        }
    }
    return out;
}

QString DaemonProfileStore::createProfile(const QString& name) {
    Q_UNUSED(name)
    // Durable ProfileCreate (PRO-2) is a later slice; the wire op + spec form are not wired yet.
    return {};
}

void DaemonProfileStore::updateProfile(const QString& id, const QVariantMap& fields) {
    Q_UNUSED(id)
    Q_UNUSED(fields)
    // Durable in-place ProfileUpdate (PRO-3) is a later slice.
}

void DaemonProfileStore::remove(const QString& id) {
    if (m_repo != nullptr) {
        m_repo->deleteProfile(id);
    }
}

void DaemonProfileStore::setDefault(const QString& id) {
    if (m_repo != nullptr) {
        m_repo->selectProfile(id);
    }
}

void DaemonProfileStore::rebuild() {
    if (m_repo == nullptr) {
        return;
    }
    QList<QVariantMap> rows;
    m_defaultId.clear();
    for (const daemonapp::daemon::DecodedProfileInfo& p : m_repo->profiles()) {
        QVariantMap row;
        row[QStringLiteral("id")] = p.id;
        row[QStringLiteral("name")] = p.id;
        row[QStringLiteral("model")] = p.model;
        row[QStringLiteral("provider")] = p.provider;
        row[QStringLiteral("description")] = QString();
        row[QStringLiteral("systemPrompt")] = QString();
        row[QStringLiteral("skills")] = QStringList{};
        row[QStringLiteral("tools")] = QStringList{};
        row[QStringLiteral("isDefault")] = p.isActive;
        rows.append(row);
        if (p.isActive) {
            m_defaultId = p.id;
        }
    }
    m_profiles->setRows(rows);
    emit changed();
}

} // namespace profiles
