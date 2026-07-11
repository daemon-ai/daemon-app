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
    if (fields.contains(QStringLiteral("tools"))) {
        const QStringList tools = fields.value(QStringLiteral("tools")).toStringList();
        spec.hasToolAllowlist = true; // an explicit (possibly empty) allowlist
        spec.toolAllowlist = tools;
    }
    // Foreign-engine model backend (wire v30). `foreignBackend` is the AgentSetupModel projection:
    // {mode, provider (NodeProvider wire selector), model (NodeProvider model / AgentNative hint),
    // credentialRef}. Only meaningful for a Foreign engine; the node defaults an absent backend to
    // AgentNative{model:null}, so a missing key leaves the spec's backend untouched.
    if (fields.contains(QStringLiteral("foreignBackend"))) {
        const QVariantMap fb = fields.value(QStringLiteral("foreignBackend")).toMap();
        spec.hasForeignBackend = true;
        daemonapp::daemon::DecodedForeignBackend& backend = spec.foreignBackend;
        if (fb.value(QStringLiteral("mode")).toString() == QStringLiteral("NodeProvider")) {
            backend.kind = QStringLiteral("NodeProvider");
            backend.nodeProvider = fb.value(QStringLiteral("provider")).toString();
            backend.nodeModel = fb.value(QStringLiteral("model")).toString();
            const QString cred = fb.value(QStringLiteral("credentialRef")).toString();
            backend.hasNodeCredentialRef = !cred.isEmpty();
            backend.nodeCredentialRef = cred;
        } else {
            backend.kind = QStringLiteral("AgentNative");
            const QString hint = fb.value(QStringLiteral("model")).toString();
            backend.hasAgentNativeModel = !hint.isEmpty();
            backend.agentNativeModel = hint;
        }
    }
}

// Project a decoded foreign-backend (wire v30) into the compact row map the AgentSetupModel editor
// + setup surfaces consume: {mode, provider, model, credentialRef}. NodeProvider carries the node
// provider selector + model (+ optional credential ref); AgentNative carries only the optional
// steer hint under `model`.
QVariantMap foreignBackendRow(const daemonapp::daemon::DecodedForeignBackend& backend) {
    QVariantMap row;
    row[QStringLiteral("mode")] = backend.kind;
    if (backend.kind == QStringLiteral("NodeProvider")) {
        row[QStringLiteral("provider")] = backend.nodeProvider;
        row[QStringLiteral("model")] = backend.nodeModel;
        row[QStringLiteral("credentialRef")] =
            backend.hasNodeCredentialRef ? backend.nodeCredentialRef : QString();
    } else {
        row[QStringLiteral("provider")] = QString();
        row[QStringLiteral("model")] =
            backend.hasAgentNativeModel ? backend.agentNativeModel : QString();
        row[QStringLiteral("credentialRef")] = QString();
    }
    return row;
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
        // Persona (SOUL.md) seam: a SoulGet answer (direct, or after a SoulSet re-fetch) means
        // soul(id) now returns fresh authoritative content -> announce it (never optimistically).
        // A SoulSet rejection surfaces on the existing profileOpFailed channel.
        connect(m_repo, &daemonapp::daemon::ProfileRepository::soulLoaded, this,
                [this](const QString& id) { emit soulChanged(id); });
        connect(m_repo, &daemonapp::daemon::ProfileRepository::soulOpFailed, this,
                [this](const QString& message) { emit profileOpFailed(message); });
        // Offline-first: seed from the local cache so the Profiles UI renders the last-known agents
        // immediately, before (or entirely without) a daemon connection. The connect-ready
        // refreshProfiles() then reconciles against the live node when it comes online.
        m_repo->loadCachedProfiles();
        rebuild();
    }
}

