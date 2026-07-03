// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "settings/ws_default.h"

#include <QUrlQuery>

namespace settings {

namespace {

// Mirrors DaemonConnectionService::parseWsUrl: remote-ws targets are FULL URLs (the scheme
// decides plaintext vs TLS, so it must be explicit) with a non-empty host.
bool wsShaped(const QString& target) {
    const QUrl url(target.trimmed(), QUrl::StrictMode);
    const bool wsScheme =
        url.scheme() == QLatin1String("ws") || url.scheme() == QLatin1String("wss");
    return url.isValid() && wsScheme && !url.host().isEmpty();
}

} // namespace

QString deriveWsDefault(const QUrl& pageUrl, const QString& savedTarget) {
    // 1. Explicit `?ws=` override in the address bar. A malformed value (wrong scheme, no host)
    //    is ignored rather than surfaced: the prefill falls back to the next tier and the
    //    picker's Test button explains the expected shape.
    const QUrlQuery query(pageUrl);
    QString queryOverride =
        query.queryItemValue(QStringLiteral("ws"), QUrl::FullyDecoded).trimmed();
    if (wsShaped(queryOverride)) {
        return queryOverride;
    }
    // 2. The persisted target, but only when ws-shaped (desktop parity: saved beats the platform
    //    default). A saved socket path or host:port must never prefill the browser picker.
    if (wsShaped(savedTarget)) {
        return savedTarget.trimmed();
    }
    // 3. Page-origin default for a single-origin deployment: the daemon that serves the bundle
    //    serves the mux WebSocket at path `/ws` on the same listener, so keep host AND port and
    //    map the scheme (http -> ws, https -> wss; the default ports line up pairwise).
    const QString scheme = pageUrl.scheme();
    if (scheme != QLatin1String("http") && scheme != QLatin1String("https")) {
        return {};
    }
    QUrl ws;
    ws.setScheme(scheme == QLatin1String("https") ? QStringLiteral("wss") : QStringLiteral("ws"));
    ws.setHost(pageUrl.host());
    ws.setPort(pageUrl.port());
    ws.setPath(QStringLiteral("/ws"));
    return ws.toString();
}

} // namespace settings
