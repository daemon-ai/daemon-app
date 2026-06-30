# Auth 6 — daemon-app clients (Track E) — implementation plan

Status: PHASE 1 (design). No source changed yet. Awaiting coordinator validation before implementing.

Worktree: `/home/j/experiments/daemon-worktrees/auth6-clients` (branch `feature/auth6-clients`,
a worktree of the `daemon-app` Qt6 C++/QML + TUI repo). All work stays in this worktree; never touch
`/home/j/experiments/daemon/daemon-app` or other worktrees. Every command runs in the devShell:
`nix develop --command <cmd>` (no host tools).

This track makes the GUI + TUI clients speak the **frozen v2 wire contract**: SASL authentication in
the L0 envelope, TLS for non-local transports, a login UI, server-token persistence, and
capability-gated surfaces with per-user cache isolation. The contract is frozen on the node side
(good); end-to-end verification waits on Auth 3 (server authn/transport integration) and the admin
page's data path waits on Auth 5 (`access-admin-api`). See **Cross-track dependencies** at the end.

---

## 0. Required gates (run before calling anything done)

From `AGENTS.md` (daemon-app), all in the devShell:

- Build:        `nix develop --command cmake -B build -G Ninja -DBUILD_TESTING=ON -DDAEMON_APP_TUI=ON && nix develop --command cmake --build build`
- clang-format: `nix develop --command bash -c "git ls-files 'src/*.cpp' 'src/*.h' 'tests/*.cpp' 'tests/*.h' | xargs clang-format --dry-run --Werror"`
- clang-tidy:   `nix develop --command bash -c "git ls-files 'src/*.cpp' | xargs -P\"$(nproc)\" -n1 clang-tidy -p build --quiet"`
- qmllint:      `nix develop --command cmake --build build --target all_qmllint`
- Tests:        `nix develop --command bash -c "QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure"`

Do NOT hand-edit `src/core/daemon/codec/{generated,vendor}` — those regenerate from the superproject
contract (`just update-codec`). The L0 envelope (incl. the new auth frames) is hand-coded and lives
outside the generated codec, so it is ours to edit. (Confirmed: the vendored generated codec is
already at v2 — `api_error_unauthenticated_m_c` / `api_error_forbidden_m_c` exist in
`codec/generated/daemon_api_client_types.h`, so no codec regeneration is needed for this track.)

---

## 1. Current-state map (what exists today)

- **Envelope (hand-coded), `src/core/daemon/node_api_codec/wire_l0.cpp` + `node_api_codec.h`:**
  `kWireVersion = 1`. Encoders: `encodeHelloFrame`/`Call`/`Open`/`Cancel`. Decoder `decodeWireFrame`
  handles `Hello`/`Reply`/`Item`/`End`/`Reset` into `DecodedWireFrame { kind, id, payload, hasError,
  epoch, headSeq, features }`. `WireFrameKind` has no auth kinds. CBOR primitives live in
  `node_api_codec/mappers.cpp` (`cborAppendUint`, `cborAppendText` [short <24B only], `cborReadHead`,
  `cborReadText`) and are declared in `node_api_codec/internal.h`.
- **Handshake / client, `src/core/daemon/node_api_client.{h,cpp}`:** state machine
  `Disconnected -> Handshaking -> Ready`. `enqueue()` buffers frames into `m_outbox` and calls
  `ensureHandshake()` (sends Hello). On the server `Hello` it goes `Ready`, records `m_features`, and
  `flushOutbox()`. **This is the existing outbox gate we extend** — today nothing is sent before the
  Hello ack; we add a step so nothing is sent before `AuthOk` when the server requires auth.
- **Transport, `src/core/daemon/daemon_transport.{h,cpp}`:** `QLocalSocket` only (Unix), length-
  prefixed (big-endian u32 + CBOR). No TLS, no TCP.
- **Connection service, `src/core/daemon/daemon_connection_service.cpp`:**
  `connectTo(mode,target,token)` has `Q_UNUSED(token)` and only supports `mode=="local"` (anything
  else -> `"needs setup"`). `testConnection` likewise. States: `checking|connecting|ready|offline|
  needs setup` (see `connection/iconnection_service.h`). It owns the transport + `NodeApiClient`
  (`client()`), and is the seam the graph hands repositories.
- **First-run, `src/core/firstrun/first_run_model.{h,cpp}`:** phases `connect -> connecting ->
  inference -> done`, driven by `IConnectionService::stateChanged` (`connecting`->connecting,
  `ready`->inference + persist setupComplete, `offline`->connect+error). GUI
  `src/DaemonApp/Pages/FirstRunGate.qml` (mounts `ConnectionPicker`, a connecting splash, the
  inference gate) + `ConnectionPicker.qml` (mode cards, target field, a **remote token field**,
  Test/Connect). TUI `src/tui/tui_dialogs.cpp` `FirstRunDialog::syncToPhase` (connecting/inference/
  connect branches; already has a masked `m_token` box shown in remote mode).
