// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

namespace profiles {

// Profiles/agents seam backing the profile editor + Skills/tools curator. A
// profile bundles a model, persona, and the enabled skills/tools an agent
// runs with. The mock stores profiles in memory; a daemon adapter later maps
// these to the node's profile endpoints. Profiles are a live VariantListModel
// (rows: id, name, model, description, systemPrompt, skills[], tools[], isDefault).
// The persona (SOUL.md) is read/written through the soul()/requestSoul()/setSoul()
// surface below — never as a profile-spec field from UI code.
class IProfileStore : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* profiles READ profiles CONSTANT)
    Q_PROPERTY(QString defaultProfileId READ defaultProfileId NOTIFY changed)
    // PRO-8: whether the connected daemon hosts a revision log (versioning needs a durable daemon).
    // Lets the UI hide the History/Revert affordances when versioning is unavailable.
    Q_PROPERTY(bool historyAvailable READ historyAvailable NOTIFY historyAvailableChanged)

public:
    using QObject::QObject;
    ~IProfileStore() override = default;

    [[nodiscard]] virtual QObject* profiles() const = 0;
    [[nodiscard]] virtual QString defaultProfileId() const = 0;

    [[nodiscard]] Q_INVOKABLE virtual QVariantMap profile(const QString& id) const = 0;

    // Persona (SOUL.md) surface. A persona is a node-owned per-profile SOUL.md document,
    // Core-engine only — a foreign profile's prompt belongs to the foreign agent, so the UI
    // hides this surface and the node answers writes with a typed rejection (surfaced via
    // profileOpFailed()). `soul()` returns the last-known persona synchronously;
    // `requestSoul()` asks the seam to (re)fetch it; `setSoul()` persists a new persona.
    // A store emits soulChanged(profileId) whenever soul(profileId) has fresh authoritative
    // content. The defaults ride an in-memory profile-row `systemPrompt` key (a mock-store
    // convention only — the wire ProfileSpec carries no system prompt at v36; the persona is a
    // node-owned SOUL.md document), so in-memory stores work unchanged; the daemon store overrides
    // all three with the SoulGet/SoulSet wire ops.
    [[nodiscard]] Q_INVOKABLE virtual QString soul(const QString& profileId) const {
        return profile(profileId).value(QStringLiteral("systemPrompt")).toString();
    }
    Q_INVOKABLE virtual void requestSoul(const QString& profileId) { Q_UNUSED(profileId) }
    Q_INVOKABLE virtual void setSoul(const QString& profileId, const QString& text) {
        updateProfile(profileId, {{QStringLiteral("systemPrompt"), text}});
    }

    // Resolve a profile selector to the canonical profile id the node resolves (the ref used by
    // ProfileCreate/ProfileGet and Submit.profile). The session-settings UI stores a display name;
    // the turn wiring calls this so the value that reaches the wire is an id, not a label. An empty
    // selector stays empty (= the node's active profile); an already-valid id passes through; an
    // unresolved selector is forwarded verbatim as a residual. Default: pass-through (stores that
    // do not distinguish id/name are unaffected).
    [[nodiscard]] Q_INVOKABLE virtual QString resolveProfileRef(const QString& selector) const {
        return selector;
    }

    // The profile display names, for QML dropdowns (e.g. the cron job profile
    // picker) that want a plain string list rather than the row model.
    [[nodiscard]] Q_INVOKABLE virtual QStringList profileNames() const = 0;

    // The skill / tool catalogs the curator offers (id, name, description).
    [[nodiscard]] Q_INVOKABLE virtual QVariantList availableSkills() const = 0;
    [[nodiscard]] Q_INVOKABLE virtual QVariantList availableTools() const = 0;

    // The REAL model providers the ProfileEditor picker offers (rows: id = ProviderSelector wire
    // string, name = display label, cloud = takes an optional base URL). Mock is deliberately
    // excluded: it is never a default or a user choice (reachable only under
    // DAEMON_APP_SERVICE_MODE=mock), so it must never appear here. Concrete + shared by every store
    // so the invariant holds in one place.
    [[nodiscard]] Q_INVOKABLE QVariantList pickerProviders() const {
        const auto mk = [](const QString& id, const QString& name, bool cloud) {
            QVariantMap m;
            m[QStringLiteral("id")] = id;
            m[QStringLiteral("name")] = name;
            m[QStringLiteral("cloud")] = cloud;
            return QVariant(m);
        };
        return {
            mk(QStringLiteral("daemon_api"), QStringLiteral("Daemon API"), true),
            mk(QStringLiteral("genai"), QStringLiteral("genai"), true),
            mk(QStringLiteral("llama_cpp"), QStringLiteral("Llama.cpp"), false),
            mk(QStringLiteral("mistral_rs"), QStringLiteral("Mistral.rs"), false),
        };
    }

    Q_INVOKABLE virtual QString createProfile(const QString& name) = 0;
    // Create a profile under `name` with an explicit initial spec (`fields` as in updateProfile:
    // provider/model/baseUrl/...). Default: compose create + update (fine for synchronous
    // stores); the daemon store overrides with a SINGLE full-spec ProfileCreate so the two ops
    // cannot race node-side. Returns the new id.
    Q_INVOKABLE virtual QString createProfileWithSpec(const QString& name,
                                                      const QVariantMap& fields) {
        const QString id = createProfile(name);
        if (!id.isEmpty() && !fields.isEmpty()) {
            updateProfile(id, fields);
        }
        return id;
    }
    // Foreign-engine create (wire v29): a named ProfileCreate carrying `engine = Foreign{agent}`,
    // where `agent` is a catalog NAME — launch recipes stay node-side (operator-managed), so the
    // client only ever sends the reference. No provider/model/key applies to a foreign agent; the
    // node validates that the agent exists in its catalog and is installed. Returns the new profile
    // id, or "" when this seam cannot create foreign agents (the mock default).
    Q_INVOKABLE virtual QString createForeignProfile(const QString& name, const QString& agent) {
        Q_UNUSED(name)
        Q_UNUSED(agent)
        return {};
    }
    // Foreign-engine create carrying an explicit foreign-backend (wire v30). `backend` is the
    // AgentSetupModel projection: {mode: "AgentNative"|"NodeProvider", provider (NodeProvider wire
    // selector), model (NodeProvider model, or the AgentNative steer hint), credentialRef
    // (NodeProvider, optional)}. Default: ignore the backend and fall back to createForeignProfile
    // (a store without foreign-backend support persists the plain foreign reference — the node
    // defaults an absent backend to AgentNative{model:null}); the daemon store overrides it to set
    // ProfileSpec.foreign_backend. Returns the new profile id, or "" when unsupported.
    Q_INVOKABLE virtual QString createForeignProfileWithBackend(const QString& name,
                                                                const QString& agent,
                                                                const QVariantMap& backend) {
        Q_UNUSED(backend)
        return createForeignProfile(name, agent);
    }
    // Clone an existing profile under a new id (a copy, not a live link); returns the new id.
    Q_INVOKABLE virtual QString cloneProfile(const QString& source, const QString& newId) = 0;
    // Patch a profile with the given fields (only present keys are updated).
    Q_INVOKABLE virtual void updateProfile(const QString& id, const QVariantMap& fields) = 0;
    Q_INVOKABLE virtual void remove(const QString& id) = 0;
    Q_INVOKABLE virtual void setDefault(const QString& id) = 0;

    // Ask the seam to re-fetch its authoritative state; `changed()` fires when the fresh state
    // is reflected. Default: an in-memory store's state is already current — re-announce it
    // synchronously. The daemon store overrides with a ProfileList round-trip, so callers can
    // sequence work on a FRESH node reflection instead of a possibly-stale snapshot.
    Q_INVOKABLE virtual void refresh() { emit changed(); }

    // PRO-7: export a profile to (import a profile distribution from) a user-chosen file. The
    // artifact is the portable Distribution (opaque CBOR). Default impls are no-ops so the mock /
    // non-daemon seams need not implement them; the daemon seam wires ProfileExport/Import.
    Q_INVOKABLE virtual void exportProfileToFile(const QString& id, const QUrl& fileUrl) {
        Q_UNUSED(id)
        Q_UNUSED(fileUrl)
    }
    Q_INVOKABLE virtual void importProfileFromFile(const QUrl& fileUrl,
                                                   const QString& newId = QString()) {
        Q_UNUSED(fileUrl)
        Q_UNUSED(newId)
    }
    // PRO-8: a profile's revision history (ProfileHistory) projected into a VariantListModel (rows:
    // seq, parent, author, reason, ts), refreshed by requestHistory(id); revert rolls forward.
    [[nodiscard]] Q_INVOKABLE virtual QObject* history() const { return nullptr; }
    // Whether the daemon hosts versioning. Default false: seams without a revision log (the mock,
    // a non-durable daemon) have no history, so the UI hides the panel.
    [[nodiscard]] virtual bool historyAvailable() const { return false; }
    Q_INVOKABLE virtual void requestHistory(const QString& id) { Q_UNUSED(id) }
    Q_INVOKABLE virtual void revertProfile(const QString& id, double seq) {
        Q_UNUSED(id)
        Q_UNUSED(seq)
    }

signals:
    void changed();
    // The persona for `profileId` freshened (a setSoul landed / a SoulGet answered):
    // soul(profileId) now returns current, authoritative content.
    void soulChanged(const QString& profileId);
    void exportSaved(const QString& fileUrl);
    void imported(const QString& newId);
    void historyChanged(const QString& id);
    void historyAvailableChanged();
    void reverted(const QString& id);
    void profileOpFailed(const QString& message);
};

} // namespace profiles
