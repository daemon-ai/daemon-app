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
// DaemonConnectionService::parseWsUrl: a FULL ws:// or wss:// URL with a host. The result is run
// through normalizeWsTarget() so a derived target is already canonical.
[[nodiscard]] QString deriveWsDefault(const QUrl& pageUrl, const QString& savedTarget = {});

// Canonicalize a ws:// / wss:// connection target so the per-target resume-token key
// (SHA256(target)[:16]) is byte-stable across a page reload: lowercase the scheme and host, drop
// a redundant default port (ws -> 80, wss -> 443), and strip trailing slashes from the path
// ("ws://h/" -> "ws://h", "ws://h/ws/" -> "ws://h/ws"). Applied both when the target is persisted
// (at login) and when it is resolved (on a fresh load), so both sides hash identically. A target
// that is not ws-shaped (a socket path, host:port, empty) is returned trimmed and otherwise
// unchanged, so this is safe to call on any target.
[[nodiscard]] QString normalizeWsTarget(const QString& target);

} // namespace settings