- **Settings, `src/core/settings/{isettings_store.h,qt_settings_store.*}`:** QSettings-backed typed
  bag + `conn/*` convenience accessors (`mode`, `target`, `managedLocalDaemon`, ...). **No token is
  persisted.** Settings schema versioned (`kSettingsSchema`, forward-only `migrate()`).
- **Cache, `src/core/daemon/daemon_cache_store.{h,cpp}`:** SQLite at `AppDataLocation/
  daemon_cache.db` (single global file, `defaultDatabasePath()`); non-authoritative (drops+rebuilds
  on schema bump, re-baselines from daemon). Constructed once in `app_service_graph.cpp` with an
  empty path (=> default). **No per-user namespacing.**
- **Service graph, `src/core/daemon/app_service_graph.cpp`:** builds the seam graph; in `Daemon`
  mode reuses the connection service's `NodeApiClient`; on `connection.ready` transition fires the
  baseline refresh storm (sessions/profiles/credentials/models/fs/fleet/transports + subscriptions).
- **Surfaces / nav, `src/core/nav/nav_controller.h` + `src/DaemonApp/App/Main.qml`:**
  `routeCommand()` maps ids (`accounts`/`profiles`/`fleet`/`sessions`/`dashboard`/`approvals`/...) to
  `Nav.open(...)`; pages open as singleton tabs. No capability gating. Context properties registered
  in `src/DaemonApp/App/application.cpp` (`Connection`, `FirstRun`, `Nav`, `Accounts`, `Profiles`,
  `FleetTree`, ...). The TUI mirrors via its own widgets.
- **ApiError decode, `src/core/daemon/node_api_codec/decode_responses.cpp` `decodeError()`:** maps
  `UnknownSession|Unsupported|Conflict` explicitly and **collapses everything else (incl. the new
  `Unauthenticated`/`Forbidden` arms) into `"Other"`** via the `default:` arm — we add explicit cases.
- **Tests:** `tests/unit/tst_wire_mux.cpp` + `tests/unit/wire_mux_fixture.h` (a `QLocalServer`
  `WireMuxServer` stand-in that answers `Hello` with a **v1** Hello and no `auth_mechanisms`),
  `tst_connection_service.cpp`, `tst_first_run_model.cpp`, `tst_settings_store.cpp`,
  `tst_app_service_graph.cpp`, `tst_daemon_integration.cpp`.

## 2. Frozen v2 wire contract (the facts the client must honor)

From `daemon-node/crates/contracts/daemon-api/src/wire.rs` + `daemon-api.cddl`
(+ `docs/specs/daemon-access-control.md`):

- `WIRE_VERSION = 2`. New feature flag `WIRE_FEATURE_AUTH = "auth"`. The **server** `Hello` carries
  `auth_mechanisms: [* tstr]` (preference order, e.g. `["SCRAM-SHA-256","EXTERNAL","PLAIN"]`); it is
  **empty** on an unauthenticated/local-trust node.
- C2S frames (CDDL canonical shapes):
  - `wire-hello-c2s = { "Hello": { "wire_version": uint, "features": [* tstr] } }`
  - `wire-auth-start  = { "AuthStart":  { "mechanism": tstr, "initial": [* uint] } }`
  - `wire-auth-step   = { "AuthStep":   { "data": [* uint] } }`
  - `wire-auth-resume = { "AuthResume": { "token": tstr } }`
- S2C frames:
  - `wire-hello-s2c = { "Hello": { "wire_version": uint, "features": [* tstr], "auth_mechanisms": [* tstr] } }`
  - `wire-auth-challenge = { "AuthChallenge": { "data": [* uint] } }`
  - `wire-auth-ok        = { "AuthOk": { "token": tstr, "principal": principal-view } }`
  - `wire-auth-error     = { "AuthError": { "reason": tstr } }`
  - `principal-view = { "user_id": tstr, "username": tstr, "roles": [* tstr], "capabilities": [* tstr] }`
- **CRITICAL ENCODING QUIRK:** auth byte payloads (`initial`/`data`) are Rust `Vec<u8>` **without
  serde_bytes**, so on the wire they are a **CBOR array of uints (`[* uint]`)**, NOT a byte string
  (`bstr`). The client must encode/decode SASL bytes as a major-4 array of one-byte uints, and read
  `AuthChallenge.data` the same way. This is the single most error-prone detail; fixtures from
  `daemon-api/fixtures/cbor/wire-*.cbor` should anchor the byte-exact tests.
- `ApiError` gained `Unauthenticated(tstr)` and `Forbidden(tstr)` arms (already in the vendored
  generated codec).
- Threat model (spec §1–2): non-local transports are hostile → TLS + no cleartext creds; the server
  is the sole authority; `PrincipalView` is **advisory, UI-gating only**; fail-closed; local trust is
  an explicit `[api].local_trust` decision (so a local Unix node legitimately sends empty
  `auth_mechanisms` and the client proceeds unauthenticated — we must preserve that path).
- Mechanisms (spec §2): `SCRAM-SHA-256` (preferred interactive), `PLAIN` (**only over TLS**),
  `EXTERNAL` (mTLS client-cert), session token via `AuthResume` (reconnect fast-path).

