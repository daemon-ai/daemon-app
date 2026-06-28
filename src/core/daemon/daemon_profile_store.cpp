#include "daemon/daemon_profile_store.h"

#include "daemon/repositories.h"
#include "uimodels/variant_list_model.h"

namespace profiles {

namespace {

// Derive a stable profile id from a free-text name (the daemon keys profiles by id; there is no
// separate display name). Lowercase, non-alphanumerics collapse to '-'.
QString slugId(const QString& name) {
    QString slug;
    for (const QChar c : name.toLower()) {
        slug.append(c.isLetterOrNumber() ? c : QChar('-'));
    }
    while (slug.contains(QStringLiteral("--"))) {
        slug.replace(QStringLiteral("--"), QStringLiteral("-"));
    }
    slug = slug.mid(0, 64);
    while (slug.startsWith('-')) {
        slug.remove(0, 1);
    }
    while (slug.endsWith('-')) {
        slug.chop(1);
    }
    return slug.isEmpty() ? QStringLiteral("profile") : slug;
}

} // namespace

DaemonProfileStore::DaemonProfileStore(daemonapp::daemon::ProfileRepository* repo, QObject* parent)
    : IProfileStore(parent), m_repo(repo), m_profiles(new uimodels::VariantListModel(this)) {
    if (m_repo != nullptr) {
        connect(m_repo, &daemonapp::daemon::ProfileRepository::profilesRefreshed, this,
                &DaemonProfileStore::onProfilesRefreshed);
        // A full spec arrived (ProfileGet): re-project so the editor shows real fields.
        connect(m_repo, &daemonapp::daemon::ProfileRepository::profileSpecLoaded, this,
                [this](const QString&) { rebuild(); });
        // Offline-first: seed from the local cache so the Profiles UI renders the last-known agents
        // immediately, before (or entirely without) a daemon connection. The connect-ready
        // refreshProfiles() then reconciles against the live node when it comes online.
        m_repo->loadCachedProfiles();
        rebuild();
    }
}

void DaemonProfileStore::onProfilesRefreshed() {
    // Fetch each profile's full spec (system_prompt, tool_allowlist, ...) so profile(id) returns
    // editable fields; rebuild now shows the sparse list immediately, enriched as specs arrive.
    if (m_repo != nullptr) {
        for (const daemonapp::daemon::DecodedProfileInfo& p : m_repo->profiles()) {
            m_repo->getProfile(p.id);
        }
    }
    rebuild();
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
    if (m_repo == nullptr) {
        return {};
    }
    const QString id = slugId(name);
    daemonapp::daemon::DecodedProfileSpec spec;
    spec.id = id;
    // Seed provider/model from the active profile so the new profile is usable out of the box
    // (the daemon defaults provider=Mock/model="" otherwise, which can't run a real turn).
    for (const daemonapp::daemon::DecodedProfileInfo& p : m_repo->profiles()) {
        if (p.isActive) {
            spec.provider = p.provider;
            spec.model = p.model;
            break;
        }
    }
    m_repo->createProfile(spec);
    return id; // the row appears after the repo re-lists; ProfilesPage selects this id
}

QString DaemonProfileStore::cloneProfile(const QString& source, const QString& newId) {
    if (m_repo == nullptr || newId.isEmpty()) {
        return {};
    }
    const QString id = slugId(newId);
    m_repo->cloneProfile(source, id);
    return id;
}

void DaemonProfileStore::updateProfile(const QString& id, const QVariantMap& fields) {
    if (m_repo == nullptr || id.isEmpty()) {
        return;
    }
    // Base the update on the full cached spec (from ProfileGet) so untouched fields are preserved;
    // fall back to the sparse list row when the spec has not loaded yet.
    daemonapp::daemon::DecodedProfileSpec spec;
    if (!m_repo->cachedSpec(id, &spec)) {
        spec.id = id;
        for (const daemonapp::daemon::DecodedProfileInfo& p : m_repo->profiles()) {
            if (p.id == id) {
                spec.provider = p.provider;
                spec.model = p.model;
                break;
            }
        }
    }
    // Overlay the editor's spec-mapped fields. name/description/skills have no ProfileSpec home
    // (a profile's id is its name; skills are curator-managed), so they are intentionally ignored.
    if (fields.contains(QStringLiteral("model"))) {
        spec.model = fields.value(QStringLiteral("model")).toString();
    }
    if (fields.contains(QStringLiteral("systemPrompt"))) {
        spec.systemPrompt = fields.value(QStringLiteral("systemPrompt")).toString();
    }
    if (fields.contains(QStringLiteral("tools"))) {
        const QStringList tools = fields.value(QStringLiteral("tools")).toStringList();
        spec.hasToolAllowlist = true; // an explicit (possibly empty) allowlist
        spec.toolAllowlist = tools;
    }
    m_repo->updateProfile(spec);
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
        row[QStringLiteral("name")] = p.id; // daemon profiles are id-keyed; id IS the name
        row[QStringLiteral("model")] = p.model;
        row[QStringLiteral("provider")] = p.provider;
        row[QStringLiteral("isDefault")] = p.isActive;
        // Enrich with the full spec when a ProfileGet has loaded it (the editable fields).
        daemonapp::daemon::DecodedProfileSpec spec;
        if (m_repo->cachedSpec(p.id, &spec)) {
            row[QStringLiteral("systemPrompt")] = spec.systemPrompt;
            row[QStringLiteral("tools")] =
                spec.hasToolAllowlist ? spec.toolAllowlist : QStringList{};
            row[QStringLiteral("baseUrl")] = spec.hasBaseUrl ? spec.baseUrl : QString();
            row[QStringLiteral("contextEngine")] = spec.contextEngine;
            row[QStringLiteral("memoryProvider")] = spec.memoryProvider;
            row[QStringLiteral("credentialRef")] =
                spec.hasCredentialRef ? spec.credentialRef : QString();
        } else {
            row[QStringLiteral("systemPrompt")] = QString();
            row[QStringLiteral("tools")] = QStringList{};
        }
        // No ProfileSpec home: presentation-only / curator-managed.
        row[QStringLiteral("description")] = QString();
        row[QStringLiteral("skills")] = QStringList{};
        rows.append(row);
        if (p.isActive) {
            m_defaultId = p.id;
        }
    }
    m_profiles->setRows(rows);
    emit changed();
}

} // namespace profiles
