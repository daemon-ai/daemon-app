#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

namespace transports {

// Transport-adapter registry seam (daemon-transport-adapter-spec.md): the client-side view of the
// node's events-IO transport adapters (Matrix, the internal Rooms loopback, HTTP, a future A2A).
// `availableAdapters()` backs the multi-protocol "Add channel" picker (family, display name,
// capability flags, account-setup schema) — the Pidgin/Kopete/Adium service-picker pattern;
// `instances()` lists the configured accounts the unified conversation roster groups by. A daemon
// adapter later decodes the node's `transport_adapters` / `transport_instances` ops over the wire
// and emits the same rows; the UI never sees the codec. The mock returns canned data so the surface
// can be built UI-first.
//
// Skeleton: the live per-account connection/presence state is owned by IPresenceService (sibling
// seam); this registry carries only identity + capabilities + binding.
class ITransportRegistry : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    ~ITransportRegistry() override = default;

    // Available adapter families for the "Add channel" picker. Each entry is a map:
    //   family (QString), displayName (QString), capabilities (QVariantMap of bool flags),
    //   schema (QVariantList of field maps: key, label, required).
    [[nodiscard]] Q_INVOKABLE virtual QVariantList availableAdapters() const = 0;

    // Configured transport instances (accounts). Each entry is a map:
    //   transport (QString), family (QString), displayName (QString), boundProfile (QString).
    [[nodiscard]] Q_INVOKABLE virtual QVariantList instances() const = 0;

signals:
    void adaptersChanged();
    void instancesChanged();
};

} // namespace transports