---

## 3. Workstream 1 — `client-sasl-tls`

### 3.1 Envelope auth frames (`wire_l0.cpp`, `node_api_codec.h`, `internal.h`, `mappers.cpp`)

- Bump `NodeApiCodec::kWireVersion` `1 -> 2`. `encodeHelloFrame()` already sends `features
  ["mux","stream"]`; keep that (the server intersects). Do **not** add `"auth"` to the client's
  requested features — auth is server-driven via `auth_mechanisms`.
- Extend `WireFrameKind` with `AuthChallenge`, `AuthOk`, `AuthError`.
- Extend `DecodedWireFrame` with: `QByteArray authData` (AuthChallenge), `QString token` +
  `DecodedPrincipalView principal` (AuthOk), `QString authReason` (AuthError). Add a
  `DecodedPrincipalView { QString userId, username; QStringList roles, capabilities; }` to
  `node_api_codec.h`.
- New CBOR primitives in `mappers.cpp` (declared in `internal.h`):
  - `cborAppendByteArrayAsUintArray(QByteArray&, const QByteArray& bytes)` → emits `major-4` array
    header for `bytes.size()` then one `cborAppendUint(b, byte)` per byte (the `[* uint]` quirk).
  - `cborAppendTextLong(QByteArray&, const QString&)` → length-prefixed major-3 for arbitrary length
    (the existing `cborAppendText` only handles <24B; tokens/usernames can exceed that).
  - `cborReadByteArrayFromUintArray(...)` → read a major-4 array of uints into a `QByteArray`.
  - reuse `cborReadText` for tstr; add a length-tolerant text reader if needed (it already is).
- New encoders:
  - `encodeAuthStartFrame(const QString& mechanism, const QByteArray& initial)`
  - `encodeAuthStepFrame(const QByteArray& data)`
  - `encodeAuthResumeFrame(const QString& token)`
- Extend `decodeWireFrame()`:
  - `Hello`: also parse `auth_mechanisms` (a tstr array) into `DecodedWireFrame.authMechanisms`
    (`QStringList`). The current loop already iterates inner fields by key — add an
    `else if (hkey == "auth_mechanisms")` arm mirroring the `features` arm. An older (v1) server omits
    it → empty list (unauth path).
  - `AuthChallenge` → read `data` (`[* uint]`) into `authData`.
  - `AuthOk` → read `token` (tstr) + nested `principal` map (`user_id`,`username`,`roles`,
    `capabilities`).
  - `AuthError` → read `reason` (tstr).
- **Byte-exactness:** add unit coverage that the encoded `AuthStart`/`AuthStep` match the CDDL
  (`[* uint]` not `bstr`) and round-trip the node fixtures `wire-c2s-hello.cbor` / `wire-s2c-hello.cbor`.

### 3.2 SASL client mechanisms (new `src/core/daemon/node_api_auth.{h,cpp}` + crypto helpers)

A small, dependency-light, **pure** SASL layer that the `NodeApiClient` drives. Keep crypto in pure
functions so they unit-test against RFC vectors with no socket.

- `SaslMechanism` interface: `QString name()`, `QByteArray initialResponse()`,
  `std::optional<QByteArray> step(const QByteArray& challenge)` (returns the next client message, or
  `std::nullopt` when the mechanism is complete / on a verification failure → caller treats as auth
  failure), `bool succeeded()`.
- `PlainClientMechanism`: `initialResponse()` = `\0 username \0 password` (UTF-8). One step, no
  challenge. **Refuses to construct/select unless the transport is TLS** (enforced by the selector,
  §3.3).
- `ExternalClientMechanism`: `initialResponse()` = empty (authzid omitted → server maps the cert
  fingerprint). Requires a client cert configured on the QSslSocket (§3.4).
- `ScramSha256ClientMechanism` (RFC 5802 + RFC 7677), the bulk:
  - client-first: `gs2-header("n,,") + "n=" + saslPrep(user) + ",r=" + cnonce` (cnonce = base64 of
    ≥24 random bytes from `QRandomGenerator::system()`). `initialResponse()` is the **whole**
    client-first-message (gs2 header included; `AuthStart.initial`).
  - on `AuthChallenge` = server-first `r=<combined>,s=<b64 salt>,i=<iter>`: validate the server nonce
    starts with our cnonce; compute and return client-final.
  - crypto (pure helpers, own functions):
    - `SaltedPassword = PBKDF2-HMAC-SHA256(password, salt, i, dkLen=32)` — hand-rolled over
      `QMessageAuthenticationCode(QCryptographicHash::Sha256)` (Qt has no PBKDF2). RFC-vector tested.
    - `ClientKey = HMAC(SaltedPassword, "Client Key")`; `StoredKey = SHA256(ClientKey)`.
    - `AuthMessage = client-first-bare + "," + server-first + "," + client-final-without-proof`
      (`client-final-without-proof = "c=biws,r=<combined>"`, `biws` = base64("n,,")).
    - `ClientSignature = HMAC(StoredKey, AuthMessage)`; `ClientProof = ClientKey XOR ClientSignature`;
      client-final = `client-final-without-proof + ",p=" + base64(ClientProof)`.
    - Keep `ServerKey = HMAC(SaltedPassword,"Server Key")`, `ServerSignature = HMAC(ServerKey,
      AuthMessage)` to verify the server's `v=` if a server-final arrives (see §3.3 round-trip note).
  - SASLprep: do a minimal pass (reject control chars; pass through normal UTF-8). Full RFC 4013 is
    over-scope; document the simplification.
