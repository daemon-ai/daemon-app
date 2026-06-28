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

    // The profile display names, for QML dropdowns (e.g. the cron job profile
    // picker) that want a plain string list rather than the row model.
    [[nodiscard]] Q_INVOKABLE virtual QStringList profileNames() const = 0;

    // The skill / tool catalogs the curator offers (id, name, description).
    [[nodiscard]] Q_INVOKABLE virtual QVariantList availableSkills() const = 0;
    [[nodiscard]] Q_INVOKABLE virtual QVariantList availableTools() const = 0;

    Q_INVOKABLE virtual QString createProfile(const QString& name) = 0;
    // Clone an existing profile under a new id (a copy, not a live link); returns the new id.
    Q_INVOKABLE virtual QString cloneProfile(const QString& source, const QString& newId) = 0;
    // Patch a profile with the given fields (only present keys are updated).
    Q_INVOKABLE virtual void updateProfile(const QString& id, const QVariantMap& fields) = 0;
    Q_INVOKABLE virtual void remove(const QString& id) = 0;
    Q_INVOKABLE virtual void setDefault(const QString& id) = 0;

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