void DaemonProfileStore::onProfilesRefreshed() {
    // Fetch each profile's full spec (tool_allowlist, tunables, ...) so profile(id) returns
    // editable fields; rebuild now shows the sparse list immediately, enriched as specs arrive.
    // (The persona is not a spec field — it rides the SoulGet/SoulSet seam below.)
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

QString DaemonProfileStore::soul(const QString& profileId) const {
    return m_repo != nullptr ? m_repo->cachedSoul(profileId) : QString();
}

void DaemonProfileStore::requestSoul(const QString& profileId) {
    if (m_repo != nullptr) {
        m_repo->requestSoul(profileId);
    }
}

void DaemonProfileStore::setSoul(const QString& profileId, const QString& text) {
    if (m_repo != nullptr) {
        m_repo->setSoul(profileId, text);
    }
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

QString DaemonProfileStore::createForeignProfile(const QString& name, const QString& agent) {
    if (m_repo == nullptr || agent.isEmpty()) {
        return {};
    }
    const QString id = slugId(name);
    daemonapp::daemon::DecodedProfileSpec spec;
    spec.id = id;
    // A foreign engine: the profile references the catalog entry BY NAME ONLY (the launch recipe
    // stays node-side, operator-managed). No provider/model applies — the node bypasses the
    // inference path entirely for foreign engines, so the spec's provider stays the inert default
    // rather than a fabricated cloud binding.
    spec.provider = QStringLiteral("mock");
    spec.engineKind = QStringLiteral("Foreign");
    spec.engineForeignAgent = agent;
    // Set an explicit AgentNative backend (wire v30): the agent uses its own login/config and the
    // node steers only its model (no hint here). Equivalent to the node's default for an absent
    // backend, but stated so a foreign profile always carries a concrete foreign_backend.
    spec.hasForeignBackend = true;
    spec.foreignBackend.kind = QStringLiteral("AgentNative");
    m_repo->createProfile(spec);
    return id; // the row appears after the repo re-lists (same flow as createProfile)
}

QString DaemonProfileStore::createForeignProfileWithBackend(const QString& name,
                                                            const QString& agent,
                                                            const QVariantMap& backend) {
    if (m_repo == nullptr || agent.isEmpty()) {
        return {};
    }
    const QString id = slugId(name);
    daemonapp::daemon::DecodedProfileSpec spec;
    spec.id = id;
    // A foreign engine references the catalog entry BY NAME ONLY; provider stays the inert default
    // (the node bypasses the inference path for foreign engines). The foreign_backend is applied
    // from `backend` via applySpecFields.
    spec.provider = QStringLiteral("mock");
    spec.engineKind = QStringLiteral("Foreign");
    spec.engineForeignAgent = agent;
    QVariantMap fields;
    fields[QStringLiteral("foreignBackend")] = backend;
    applySpecFields(spec, fields);
    m_repo->createProfile(spec);
    return id; // the row appears after the repo re-lists (same flow as createForeignProfile)
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
        // Provenance (wire v31), projected from the ProfileList entry so the list can badge +
        // subtree-filter WITHOUT waiting for a per-profile ProfileGet. `createdBy` is the wire
        // author ("operator" or an agent id); `createdByIsAgent` disambiguates an agent literally
        // named "operator"; `owner` is the owning agent session id. Absent -> operator-authored.
        // Consumed verbatim by the provenance surface (phase H) - never re-derived client-side.
        row[QStringLiteral("createdBy")] =
            p.hasCreatedBy ? p.createdBy : QStringLiteral("operator");
        row[QStringLiteral("createdByIsAgent")] = p.hasCreatedBy && p.createdByIsAgent;
        row[QStringLiteral("owner")] = p.hasOwner ? p.owner : QString();
        // Enrich with the full spec when a ProfileGet has loaded it (the editable fields).
        daemonapp::daemon::DecodedProfileSpec spec;
        if (m_repo->cachedSpec(p.id, &spec)) {
            row[QStringLiteral("tools")] =
                spec.hasToolAllowlist ? spec.toolAllowlist : QStringList{};
            row[QStringLiteral("baseUrl")] = spec.hasBaseUrl ? spec.baseUrl : QString();
            row[QStringLiteral("contextEngine")] = spec.contextEngine;
            row[QStringLiteral("memoryProvider")] = spec.memoryProvider;
            row[QStringLiteral("credentialRef")] =
                spec.hasCredentialRef ? spec.credentialRef : QString();
            // Foreign-engine binding (wire v29): "Core" for native profiles; "Foreign" + the
            // catalog agent name for foreign ones (so the UI can label/hide inference-only
            // affordances). The agent's protocol (ACP vs stream-json) is a catalog-entry property.
            // NB: the presentation row key stays `acpAgent` (the foreign agent name) — it is the
            // contract read by the app-delegation fleet surfaces (the tree row's engineAgent key,
            // mirror_fleet_tree.cpp), which this stream must not touch. Only the engine VALUE
            // moves "Acp" -> "Foreign".
            row[QStringLiteral("engine")] = spec.engineKind;
            row[QStringLiteral("acpAgent")] = spec.engineForeignAgent;
            // Foreign model backend (wire v30), projected as a compact map the AgentSetupModel
            // editor + surfaces consume: {mode, provider (NodeProvider wire selector), model
            // (NodeProvider model / AgentNative hint), credentialRef}. Empty for a Core profile or
            // a pre-v30 encoding that omitted it (the node defaults absent -> AgentNative).
            row[QStringLiteral("foreignBackend")] =
                spec.hasForeignBackend ? foreignBackendRow(spec.foreignBackend) : QVariantMap{};
        } else {
            row[QStringLiteral("tools")] = QStringList{};
            row[QStringLiteral("engine")] = QStringLiteral("Core");
            row[QStringLiteral("acpAgent")] = QString();
            row[QStringLiteral("foreignBackend")] = QVariantMap{};
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