- Crypto helpers tested with **published SCRAM-SHA-256 / PBKDF2-HMAC-SHA256 test vectors** so they are
  verifiable without the live server (the e2e SCRAM round-trip waits on Auth 3).

### 3.3 `NodeApiClient` auth state machine + outbox gate

- Add `Handshake::Authenticating` between `Handshaking` and `Ready`. New members:
  `m_authMechanisms` (from server Hello), `m_principal` (`DecodedPrincipalView`), and an
  `AuthCredentials` the connection service sets before connect:
  ```
  struct AuthCredentials {
      QString username, password;   // interactive (SCRAM / PLAIN)
      QString resumeToken;          // AuthResume fast-path (preferred when present)
      bool tlsActive = false;       // set by the transport; gates PLAIN/EXTERNAL
      bool hasClientCert = false;   // gates EXTERNAL
      QStringList preferred;        // optional client-side ordering override
  };
  ```
- On the server `Hello`:
  - If `auth_mechanisms` is **empty** → behave exactly as today: `Ready` + `flushOutbox()` (this is
    the local-trust / unauth Unix path; must keep working unchanged).
  - Else → `Authenticating`, **do not flush the outbox**. Select a mechanism:
    1. If `resumeToken` non-empty → send `AuthResume{token}` (skips mechanism negotiation).
    2. Else pick from `intersection(server auth_mechanisms, client-capable)` in preference order:
       `EXTERNAL` (iff `hasClientCert`), `SCRAM-SHA-256` (iff username+password), `PLAIN` (iff
       username+password **and** `tlsActive`). Send `AuthStart{mechanism, initialResponse()}`.
    3. If nothing is selectable → fail with a clear "this node requires credentials" (no frames sent;
       stay disconnected) so the UI shows the login form.
- On `AuthChallenge` while `Authenticating` → `mechanism->step(data)`; if it returns a message send
  `AuthStep{data}`; if it returns `nullopt` → auth failure (abort, no flush). **Round-trip
  tolerance:** loop on `AuthChallenge` until `AuthOk`/`AuthError` — the exact step count (e.g.
  whether the server sends a final `AuthChallenge` carrying `v=` that expects an empty `AuthStep`
  before `AuthOk`, vs folding it into `AuthOk`) is an Auth 3 wiring detail; the client must not assume
  a fixed count. (OPEN-1.)
- On `AuthOk` → record `m_principal` + token, transition `Ready`, `flushOutbox()`, emit
  `authenticated(principal)` and `tokenIssued(token)`.
- On `AuthError` → emit `authFailed(reason)`, clear creds/outbox, stay `Disconnected` (so the next
  attempt re-handshakes). **Never** flush queued requests. `reason` is intentionally coarse (no
  account-probing oracle) — surface verbatim.
- New signals: `authenticated(DecodedPrincipalView)`, `authFailed(QString reason)`,
  `tokenIssued(QString token)`. Add `principal()` + `isAuthenticated()` getters.
- **No-leak invariant:** username/password/token never go through `qDebug`/`qInfo`/`failed()` text.
  Add a clang-tidy-friendly review note + a test that asserts no credential substring appears in any
  emitted `failed`/status message.

### 3.4 TLS / TCP transport (`daemon_transport.{h,cpp}`)

Generalize the transport to carry either a Unix `QLocalSocket` (plaintext, unchanged) or a TCP
`QSslSocket` (TLS). Both are `QIODevice`s; share the read-buffer/`tryTakeFrame` logic.

- Target parsing: keep `setSocketPath(path)` for Unix; add `setTcpTarget(host, port, TlsConfig)`.
  The connection service decides which based on `mode`/`target` (§3.5). A `target` like
  `https://host:port` or `host:port` → TCP+TLS; a filesystem path or `unix://` → Unix.
- Internals: hold `QIODevice* m_io` plus the concrete socket; wire `readyRead`→`handleReadyRead`,
  `disconnected`→`disconnected`, error→`failed` for whichever socket. `isConnected()` checks the
  active socket's state.
- TLS defaults (the **fail-closed** posture):
  - `QSslSocket::connectToHostEncrypted(host, port)`. Default `QSslSocket` verification is full CA +
    hostname; **do not** call `ignoreSslErrors()` by default. On `sslErrors`, if no explicit override
    is configured → `abort()` + `emit failed("TLS certificate not trusted: ...")`. This is the
    "TLS rejects an untrusted cert unless explicitly configured" guard.
  - `TlsConfig` knobs (from settings, §4.4): `caFile` (extra/pinned CA), `pinnedSha256` (cert
    fingerprint pinning → accept iff the leaf matches, even self-signed), `clientCertFile` +
    `clientKeyFile` (mTLS → `setLocalCertificate`/`setPrivateKey`, enables EXTERNAL), and a loud,
    default-`false`, dev-only `insecureSkipVerify` (logs a warning, never the default, never silent).
  - Expose `bool tlsActive()` so the client gates PLAIN/EXTERNAL (§3.3) and a `clientCertConfigured()`.
