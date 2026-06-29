// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QMetaType>
#include <QString>
#include <QVariantMap>

namespace connection {

// The active connection target. `mode` is "embedded" | "local" | "remote"
// (matching the audit's three transports; embedded is gated until full-node FFI
// lands). `target` is a socket path (local) or URL (remote). `token` is an
// optional bearer for remote. This is the shape the picker writes and the
// settings store persists.
struct ConnectionConfig {
    QString mode = QStringLiteral("local");
    QString target;
    QString token;

    [[nodiscard]] QVariantMap toVariantMap() const {
        return {
            {QStringLiteral("mode"), mode},
            {QStringLiteral("target"), target},
            {QStringLiteral("token"), token},
        };
    }
};

} // namespace connection

Q_DECLARE_METATYPE(connection::ConnectionConfig)
