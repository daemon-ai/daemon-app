// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "transports/mock_transport_registry.h"

#include <QVariantMap>

namespace transports {

namespace {

QVariantMap adapterRow(const QString& family, const QString& displayName, bool rooms, bool dms,
                       bool presence, bool interactiveAuth) {
    QVariantMap caps;
    caps.insert(QStringLiteral("rooms"), rooms);
    caps.insert(QStringLiteral("directMessages"), dms);
    caps.insert(QStringLiteral("presence"), presence);
    caps.insert(QStringLiteral("interactiveAuth"), interactiveAuth);

    QVariantMap row;
    row.insert(QStringLiteral("family"), family);
    row.insert(QStringLiteral("displayName"), displayName);
    row.insert(QStringLiteral("capabilities"), caps);
    row.insert(QStringLiteral("schema"), QVariantList{});
    return row;
}

} // namespace

QVariantList MockTransportRegistry::availableAdapters() const {
    return QVariantList{
        adapterRow(QStringLiteral("matrix"), QStringLiteral("Matrix"), true, true, true, true),
        adapterRow(QStringLiteral("room"), QStringLiteral("Rooms (internal)"), true, true, false,
                   false),
    };
}

QVariantList MockTransportRegistry::instances() const {
    return QVariantList{};
}

} // namespace transports