- Unix path stays byte-for-byte the same → `local_trust` plaintext unchanged.

### 3.5 Connection service (`daemon_connection_service.cpp`) — stop ignoring `token`

- `connectTo(mode, target, token)`:
  - Remove `Q_UNUSED(token)`. Treat a non-empty `token` as the **AuthResume** credential.
  - Support `mode == "remote"`: parse `target` → host/port + TLS, call `transport->setTcpTarget(...)`
    with the `TlsConfig` read from settings; for `mode == "local"` keep `setSocketPath`.
  - Set `m_client` credentials (`AuthCredentials`) before the first send: token (if any) for resume;
    username/password come from `login()` (below) for interactive.
- New interactive entry point on `IConnectionService` (extend the interface):
  `Q_INVOKABLE virtual void login(const QString& username, const QString& password)` — stores creds
  on the client and kicks the connect/handshake (or re-handshake) so SASL runs. (Keeps `connectTo`'s
  signature for the token/resume path; adds the password path the login UI needs.)
- New states: add `"authenticating"` to the documented set; surface auth outcome:
  - relay `NodeApiClient::authenticated(principal)` → keep/await `ready` (Health still gates "ready");
    store the principal and `emit` a new `principalChanged()`.
  - relay `authFailed(reason)` → `setStatusMessage(reason)` + `setState("offline")` (stays
    disconnected; the UI shows the error and the login form). **Wrong password ⇒ offline + reason.**
  - relay `tokenIssued(token)` → persist via the token store (§4.4) keyed by target; this is the
    server-issued token, **not** the password.
- `disconnect()` (logout path, §4.4): clear stored token + in-memory creds + principal.
- `testConnection`: extend so `remote` targets validate URL/port shape (full probe still needs the
  server; keep it shape-only and say so).
- Expose `principalView()` (or hand it to a dedicated `PrincipalModel`, §5.1).

---

## 4. Workstream 2 — `client-login-ui` (GUI + TUI)

### 4.1 `FirstRunModel` auth phase

- New phase `"auth"` between `connecting` and `inference`. Drive it off connection state:
  - `connecting` → phase `connecting` (unchanged).
  - new state `authenticating` (or a `connection->authRequired()` signal) → phase `auth` (show the
    login form; clear error).
  - `authFailed`/`offline` with an auth reason → stay/return to phase `auth` with the reason as
    `error` (NOT back to `connect`, so the user can retry the password without re-entering the
    target). A transport/connect failure (not auth) still returns to `connect` as today.
  - `ready` → `inference` (unchanged; setupComplete persisted as today).
- Add `Q_INVOKABLE void submitLogin(username, password)` that calls `connection->login(...)`, or wire
  the UI directly to `Connection.login` and let `FirstRunModel` only reflect phase. Prefer the latter
  (keep the model thin) — the model just maps connection state → phase + error.
- **Shell-never-mounts-pre-auth** is already structurally guaranteed: `Main.qml`'s shell is always
  built, but the `FirstRunGate` Loader (`z:200`, `active: FirstRun.active`) covers it until
  `done`, and `done` is only reached after `ready` (post-`AuthOk`). We extend the gate to also cover
  the `auth` phase, and add a test asserting `FirstRun.active` stays true through `authenticating`.
  (The TUI `FirstRunDialog` is modal and only closes on `done` — same guarantee.)

### 4.2 GUI — `FirstRunGate.qml` (+ `ConnectionPicker.qml`)

- Add an **auth section** to `FirstRunGate.qml`, visible when `root.phase === "auth"`:
  username `Kit.TextField`, password `Kit.TextField` (`echoMode: TextInput.Password`), a "Sign in"
  accent button → `Connection.login(user, pass)`, an error `Text` bound to `FirstRun.error`, and a
  busy state while `Connection.state === "authenticating"`. A "Back" affordance returns to `connect`.
- `ConnectionPicker.qml`: keep the existing remote token field but relabel it for the
  resume/token path; the primary interactive path is now the `auth` phase. When the user supplies a
  token it flows through `Connection.connectTo(mode, target, token)` (AuthResume); when empty,
  reaching a server that requires auth advances to the `auth` phase for password login.
- Password/token are bound to local QML fields only and passed straight to the seam — never written
  to `AppSettings` and never logged.

### 4.3 TUI — `FirstRunDialog::syncToPhase` (`tui_dialogs.cpp`)

- Add an `auth` branch: reuse the existing masked `m_token` box repurposed as a password field plus a
  username `ZInputBox` (add `m_username`); set the primary button text to `tr("Sign in")`, wire it to
  `m_connection->login(m_username->text(), m_password->text())`. Hide the target/mode controls in this
  phase (target already chosen). Show `m_model->error()` in `m_status`.
