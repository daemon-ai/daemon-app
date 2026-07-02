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
// profile bundles a model, system prompt, and the enabled skills/tools an agent
// runs with. The mock stores profiles in memory; a daemon adapter later maps
// these to the node's profile endpoints. Profiles are a live VariantListModel
// (rows: id, name, model, description, systemPrompt, skills[], tools[], isDefault).
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
    void exportSaved(const QString& fileUrl);
    void imported(const QString& newId);
    void historyChanged(const QString& id);
    void historyAvailableChanged();
    void reverted(const QString& id);
    void profileOpFailed(const QString& message);
};

} // namespace profiles
