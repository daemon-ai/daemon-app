// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QString>
#include <QUrl>

namespace settings {

// Derives the browser build's remote-ws connection target from the page URL. Pure (no
// emscripten), so the precedence and URL shapes are unit-tested on desktop; the wasm-only
// wasm_page_origin.cpp feeds it window.location.href at runtime.
//
// Precedence:
//  1. `?ws=<url>` page-query override - an explicit instruction in the address bar, so it wins
//     over the persisted target (the browser analog of the DAEMON_APP_SOCKET env override).
//  2. `savedTarget`, but only when it is ws-shaped - a persisted socket path or host:port is
//     unusable in a browser and must not leak into the picker prefill.
//  3. The single-origin default `ws://<host[:port]>/ws` (`wss://` on https pages): the daemon
//     that serves this bundle also serves the CBOR mux WebSocket at path `/ws` on the SAME
//     listener, so a plain page load prefills correctly with zero typing.
//
// Returns empty when nothing applies (non-http(s) page with no override), letting the caller
// fall through to its platform fallback. Accepted override/saved shapes mirror
// DaemonConnectionService::parseWsUrl: a FULL ws:// or wss:// URL with a host.
[[nodiscard]] QString deriveWsDefault(const QUrl& pageUrl, const QString& savedTarget = {});

} // namespace settings
