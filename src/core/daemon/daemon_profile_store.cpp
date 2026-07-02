// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_profile_store.h"

#include "daemon/repositories.h"
#include "uimodels/variant_list_model.h"

#include <QFile>

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

// Overlay the editor's spec-mapped fields onto `spec` (shared by update + full-spec create).
// name/description/skills have no ProfileSpec home (a profile's id is its name; skills are
// curator-managed), so they are intentionally ignored.
void applySpecFields(daemonapp::daemon::DecodedProfileSpec& spec, const QVariantMap& fields) {
    if (fields.contains(QStringLiteral("provider"))) {
        spec.provider = fields.value(QStringLiteral("provider")).toString();
    }
    if (fields.contains(QStringLiteral("baseUrl"))) {
        const QString base = fields.value(QStringLiteral("baseUrl")).toString();
        // Empty base URL = no override = the provider/node default (Option::None on the wire).
        spec.hasBaseUrl = !base.isEmpty();
        spec.baseUrl = base;
    }
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
}

} // namespace

DaemonProfileStore::DaemonProfileStore(daemonapp::daemon::ProfileRepository* repo, QObject* parent)
    : IProfileStore(parent), m_repo(repo), m_profiles(new uimodels::VariantListModel(this)),
      m_history(new uimodels::VariantListModel(this)) {
    if (m_repo != nullptr) {
        connect(m_repo, &daemonapp::daemon::ProfileRepository::profilesRefreshed, this,
                &DaemonProfileStore::onProfilesRefreshed);
        // A full spec arrived (ProfileGet): re-project so the editor shows real fields.
        connect(m_repo, &daemonapp::daemon::ProfileRepository::profileSpecLoaded, this,
                [this](const QString&) { rebuild(); });
        // PRO-7: a ProfileExport answered with the Distribution; flush the held bytes to the file
        // the user picked (the portable artifact is the raw, lossless response CBOR).
        connect(m_repo, &daemonapp::daemon::ProfileRepository::distributionExported, this,
                [this](const QString& id, const QByteArray& bytes,
                       const daemonapp::daemon::DecodedDistribution&) {
                    const QUrl url = m_pendingExports.take(id);
                    if (url.isEmpty()) {
                        return;
                    }
                    QFile f(url.toLocalFile());
                    if (!f.open(QIODevice::WriteOnly) || f.write(bytes) != bytes.size()) {
                        emit profileOpFailed(
                            QStringLiteral("Could not write %1").arg(f.fileName()));
                        return;
                    }
                    f.close();
                    emit exportSaved(url.toString());
                });
        connect(m_repo, &daemonapp::daemon::ProfileRepository::imported, this,
                [this](const QString& newId) { emit imported(newId); });
        connect(
            m_repo, &daemonapp::daemon::ProfileRepository::historyLoaded, this,
            [this](const QString& id, const QList<daemonapp::daemon::DecodedRevision>& revisions) {
                if (id != m_historyId) {
                    return;
                }
                QList<QVariantMap> rows;
                for (const daemonapp::daemon::DecodedRevision& r : revisions) {
                    QVariantMap row;
                    row[QStringLiteral("seq")] = static_cast<double>(r.seq);
                    row[QStringLiteral("hasParent")] = r.hasParent;
                    row[QStringLiteral("parent")] = static_cast<double>(r.parent);
                    row[QStringLiteral("author")] = r.author;
                    row[QStringLiteral("reason")] = r.reason;
                    row[QStringLiteral("tsMs")] = static_cast<double>(r.tsMs);
                    rows.prepend(row); // newest first for the panel
                }
                m_history->setRows(rows);
                if (!m_historyAvailable) {
                    m_historyAvailable = true;
                    emit historyAvailableChanged();
                }
                emit historyChanged(id);
            });
        connect(m_repo, &daemonapp::daemon::ProfileRepository::historyUnavailable, this,
                [this](const QString&, const QString&) {
                    m_history->setRows({});
                    if (m_historyAvailable) {
                        m_historyAvailable = false;
                        emit historyAvailableChanged();
                    }
                });
        connect(m_repo, &daemonapp::daemon::ProfileRepository::reverted, this,
                [this](const QString& id) { emit reverted(id); });
        // Proactive capability: once the Hello handshake lands, gate the History/Revert UI on the
        // daemon's advertised `versioning` feature instead of waiting for a request to 404.
        connect(m_repo, &daemonapp::daemon::ProfileRepository::capabilitiesChanged, this, [this] {
            const bool available = m_repo->daemonSupportsVersioning();
            if (available != m_historyAvailable) {
                m_historyAvailable = available;
                emit historyAvailableChanged();
            }
        });
        connect(m_repo, &daemonapp::daemon::ProfileRepository::operationFailed, this,
                [this](const QString& message) { emit profileOpFailed(message); });
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

QString DaemonProfileStore::resolveProfileRef(const QString& selector) const {
    if (selector.isEmpty()) {
        return {};
    }
    // Daemon profiles are id-keyed (id IS the name), so a valid selector is normally already the
    // id; still match by name defensively in case a display name ever diverges from the id.
    if (m_profiles->indexOfId(selector) >= 0) {
        return selector;
    }
    for (const QVariantMap& row : m_profiles->rows()) {
        if (row.value(QStringLiteral("name")).toString() == selector) {
            return row.value(QStringLiteral("id")).toString();
        }
    }
    return selector; // unresolved: forward verbatim (residual)
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
    // New-profile default = Daemon Cloud (daemon_api selector + its base URL): a usable, keyless
    // provider out of the box rather than the daemon's Mock/empty default. Seed from the active
    // profile instead when one exists (clone-like continuity).
    spec.provider = QStringLiteral("daemon_api");
    spec.hasBaseUrl = true;
    spec.baseUrl = QStringLiteral("https://api.daemon.ai/api/v1");
    for (const daemonapp::daemon::DecodedProfileInfo& p : m_repo->profiles()) {
        if (p.isActive) {
            spec.provider = p.provider;
            spec.model = p.model;
            spec.hasBaseUrl = false;
            spec.baseUrl.clear();
            break;
        }
    }
    m_repo->createProfile(spec);
    return id; // the row appears after the repo re-lists; ProfilesPage selects this id
}

QString DaemonProfileStore::createProfileWithSpec(const QString& name, const QVariantMap& fields) {
    if (m_repo == nullptr) {
        return {};
    }
    // The whole point over createProfile + updateProfile: ONE ProfileCreate carries the caller's
    // spec, so the node (which dispatches pipelined Calls concurrently) cannot observe a create
    // racing its own follow-up update.
    daemonapp::daemon::DecodedProfileSpec spec;
    spec.id = slugId(name);
    applySpecFields(spec, fields);
    m_repo->createProfile(spec);
    return spec.id; // the row appears after the repo re-lists
}

QString DaemonProfileStore::createAcpProfile(const QString& name, const QString& agent) {
    if (m_repo == nullptr || agent.isEmpty()) {
        return {};
    }
    const QString id = slugId(name);
    daemonapp::daemon::DecodedProfileSpec spec;
    spec.id = id;
    // A foreign (ACP) engine: the profile references the catalog entry BY NAME ONLY (the launch
    // recipe stays node-side, operator-managed). No provider/model applies — the node bypasses
    // the inference path entirely for foreign engines, so the spec's provider stays the inert
    // default rather than a fabricated cloud binding.
    spec.provider = QStringLiteral("mock");
    spec.engineKind = QStringLiteral("Acp");
    spec.engineAcpAgent = agent;
    m_repo->createProfile(spec);
    return id; // the row appears after the repo re-lists (same flow as createProfile)
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
    applySpecFields(spec, fields);
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

void DaemonProfileStore::refresh() {
    if (m_repo == nullptr) {
        emit changed(); // nothing to fetch; the (empty) state is already current
        return;
    }
    // Fresh ProfileList; the reflected response rebuilds the rows and emits changed(), so a
    // caller sequencing on changed() acts on the node's CURRENT state, never a stale snapshot.
    m_repo->refreshProfiles();
}

void DaemonProfileStore::exportProfileToFile(const QString& id, const QUrl& fileUrl) {
    if (m_repo == nullptr || id.isEmpty() || fileUrl.isEmpty()) {
        emit profileOpFailed(QStringLiteral("Cannot export profile"));
        return;
    }
    m_pendingExports.insert(id, fileUrl);
    m_repo->exportProfile(id);
}

void DaemonProfileStore::importProfileFromFile(const QUrl& fileUrl, const QString& newId) {
    if (m_repo == nullptr || fileUrl.isEmpty()) {
        emit profileOpFailed(QStringLiteral("Cannot import profile"));
        return;
    }
    QFile f(fileUrl.toLocalFile());
    if (!f.open(QIODevice::ReadOnly)) {
        emit profileOpFailed(QStringLiteral("Could not open %1").arg(f.fileName()));
        return;
    }
    const QByteArray bytes = f.readAll();
    f.close();
    daemonapp::daemon::DecodedDistribution dist;
    if (!daemonapp::daemon::NodeApiCodec::decodeDistribution(bytes, &dist)) {
        emit profileOpFailed(
            QStringLiteral("Not a valid profile distribution: %1").arg(f.fileName()));
        return;
    }
    m_repo->importProfile(dist, newId);
}

QObject* DaemonProfileStore::history() const {
    return m_history;
}

void DaemonProfileStore::requestHistory(const QString& id) {
    if (m_repo == nullptr || id.isEmpty()) {
        return;
    }
    m_historyId = id;
    m_history->setRows({}); // clear stale rows while the fetch is in flight
    m_repo->fetchProfileHistory(id);
}

void DaemonProfileStore::revertProfile(const QString& id, double seq) {
    if (m_repo != nullptr && !id.isEmpty()) {
        m_repo->revertProfile(id, static_cast<quint64>(seq));
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
            // Foreign-engine binding (wire v23): "Core" for native profiles; "Acp" + the catalog
            // agent name for foreign ones (so the UI can label/hide inference-only affordances).
            row[QStringLiteral("engine")] = spec.engineKind;
            row[QStringLiteral("acpAgent")] = spec.engineAcpAgent;
        } else {
            row[QStringLiteral("systemPrompt")] = QString();
            row[QStringLiteral("tools")] = QStringList{};
            row[QStringLiteral("engine")] = QStringLiteral("Core");
            row[QStringLiteral("acpAgent")] = QString();
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
