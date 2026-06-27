#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
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

signals:
    void changed();
};

} // namespace profiles
