<!--
SPDX-License-Identifier: MPL-2.0
SPDX-FileCopyrightText: 2026 Jarrad Hope
-->

# Browser token persistence (wasm)

How the WebAssembly build keeps you signed in across page reloads, and the security
trade-off that comes with it.

## What is stored, and where

On a successful SASL handshake the node issues an opaque **session resume token**. The
client persists it per connection target so a reload (or a transport drop) can re-authenticate
silently instead of prompting for credentials again:

- The token is written by `DaemonConnectionService` on `tokenIssued`, keyed by
  `conn/tokens/<sha256(target)[:16]>` in the settings store.
- On the browser build the settings store is Qt's default wasm backend
  (`WebLocalStorageFormat`), i.e. the origin's `localStorage`. There is **no OS keychain** on
  wasm, so the token is held as **plaintext in origin-scoped browser storage**.
- Only the server-issued token is ever stored — never the password.

## Target-key stability

The token key hashes the connection target string. For a reload to find the token the app wrote
at login, the target must be byte-identical across loads. The browser target is derived by
`settings::deriveWsDefault()` (the `?ws=` page override, else a saved ws-shaped target, else the
page-origin `ws(s)://<host[:port]>/ws` default). To keep the hash stable, every ws target is run
through `settings::normalizeWsTarget()` — lowercase scheme + host, drop the redundant default port
(`ws`→80, `wss`→443), strip trailing slashes — both when the target is persisted and when it is
resolved on a fresh load. Normalization is applied in `deriveWsDefault()` and again in
`DaemonConnectionService::connectTo()` for `remote-ws`, so the picker path and the derived-default
path key the same token.

## Lifetime and revocation

- The server enforces a **7-day TTL** from the token's mint time; an expired token is rejected and
  the client falls back to an interactive SCRAM login.
- **Sign out** (Settings → Connection → *Sign out*, calling `Connection.logout()`) clears the
  stored token for the active target and disconnects. This is distinct from *Disconnect*, which
  drops the link but keeps the token so the next connect re-auths silently.
- A rejected/stale token is cleared automatically so it can't wedge future logins.
- **Clear local data** (W4) wipes the token along with the rest of this device's client-local
  state.

## Threat model

Because the token is plaintext in `localStorage`, anyone (or any script) with access to the
origin's storage — a shared/unlocked machine, a malicious extension, or a successful XSS against
the served bundle — can read it and impersonate the session until it expires or is signed out.
Treat a browser deployment's origin storage as roughly as sensitive as a logged-in session cookie:

- Serve the bundle from a **trusted, single origin** over **TLS** (`wss://`), and keep the origin
  free of untrusted third-party script.
- On shared machines, **Sign out** (or use *Clear local data*) when finished rather than only
  closing the tab.
- The 7-day server TTL bounds the exposure window for a leaked token.

The desktop build differs: there a keychain-backed store may hold the token instead of plaintext
settings. This document covers the browser build specifically.
