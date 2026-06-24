#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

namespace session {

// Per-session override seam backing the composer's session-settings popover. It
// represents the active session's effective model/effort/profile overrides;
// the mock holds them in memory. A real implementation binds these to the live
// ComposerSessionController for the focused tab.
class ISessionSettings : public QObject {
    Q_OBJECT
    // The session these overrides apply to. Setting it switches the active
    // per-session state (so the composer popover reflects the focused chat,
    // not a single global), emitting changed() so bound views refresh.
    Q_PROPERTY(int sessionId READ sessionId WRITE setSessionId NOTIFY
                   sessionIdChanged)
    Q_PROPERTY(QString profile READ profile WRITE setProfile NOTIFY changed)
    Q_PROPERTY(QString effort READ effort WRITE setEffort NOTIFY changed)
    Q_PROPERTY(bool fast READ fast WRITE setFast NOTIFY changed)
    Q_PROPERTY(bool verbose READ verbose WRITE setVerbose NOTIFY changed)

public:
    using QObject::QObject;
    ~ISessionSettings() override = default;

    [[nodiscard]] virtual int sessionId() const = 0;
    virtual void setSessionId(int id) = 0;

    [[nodiscard]] virtual QString profile() const = 0;
    virtual void setProfile(const QString& profile) = 0;
    [[nodiscard]] virtual QString effort() const = 0;
    virtual void setEffort(const QString& effort) = 0;
    [[nodiscard]] virtual bool fast() const = 0;
    virtual void setFast(bool on) = 0;
    [[nodiscard]] virtual bool verbose() const = 0;
    virtual void setVerbose(bool on) = 0;

    [[nodiscard]] Q_INVOKABLE virtual QStringList effortOptions() const = 0;

signals:
    void changed();
    void sessionIdChanged();
};

} // namespace session
