#pragma once

#include <QObject>
#include <QString>
#include <QVariant>

namespace settings {

// Shared client-local preference store seam. App preferences (theme target,
// setupComplete, last connection) persist locally and work offline; both front
// ends bind one instance. A QSettings-backed implementation is used now; the
// daemon never owns these (it owns node-config, read over the wire instead).
//
// The store is intentionally a thin typed key/value bag plus a few named
// convenience accessors for the first-run / connection keys the gate needs, so a
// new section can persist a value without growing the interface.
class ISettingsStore : public QObject {
    Q_OBJECT
    // The first-run gate flips this once setup finishes; the shell mounts
    // straight to the main view on subsequent launches.
    Q_PROPERTY(bool setupComplete READ setupComplete WRITE setSetupComplete NOTIFY changedAny)

public:
    using QObject::QObject;
    ~ISettingsStore() override = default;

    [[nodiscard]] Q_INVOKABLE virtual QVariant value(const QString& key,
                                                     const QVariant& fallback = {}) const = 0;
    Q_INVOKABLE virtual void setValue(const QString& key, const QVariant& value) = 0;

    // Named convenience over the raw keys (so QML/TUI never hardcode key strings).
    [[nodiscard]] bool setupComplete() const
    {
        return value(QStringLiteral("app/setupComplete"), false).toBool();
    }
    void setSetupComplete(bool on) { setValue(QStringLiteral("app/setupComplete"), on); }

    [[nodiscard]] Q_INVOKABLE QString lastConnectionMode() const
    {
        return value(QStringLiteral("conn/mode"), QStringLiteral("local")).toString();
    }
    [[nodiscard]] Q_INVOKABLE QString lastConnectionTarget() const
    {
        return value(QStringLiteral("conn/target"), QString()).toString();
    }
    Q_INVOKABLE void setLastConnection(const QString& mode, const QString& target)
    {
        setValue(QStringLiteral("conn/mode"), mode);
        setValue(QStringLiteral("conn/target"), target);
    }

signals:
    // Emitted on every write (carries the key), plus a no-arg overload so QML
    // property bindings (setupComplete) refresh without naming the key.
    void changed(const QString& key);
    void changedAny();
};

} // namespace settings