- The `connect` branch's existing token box stays for the AuthResume/token path (remote mode).

### 4.4 Token persistence (server-issued), AuthResume, logout

- Persist the **server token from `AuthOk`**, never the password. Keyed by connection target so
  switching nodes doesn't cross tokens.
- `ISettingsStore` additions (named accessors, no hardcoded keys in UI):
  `QString connectionToken(target)`, `void setConnectionToken(target, token)`,
  `void clearConnectionToken(target)`. Default impl → `conn/token` (single) or
  `conn/tokens/<sha256(target)>` (multi-node) in QSettings. Bump `kSettingsSchema` only if a key is
  renamed; new keys need no migration.
- **Keychain (preferred):** add an optional `ITokenStore` seam with two impls — a
  `KeychainTokenStore` (Qt Keychain) and a `SettingsTokenStore` (the QSettings fallback). Select
  Keychain when available, else fall back. CMake `find_package(Qt6Keychain)` guarded by
  `#ifdef DAEMON_APP_HAVE_KEYCHAIN`; **flake change required**: add `qtkeychain` to the devShell Qt
  packages in `flake.nix` (nixpkgs `qt6Packages.qtkeychain`). If the coordinator prefers to avoid the
  flake change now, ship the QSettings fallback as the guaranteed path and land Keychain behind the
  ifdef later (the fallback already satisfies the "persist the token, not the password" guard).
- **Reconnect uses AuthResume:** on (re)connect, the connection service loads the stored token for the
  target and sets it as `AuthCredentials.resumeToken`, so `NodeApiClient` sends `AuthResume` instead
  of a full SCRAM exchange. If `AuthResume` returns `AuthError` (token expired/revoked), clear the
  stored token and fall back to the password login UI (phase `auth`).
- **Logout** (a Settings/cog action + `connection->disconnect()` path): clear the stored token,
  in-memory creds, principal, and reset the per-user cache namespace (§5.4) so the next login starts
  clean.

---

## 5. Workstream 3 — `client-access-ui`

### 5.1 `WhoAmI` / principal model

- New `PrincipalModel : QObject` (`src/core/access/principal_model.{h,cpp}`), context property
  `Principal` (registered in `application.cpp`; the TUI gets the same object). Properties:
  `userId`, `username`, `roles` (QStringList), `capabilities` (QStringList), `authenticated` (bool).
  `Q_INVOKABLE bool hasCapability(const QString& cap)`, `Q_INVOKABLE bool hasRole(const QString&)`.
- Populated from `NodeApiClient::authenticated(principal)` relayed by the connection service. On
  disconnect/logout → cleared (`authenticated=false`, empty caps) so surfaces re-gate closed.
- **WhoAmI refresh:** scaffold a `WhoAmI` request path so the principal can be re-fetched live once
  Auth 5 exposes it; mark the actual call **pending Auth 5** (the `AuthOk` `PrincipalView` is the
  source until then). (See `ApiRequest::WhoAmI` / `ApiResponse` once the admin DTOs land — Auth 5.)

### 5.2 Capability-gating of surfaces

- Capabilities are **advisory UI hints** (server still enforces). Use the snake_case names from
  `daemon-auth/capability.rs` mirrored on `PrincipalView` (e.g. `session_write`, `session_see_all`,
  `session_control_any`, `fleet_write`, `access_admin`). Role ladder: viewer ⊂ user ⊂ operator ⊂ admin.
- GUI: in `Main.qml` `routeCommand()` + the cog/command-palette entry lists, gate each page:
  - `accounts`/`profiles` (write surfaces) — visible to `user`+ (have `profile_write`); a `viewer`
    sees read-only or hidden controls.
  - `fleet`/`dashboard`/`sessions` node-wide views — gated on `session_see_all` (operator+).
  - **new** `access` (Users & Access) — gated on `access_admin` (admin only). **A Viewer sees no
    admin page** (the explicit test guard).
  - Hide vs disable: hide admin-only entries entirely; disable+tooltip for partially-available ones.
- TUI: mirror the gating where these pages are surfaced (page-launcher menu / slash commands).
- Add a tiny QML helper (e.g. `Principal.hasCapability("access_admin")`) used in `visible:` bindings;
  re-evaluates on `Principal` change.

### 5.3 Users & Access admin page (scaffold; CRUD pending Auth 5)

- New GUI page `src/DaemonApp/Pages/UsersAccessPage.qml` + nav id `"access"` (+ `routeCommand` +
  cog/palette entry gated on `access_admin`) and a matching TUI page/window.
- Content now: the **WhoAmI** panel (current principal: username, roles, effective capabilities) +
  a placeholder user list with "Admin API not yet available" state.
