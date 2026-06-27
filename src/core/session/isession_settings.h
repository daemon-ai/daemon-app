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
    Q_PROPERTY(QString sessionId READ sessionId WRITE setSessionId NOTIFY sessionIdChanged)
    Q_PROPERTY(QString profile READ profile WRITE setProfile NOTIFY changed)
    Q_PROPERTY(QString effort READ effort WRITE setEffort NOTIFY changed)
    Q_PROPERTY(bool fast READ fast WRITE setFast NOTIFY changed)
    Q_PROPERTY(bool verbose READ verbose WRITE setVerbose NOTIFY changed)
    // The per-session approval mode (CHA-4): "ask" | "accept_edits" | "auto_allow" | "deny".
    // Default "ask" (interactive). In daemon mode the setter sends SetSessionMode so the node's
    // SessionOverlay.approval_mode tracks it; the mock holds it in memory.
    Q_PROPERTY(QString approvalMode READ approvalMode WRITE setApprovalMode NOTIFY changed)

public:
    using QObject::QObject;
    ~ISessionSettings() override = default;

    [[nodiscard]] virtual QString sessionId() const = 0;
    virtual void setSessionId(const QString& id) = 0;

    [[nodiscard]] virtual QString profile() const = 0;
    virtual void setProfile(const QString& profile) = 0;
    [[nodiscard]] virtual QString effort() const = 0;
    virtual void setEffort(const QString& effort) = 0;
    [[nodiscard]] virtual bool fast() const = 0;
    virtual void setFast(bool on) = 0;
    [[nodiscard]] virtual bool verbose() const = 0;
    virtual void setVerbose(bool on) = 0;
    [[nodiscard]] virtual QString approvalMode() const = 0;
    virtual void setApprovalMode(const QString& mode) = 0;

    [[nodiscard]] Q_INVOKABLE virtual QStringList effortOptions() const = 0;
    // The selectable approval modes for the popover (CHA-4).
    [[nodiscard]] Q_INVOKABLE virtual QStringList approvalModeOptions() const = 0;

signals:
    void changed();
    void sessionIdChanged();
};

} // namespace session
