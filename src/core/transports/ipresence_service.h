#pragma once

#include <QObject>
#include <QString>

namespace transports {

// Per-account connection/presence seam (daemon-transport-adapter-spec.md §3.3): the live status the
// roster status dots and the account-status bar bind to. `connectionState` is the Pidgin-style
// connection state machine ("offline" | "connecting" | "connected" | "error"); `presence` is the
// normalized presence primitive ("unknown" | "offline" | "available" | "idle" | "away" | "busy").
// A daemon adapter later decodes per-instance state from the node's `transport_instances` op and
// emits presenceChanged; the mock reports inert defaults. Closes the per-account connection-state
// gap (EIO-9) on the client side.
class IPresenceService : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    ~IPresenceService() override = default;

    // The live connection state of a transport instance ("offline" when unknown).
    [[nodiscard]] Q_INVOKABLE virtual QString connectionState(const QString& transport) const = 0;
    // The reported presence of a transport instance ("unknown" when not reported).
    [[nodiscard]] Q_INVOKABLE virtual QString presence(const QString& transport) const = 0;

signals:
    // Emitted when a transport instance's connection/presence changes.
    void presenceChanged(const QString& transport);
};

} // namespace transports