- A new `AccessAdminRepository` (`src/core/daemon/`) scaffolded with the intended ops (list users,
  create/disable user, set role, set password, revoke sessions, `WhoAmI`) — but the actual NodeApi
  calls are **stubbed/marked pending the node `access-admin-api` (Auth 5)**; the contract reserves the
  `AccessControl*` / `ResourceGrant*` DTOs (Track D / additive CDDL). Wire the UI to the repository so
  the data path drops in when Auth 5 lands; until then it returns an "unsupported/pending" state.

### 5.4 Per-user cache namespacing

- Goal: switching users must not surface the prior user's cached roster/sessions/transcripts.
- `DaemonCacheStore`: add `void setUserNamespace(const QString& userKey)` that closes the current
  SQLite connection and reopens at a namespaced path
  `AppDataLocation/daemon_cache-<sanitized userKey>.db` (and `defaultDatabasePath()` keeps the bare
  `daemon_cache.db` for the pre-auth/local-trust default). `userKey` = the principal's stable
  `user_id` (opaque), sanitized/hashed for a safe filename. Reopening is cheap and safe — the cache is
  non-authoritative and re-baselines from the daemon on the next `ready`.
- Wiring (`app_service_graph.cpp`): on `NodeApiClient::authenticated(principal)` (relayed via the
  connection service), call `cache->setUserNamespace(principal.userId)` **before** the baseline
  refresh storm fires (the storm runs on `ready`, which follows `AuthOk`, so order is: AuthOk →
  setUserNamespace → ready → refresh). For the unauth/local-trust node (empty `auth_mechanisms`) the
  namespace stays the default.
