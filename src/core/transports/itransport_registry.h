// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

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

    // Live conversations/rooms for a transport (EIO-8). Each entry is a map:
    //   transport, id, kind, title, topic. `conversations()` returns the last-known (cached) set;
    //   `refreshConversations()` triggers a live enumeration (ConvList) and fires
    //   conversationsChanged when it lands. Default no-ops so the mock + non-daemon seams need not
    //   implement them.
    [[nodiscard]] Q_INVOKABLE virtual QVariantList conversations(const QString& transport) const {
        Q_UNUSED(transport)
        return {};
    }
    Q_INVOKABLE virtual void refreshConversations(const QString& transport) { Q_UNUSED(transport) }

    // [wave2:app-channels-liveness] B2: presentation-only "new room" affordance. A conversation
    // that surfaced (via ConvList) after the operator's baseline for its transport — e.g. an
    // auto-accepted invite the node joined — reads back true until viewed. Membership itself is the
    // node's; this only tracks what the operator has already seen. Default no-op/false so the mock
    // and non-daemon seams need not implement it.
    [[nodiscard]] Q_INVOKABLE virtual bool isNewConversation(const QString& transport,
                                                             const QString& conversation) const {
        Q_UNUSED(transport)
        Q_UNUSED(conversation)
        return false;
    }
    Q_INVOKABLE virtual void markConversationSeen(const QString& transport,
                                                  const QString& conversation) {
        Q_UNUSED(transport)
        Q_UNUSED(conversation)
    }

signals:
    void adaptersChanged();
    void instancesChanged();
    void conversationsChanged(const QString& transport);
};

} // namespace transports
