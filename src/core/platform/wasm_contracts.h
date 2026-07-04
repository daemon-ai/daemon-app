// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

// Single source of truth for the string contracts shared across the four Browser Persistence
// Polish streams (W1-W8). These literals are pinned by the pre-fork contract commit so the
// sentinel *emitters* (Stream B) and the e2e *consumers* (Stream D) agree byte-for-byte without
// coordinating on the exact spelling. Only the values C++ actually reads/prints live here;
// shell-only (JS) and justfile-only strings are documented as comments below, since a C++ header
// cannot be a source of truth for them anyway.

namespace platform {

// --- Console/stdout sentinels (printed via std::fprintf(stdout, ...); stdout reaches the browser
// console on wasm). Consumed by the CDP harness (Stream D / W6). ---

// Silent re-auth succeeded from a persisted resume token (AuthOk with no AuthChallenge). W3/W6.
inline constexpr char kSentinelAuthResumed[] = "DAEMON_APP_AUTH resumed";
// Fresh SASL SCRAM login (an AuthChallenge was seen before AuthOk). W3/W6.
inline constexpr char kSentinelAuthScram[] = "DAEMON_APP_AUTH scram";
// First-run gate reached its "done" phase (async equivalent of settleFirstRunGate). W3/W6.
inline constexpr char kSentinelFirstRunDone[] = "DAEMON_APP_FIRSTRUN done";
// Cache row count at boot, before any network fetch (proves IDBFS survived a reload). The decimal
// row count is substituted at runtime, e.g. std::fprintf(stdout, "%s%d\n",
// kSentinelCacheRowsPrefix, n) yields "DAEMON_APP_CACHE rows=42". W2/W6.
inline constexpr char kSentinelCacheRowsPrefix[] = "DAEMON_APP_CACHE rows=";

// --- Automation env hook (Stream B / W3). ---

// Guarded login hook for the reload-survival harness: value is "user:pass", honored ONLY when
// DAEMON_APP_WAIT_READY is also set (keeps it out of production paths). Drives
// DaemonConnectionService::login() when the state machine hits "authenticating".
inline constexpr char kEnvLogin[] = "DAEMON_APP_LOGIN";

// --- Workspace uploads (Stream C / W7). ---

// Browser file attachments are uploaded into the node workspace under this directory before an
// @file:/@image: chip is created. Full template: "uploads/<utc-timestamp>-<sanitized-name>"
// (timestamp + sanitized original filename substituted at runtime).
inline constexpr char kUploadsDirPrefix[] = "uploads/";

// --- Documentary only (NOT C++ constants; here so the one file lists every pinned string) ---
// Shell escape hatch (handled in daemon-app.html JS, W4): the URL query param "?clear=1" wipes
//   origin storage (IndexedDB + localStorage) before qtLoad and reloads.
// Superproject justfile recipes (applied at integration, W6): "wasm-smoke" and "e2e-web".

} // namespace platform