- Logout/user-switch resets to the default namespace (or the new user's) so no rows leak across the
  boundary. Repositories already read through the cache seam, so they pick up the swapped DB
  transparently.
- **Guard test:** seed cache as user A, switch namespace to user B, assert B sees none of A's rows
  (and the files are distinct paths).

### 5.5 `ApiError` Unauthenticated / Forbidden handling

- `decodeError()` (`decode_responses.cpp`): add explicit `case api_error_unauthenticated_m_c` →
  `kind="Unauthenticated"` and `case api_error_forbidden_m_c` → `kind="Forbidden"` (today they fall
  into `default → "Other"`). Update the `DecodedApiError.kind` doc comment to list them.
- Behavior: a repository/Connection seeing `Unauthenticated` (e.g. a request raced ahead, or token
  expired mid-session) → drop to the login/auth flow (re-auth, AuthResume then password). `Forbidden`
  → a non-fatal capability error surfaced as a toast/status, not a disconnect (the server denied one
  op; the connection stays up). Keep messages free of secret material.

---

## 6. Test & build plan (mapped to the safety guards)

Extend `tests/unit/wire_mux_fixture.h` `WireMuxServer` first (it underpins most tests):
- Bump its `buildHello()` to `wire_version=2` and add a configurable `auth_mechanisms` list
  (default empty = unauth, so existing tests are unchanged).
- Add a scriptable auth flow: parse `AuthStart`/`AuthStep`/`AuthResume`, answer with `AuthChallenge`/
  `AuthOk`/`AuthError` per a configured script; a tiny server-side SCRAM-SHA-256 responder (or a
  canned challenge + accept-on-correct-proof) so the client SCRAM path round-trips in-process. Add a
  "wrong password" mode (→ `AuthError`) and a "resume" mode (accept `AuthResume{token}` → `AuthOk`).
- Helpers to build `AuthChallenge`/`AuthOk`/`AuthError` frames (with the `[* uint]` byte encoding) and
  a parser for `AuthStart`/`AuthStep` initial/data arrays.

Tests (new + extended), each tied to a plan safety guard:

| Guard (from meta plan) | Test |
|---|---|
| No request before `AuthOk` (outbox) | extend `tst_wire_mux`: with `auth_mechanisms` set, queue a `Call` pre-login; assert the server sees **zero** `Call` frames until `AuthOk`, then exactly one after. |
| SCRAM round-trips | new `tst_node_api_auth`: PBKDF2/HMAC/ClientProof against published RFC 7677/5802 vectors; full client↔fixture SCRAM exchange ending in `AuthOk`. |
| TLS rejects untrusted cert unless configured | new `tst_tls_transport`: a `QSslServer` (self-signed) → connect with defaults fails (`failed`, no frames); connect with the pin/CA/`insecureSkipVerify` configured succeeds. |
| token/password never logged | `tst_node_api_auth`/`tst_connection_service`: capture emitted `failed`/status text and assert no username/password/token substring appears. |
| Shell never mounts pre-auth (GUI + TUI) | extend `tst_first_run_model`: `FirstRun.active` stays true through `authenticating`/`auth`; only `done` (post-AuthOk→ready) clears it. (+ a TUI dialog test that it closes only on `done`.) |
| Wrong password → error, stays disconnected | `tst_connection_service`: fixture in wrong-password mode → state `offline`, `statusMessage` = reason, no token persisted. |
| Reconnect uses `AuthResume` | `tst_connection_service`: with a stored token, (re)connect sends `AuthResume{token}` (assert via fixture) and reaches ready without a SCRAM exchange; expired token → falls back to password. |
| Logout clears token | `tst_settings_store`/`tst_connection_service`: after `disconnect()`/logout the stored token is gone. |
| Capability-gated surfaces (Viewer sees no admin page) | new `tst_principal_model` (+ a QML gating test if feasible): `hasCapability` truth table; admin entry hidden without `access_admin`. |
| Per-user cache namespacing (no cross-login leak) | new `tst_cache_namespacing`: user A rows invisible after `setUserNamespace(B)`; distinct DB files. |
| ApiError Unauthenticated/Forbidden | extend `tst_daemon_integration`/a codec test: `decodeError` returns the two new kinds. |

Plus: keep `tst_app_service_graph` green (new wiring), and re-run the full gate set in §0. New source
files added to the relevant `CMakeLists.txt` (`src/core/daemon/CMakeLists.txt`,
`src/core/access/...`, `src/DaemonApp/Pages/CMakeLists.txt`, `tests/unit/CMakeLists.txt`).

---

## 7. Cross-track dependencies & blockers (flag clearly)

- **Auth 3 (server authn + transport) — blocks end-to-end verification.** The wire contract is frozen,
  so the client builds and the SCRAM/PLAIN/EXTERNAL/resume logic is **unit-tested against an in-repo
  fixture + RFC vectors**, but a real login (and the exact SASL round-trip count, OPEN-1) can only be
  confirmed once the node's `rsasl-authenticator` + `tls-listener` + `serve_mux` integration is live.
  Phase-2 "verification" for this track = build + unit/widget/fixture tests, **not** full e2e (that's
  Auth 7).
- **Auth 5 (`access-admin-api`) — blocks the admin page's data path.** The contract defers the admin
  `AccessControl*`/`ResourceGrant*` DTOs to Track D (additive CDDL). So §5.3 **scaffolds** the Users &
  Access page + `AccessAdminRepository` + WhoAmI panel, but the user-CRUD calls are stubbed/marked
  "pending Auth 5". When the regenerated codec + admin DTOs land, drop them in behind the existing UI.
- **Codec generation (superproject).** The vendored generated codec is already at v2
  (`api_error_unauthenticated/forbidden`, `principal-view`), so this track needs no `just update-codec`
  run. If Auth 5 adds admin request/response DTOs, the generated codec must be regenerated in the
  superproject (not in this worktree) before §5.3's calls can be un-stubbed — note for the coordinator.
- **Flake change for Qt Keychain (§4.4).** Preferred token storage needs `qtkeychain` added to
  `flake.nix`. If the coordinator wants to avoid touching the flake in this track, the QSettings
  fallback ships first and Keychain lands behind `#ifdef DAEMON_APP_HAVE_KEYCHAIN` later. **Decision
  requested.**

## 8. Open decisions (need coordinator input)

- **OPEN-1 (SASL round-trips).** Does Auth 3's server send a terminal `AuthChallenge` with the SCRAM
  `v=` server-signature (expecting an empty `AuthStep` before `AuthOk`), or fold success straight into
  `AuthOk`? The client is written tolerantly (loop until `AuthOk`/`AuthError`, verify `v=` if
  present), but confirm so the fixture matches the real server.
- **OPEN-2 (token-per-target vs single).** Persist one `conn/token` or per-target
  `conn/tokens/<hash(target)>`? Recommend per-target (supports multiple nodes; avoids cross-node token
  reuse). 
- **OPEN-3 (Keychain now vs later).** Add `qtkeychain` to the flake in this track, or ship the
  QSettings fallback first? (See §7.)
- **OPEN-4 (remote target syntax).** Accept `https://host:port`, `tcp://host:port`, and bare
  `host:port`? Recommend all three, normalizing to host+port+TLS; `unix://` / a path → Unix.
- **OPEN-5 (capability→surface map).** Confirm the exact capability gating each page on
  (`session_see_all` for Fleet/Dashboard, `access_admin` for the admin page, etc.) against the final
  `capability.rs` vocabulary.

## 9. Suggested implementation order (post-validation)

1. Envelope auth frames + `kWireVersion=2` + `decodeError` arms + fixture/codec byte-exact tests
   (§3.1, §5.5). Lowest risk, unblocks everything.
2. SASL mechanisms + crypto helpers with RFC-vector tests (§3.2).
3. `NodeApiClient` auth state machine + outbox gate; extend `WireMuxServer`; outbox/SCRAM/resume/
   wrong-password tests (§3.3, §6).
4. TLS transport + `tst_tls_transport` (§3.4).
5. Connection service: token + `login()` + states + principal/token relay (§3.5).
6. Token store (QSettings fallback first, Keychain behind ifdef) + AuthResume reconnect + logout (§4.4).
7. `FirstRunModel` auth phase + GUI auth section + TUI auth branch (§4.1–4.3).
8. `PrincipalModel`/WhoAmI + capability gating + per-user cache namespacing (§5.1, §5.2, §5.4).
9. Users & Access page + `AccessAdminRepository` scaffold (pending Auth 5) (§5.3).
10. Full gate sweep (§0) + green ctest.

Each step builds + passes its tests before the next; nothing here can be e2e-confirmed until Auth 3.
