// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QMetaType>
#include <QString>
#include <QVariantMap>

namespace connection {

// The active connection target. `mode` is "embedded" | "local" | "remote" |
// "remote-ws" (embedded is gated until full-node FFI lands). `target` is a
// socket path (local) or URL (remote/remote-ws). `token` is an optional bearer
// for remote. This is the shape the picker writes and the settings store
// persists.
struct ConnectionConfig {
#if defined(Q_OS_WASM) || defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    // The browser, Android and iOS builds have exactly one usable transport - the WebSocket mux -
    // so the picker's initial selection (bound via IConnectionService::mode) starts there.
    QString mode = QStringLiteral("remote-ws");
#else
    QString mode = QStringLiteral("local");
#endif
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
