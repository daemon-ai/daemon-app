# HARDENING-PLAN — Phase 5 / daemon-app (`app-session-meta`)

Track: `hardening/app-session-meta` (worktree `/home/j/experiments/daemon-worktrees/app-session-meta`,
off `hardening/app-v28-codec`). **This document is plan-only; no source touched yet.**

Scope (from the OpenClaw plan, Phase 5 first daemon-app item):
1. Wire `SessionUpdateMeta` for **pin / archive / title** instead of the current cache-only writes.
2. Continue the `seam_migration.h` migration for **roster, dashboard, routing, cron, checkpoints,
   IDaemonConfig** so no mock stands in as *authority* in daemon mode.

Governing invariant (`daemon-app/AGENTS.md`): the node decides, the app renders; the app sends
intents and never caches-and-mutates domain state. GUI + TUI are two thin renderers over ONE shared
C++ view-model layer — a GUI-only change is incomplete.

---

## 0. Node-side facts I confirmed (no node work needed on this track)

- `SessionUpdateMeta { session, patch: SessionMetaPatch }` exists on the wire
  (daemon-api `dispatch.rs:165`, `wire.rs:525`) and is in the v28 vendored codec on the base branch.
- The generated codec already carries the request + patch types
  (`src/core/daemon/codec/generated/daemon_api_client_types.h`):
  - `struct request_session_update_meta { zcbor_string SessionUpdateMeta_session; session_meta_patch SessionUpdateMeta_patch; }`
  - `struct session_meta_patch` with three optional-and-nullable members, each a `_present` flag +
    a `_choice` (value arm vs `null_m`):
    - `session_meta_patch_title` → `session_meta_patch_title_tstr` / `..._null_m`
    - `session_meta_patch_pinned` → `session_meta_patch_pinned_bool` / `..._null_m`
    - `session_meta_patch_archived` → `session_meta_patch_archived_bool` / `..._null_m`
  - `encode_request_session_update_meta(...)` and the `api_request_request_session_update_meta_m` /
    `..._m_c` arm are present (`daemon_api_client_encode.c`).
- The node emits `SessionMetaChanged { session, rev }` after a meta change; the app already routes it
  (`subscription_manager.cpp:114`) to a debounced `SessionRepository::refreshSessions()`. **This is
  the authoritative re-render path** — once the node accepts the intent, the roster refetch reflects
  the node's truth with no local re-derivation.

So pin/archive/title is **pure app-side**: add the encoder + repository intent + route the store
writes through it; render only the node's response.

---

## 1. Where pin/archive/title write cache-only today (the bug)

The cache-only writes live in the daemon-mode session store,
[`src/core/daemon/cached_session_store.cpp`](src/core/daemon/cached_session_store.cpp):

- `setArchived(id, archived)` (lines ~343–354): mutates the local `CachedSessionRow.archived` +
  `updatedAtMs`, `m_cache->upsertSession(row)`, `reload()`. **Never contacts the node.**
- `setPinned(id, pinned)` (lines ~360–371): same shape for `pinned`.
- `renameSession(id, title)` (line ~356): currently a **no-op** (title never persisted at all).

These are the three `ISessionStore` domain-meta mutators. Everything upstream already funnels through
them on BOTH surfaces:

- **GUI** (QML → shared `ISessionStore` via `Q_INVOKABLE` QString shims in
  [`isession_store.h`](src/core/persistence/isession_store.h)):
  - `SessionsList/SessionsList.qml`, `Session/Session.qml`, `Pages/ArchivedSection.qml`,
    `SessionsList/sessions_list_model.cpp` (pinned role).
- **TUI** (Tui Widgets → same `ISessionStore`), in
  [`src/tui/root_widget_wiring.cpp`](src/tui/root_widget_wiring.cpp):
  - pin toggle `store->setPinned(id, !store->isPinned(id))` (~L288),
  - rename `store->renameSession(id, text)` (~L322, ~L438).

**Consequence of the current design:** because both surfaces call the same `ISessionStore` methods,
routing the fix through `CachedSessionStore` (the shared C++ layer) fixes GUI **and** TUI in one
change — no per-surface code needed. This satisfies the parity invariant by construction.

Mock mode uses `InMemorySessionStore` (dev stand-in) — unchanged; local mutation there is legitimate.

---

## 2. Part A — Wire `SessionUpdateMeta`

### A1. Codec facade: `encodeSessionUpdateMetaRequest`

Add to the facade declaration
[`src/core/daemon/node_api_codec.h`](src/core/daemon/node_api_codec.h) (near the session ops, after
`encodeSessionCreateRequest`) and implement in
[`src/core/daemon/node_api_codec/encode_requests.cpp`](src/core/daemon/node_api_codec/encode_requests.cpp).

Signature (tri-state optionals so a patch touches only the field(s) the caller set):

```cpp
// SessionUpdateMeta{session, patch}: patch a session's node-owned metadata (pin/archive/title).
// Each optional is Some(value)=set, absent=leave unchanged. Node emits SessionMetaChanged on Ok.
[[nodiscard]] static QByteArray
encodeSessionUpdateMetaRequest(const QString& sessionId,
                               std::optional<bool> pinned = std::nullopt,
                               std::optional<bool> archived = std::nullopt,
                               std::optional<QString> title = std::nullopt);
```

Implementation follows the `encodeSessionCreateRequest` pattern exactly (borrowed UTF-8 buffers kept
alive across the synchronous encode; `encodeWithFill` selecting
`api_request_r::api_request_request_session_update_meta_m_c`). For each present optional set
`patch.session_meta_patch_<f>_present = true`, `_choice = ..._<value>_c`, and the value; leave
`_present = false` otherwise. Title uses the `_tstr` arm with a scratch `QByteArray` from `.toUtf8()`.
(We only ever send the value arm, not `null_m`; clearing a title is not a current UI affordance.)

No hand-rolled CBOR — the generated `encode_request_session_update_meta` does the map framing.

### A2. Repository intent: `SessionRepository::updateSessionMeta`

[`src/core/daemon/repositories.h`](src/core/daemon/repositories.h) /
[`repositories.cpp`](src/core/daemon/repositories.cpp) — add a method modeled on `createSession`
(send) + the `-> Ok`/`Error` response handling (createSession's failure surfacing is the template):

```cpp
// Send SessionUpdateMeta; on Ok the node emits SessionMetaChanged -> debounced refetch, and we
// also issue an immediate authoritative refreshSessions() (race-free: the node persisted + bumped
// rev before replying Ok, mirroring the createSession refetch). On Error/transport failure emit
// metaUpdateFailed so a rejected pin/rename (e.g. Forbidden for a non-owner) never looks applied.
void updateSessionMeta(const QString& sessionId, std::optional<bool> pinned,
                       std::optional<bool> archived, std::optional<QString> title);
signals: void metaUpdateFailed(const QString& sessionId, const QString& message);
```

Wiring detail:
- add `kUpdateMetaCorrelation = "repo/session-update-meta"`;
- `handleResponse`: on that correlation, `responseKind == Ok` → `refreshSessions()`; else
  `decodeError` → `emit metaUpdateFailed(...)`;
- `handleFailure`: same correlation → `emit metaUpdateFailed(sessionId, message)`.

This is the OpenClaw-relevant part: the failure path is explicit, so the app never *implies* a policy
(pin/rename succeeded) that the node did not enforce.

### A3. Route the store writes through the intent

[`cached_session_store.cpp`](src/core/daemon/cached_session_store.cpp): replace the cache-only bodies
of `setPinned` / `setArchived` and the empty `renameSession` with delegation to
`m_sessions->updateSessionMeta(...)` (the store already holds `SessionRepository* m_sessions`).

Recommended design — **node-authoritative, no optimistic local mutation**:
- `setPinned(id, pinned)` → `m_sessions->updateSessionMeta(id.toString(), pinned, {}, {})`
- `setArchived(id, archived)` → `updateSessionMeta(id, {}, archived, {})`
- `renameSession(id, title)` → `updateSessionMeta(id, {}, {}, title)`
- Do **not** `upsertSession(...)` locally. The UI updates when the node's row lands via the Ok-driven
  `refreshSessions()` (one round-trip) and/or the `SessionMetaChanged` feed. This is the strict
  reading of the invariant ("never caches-and-mutates domain state").
- Guard `m_sessions == nullptr` (offline / no client): keep the write inert and (optionally) surface
  via `metaUpdateFailed`. **Behavior change to call out:** offline pin/archive/rename no longer
  "stick" in the cache — matching the thin-client rule that these are node-owned. See Open Decision D1
  if we want a queued/optimistic compromise.

Header change: `cached_session_store.h` needs no new members (delegation only). If we surface
`metaUpdateFailed` to the UI, relay it as an `ISessionStore` signal (Open Decision D2).

### A4. GUI + TUI parity

Automatic: both surfaces already call `ISessionStore::{setPinned,setArchived,renameSession}`; the
behavior change lives entirely in the shared `CachedSessionStore`. No `.qml` and no `src/tui` change
is required for the write path. If we add a user-visible failure toast (D2), that IS a per-surface
render bit and must land on both GUI (a Kit dialog/notice) and TUI (`root_widget_wiring.cpp`) in the
same change.

---

## 3. Part B — Continue the seam migration

Current daemon-mode wiring authority: [`src/core/daemon/app_service_graph.cpp`](src/core/daemon/app_service_graph.cpp).
Its closing `qInfo()` already enumerates "still mock: daemonConfig, memory, daemonNet, roster/dashboard,
routing/cron, checkpoints." Two of those have ready-but-unwired daemon backings; the rest are
node-blocked.

### B1. Roster + Dashboard — **wire the ready scaffolding** (the real win)

Findings:
- [`daemon_session_roster.{h,cpp}`](src/core/daemon/daemon_session_roster.cpp) (`fleet::DaemonSessionRoster`,
  offline-first over `CachedSessionStore`) and
  [`daemon_dashboard.{h,cpp}`](src/core/daemon/daemon_dashboard.cpp) (`fleet::DaemonDashboard`, counters
  derived from roster+fleet+approvals+connection) are **implemented but NOT compiled**: they are absent
  from `src/core/daemon/CMakeLists.txt`, and `app_service_graph.cpp` still constructs
  `MockSessionRoster` / `MockDashboard` in both modes.
- A ready test, [`tests/unit/tst_daemon_roster_dashboard.cpp`](tests/unit/tst_daemon_roster_dashboard.cpp),
  exists but is **NOT registered** in `tests/unit/CMakeLists.txt`.

Steps:
1. Add `daemon_session_roster.h/.cpp` + `daemon_dashboard.h/.cpp` to the `da_daemon_client` target in
   [`src/core/daemon/CMakeLists.txt`](src/core/daemon/CMakeLists.txt).
2. In `app_service_graph.cpp` daemon branch: after the final seam pointers exist (fleetTree swapped to
   `DaemonFleetTree`, approvals swapped to `DaemonApprovalsInbox`), replace
   `graph.roster` with `new fleet::DaemonSessionRoster(graph.store, owner)` and the rebuilt
   `graph.dashboard` with `new fleet::DaemonDashboard(graph.roster, graph.fleetTree, graph.approvals, graph.connection, owner)`
   (delete the mocks first, as the existing dashboard-rebuild already does to avoid the documented
   use-after-free). Keep mock mode on the mocks.
3. Update the daemon-mode `qInfo()` seam log to move roster/dashboard from "still mock" to live.
4. **Parity:** `graph.roster`/`graph.dashboard` are single seam pointers consumed by BOTH the GUI
   (QML context props) and the TUI (`tui_page_hub`, `root_widget_pages`, `tui_parity_routes`). Swapping
   the concrete class at the graph covers both surfaces automatically — verify the TUI Dashboard/Fleet
   pages still render (offscreen ctest).
5. Register `tst_daemon_roster_dashboard` in `tests/unit/CMakeLists.txt` (link `daemon-app::daemon_client`,
   `Qt6::Test`, `daemon-app::uimodels`, `daemon-app::fleet`) and add the `set_tests_properties … offscreen`.
6. Update `seam_migration.h`: `ISessionRoster + IDashboard + IApprovalsInbox` entry from `MockOnly` →
   `DaemonAligned` with a note ("offline-first over CachedSessionStore + DaemonFleetTree +
   DaemonApprovalsInbox; suspend/close are client-local overlays — no session-lifecycle wire op yet").

Honest degradation to state in the entry/commit: `DaemonSessionRoster` keeps a client-local
suspend/resume cosmetic overlay and `tokens`/`rewindable` are placeholders (no per-session token or
checkpoint wire data). That is a *presentation* degradation, not a domain re-derivation — acceptable
and documented, mirroring the existing DaemonNet "seed empty" honesty pattern.

### B2. Routing, Cron, Checkpoints, IDaemonConfig — **node-blocked; make the honesty explicit**

Findings — **no node wire ops exist** for these in the codec facade (`node_api_codec.h` has zero
cron/routing/checkpoint encoders), so they cannot become daemon-backed without node-side work that is
out of this track's scope (the node is the single authority; adding capability = a daemon-node change):

- **IDaemonConfig** ([`config/idaemon_config.h`](src/core/config/idaemon_config.h), `MockDaemonConfig`
  wired in both modes): `seam_migration.h::kConfigMigration` already records the **Phase-4 DECISION**
  that `ConfigGet/ConfigSet` are deliberately NOT wired (wire v9 collapsed runtime config into
  `ProfileUpdate`/`SetSessionOverlay`; the daemon trait returns `Unsupported`). It stays `MockOnly`.
- **Routing / Cron** ([`automation/*`](src/core/automation), `MockRoutingStore`/`MockCronStore`):
  no `automation`/routing/cron ApiRequest in the contract subset the client speaks.
- **Checkpoints** ([`session/icheckpoint_timeline.h`](src/core/session/icheckpoint_timeline.h),
  `MockCheckpointTimeline`; `CheckpointRepository` in `repositories.h` is an explicit
  cache/NodeApi-aware **stub** "until checkpoint timelines are modeled in the daemon-api codec subset").

"Continue the migration" for these = **stop them from masquerading as authority in daemon mode**,
without inventing local domain behavior:
1. In `app_service_graph.cpp`, keep them as mocks but ensure the daemon-mode `qInfo()` seam log names
   each explicitly as *node-unsupported / cosmetic*, not merely "still mock".
2. In `seam_migration.h`, give routing/cron/checkpoints entries the same explicit-DECISION treatment
   `kConfigMigration` has: state that they remain `MockOnly` because the node exposes no wire op yet,
   and that promoting them requires a daemon-node contract addition (out of app scope). This is the
   documentation deliverable that keeps the seam map truthful.
3. If any of these mocks currently render **fabricated live-looking data** in daemon mode (as the
   DaemonNet Integrations tree once did), seed them empty/degraded in daemon mode behind a
   `DAEMON_APP_MOCK_*` dev gate — mirroring the existing `demoTransports` pattern. **To verify during
   implementation:** whether the Routing/Cron/Checkpoints pages present mock rows as real in daemon
   mode; if they only show an empty/placeholder state already, no seeding change is needed and the
   documentation step suffices.

Net: B2 lands as `app_service_graph.cpp` log honesty + `seam_migration.h` decision records (+ optional
empty-seed gate), with a clear "node capability missing" note so a future node wave can pick it up.

---

## 4. Tests

Harness precedent: [`tests/unit/tst_daemon_integration.cpp`](tests/unit/tst_daemon_integration.cpp)
uses `daemonapp::test::WireMuxServer` + `DaemonTransport` + `NodeApiClient` + `SessionRepository` +
`QSignalSpy` (see `sessionCreateUpsertsCacheAndRefreshes` / `sessionCreateErrorSurfaces`).

New / touched tests:
1. **Codec round-trip** (new case in `tst_daemon_integration.cpp` or a codec test): encode
   `encodeSessionUpdateMetaRequest(id, pinned=true)`, decode with the generated
   `cbor_decode_api_request` and assert `api_request_choice == …_session_update_meta_m_c`, the session
   string, and that only `session_meta_patch_pinned_present` is set with the bool arm. Repeat for
   archived and title. Guards the tri-state optional encoding.
2. **Repository happy path**: canned `Ok` reply → `updateSessionMeta(id, pinned=true)` → assert a Call
   went out and an authoritative `SessionsQuery` refetch followed (request count ≥ 2), no
   `metaUpdateFailed`.
3. **Repository failure surfacing**: canned `Error` reply (e.g. `Forbidden`) → assert `metaUpdateFailed`
   fires with the node message and the cache row is unchanged (no local mutation) — the OpenClaw
   "no implied policy" assertion. Plus a dead-transport case (mirrors `sessionCreateErrorSurfaces`).
4. **Store delegation**: `CachedSessionStore::setPinned/setArchived/renameSession` invoke
   `SessionRepository::updateSessionMeta` with the right optionals (spy on the emitted request via the
   WireMuxServer, or a seam) and do NOT write the cache directly.
5. **Roster/Dashboard**: register + run the existing `tst_daemon_roster_dashboard.cpp` (B1). Add a
   focused `tst_app_service_graph.cpp` assertion that daemon mode now yields `DaemonSessionRoster` /
   `DaemonDashboard` (qobject_cast) rather than the mocks.
6. **Parity**: rely on the shared-seam construction (no per-surface logic) + the offscreen ctest of the
   TUI pages; no fabricated logic to test twice.

---

## 5. Gate (from worktree root, all via the devShell — no host tools)

```
nix develop --command just lint          # rustfmt/clippy/clang-tidy/clang-format/qmllint/secrets/spell
nix develop --command just build-all
# app-scoped (matches daemon-app/AGENTS.md required gate):
nix develop --command bash -lc 'cmake -B build -G Ninja -DBUILD_TESTING=ON -DDAEMON_APP_TUI=ON && cmake --build build'
nix develop --command bash -lc 'QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure'
```

No `qsTr()`/`tr()` string changes are anticipated in Part A (write path is model-level). Part B1 adds
no new user strings (reuses existing roster/dashboard roles). **If** D2 (failure toast) or a Part-B UI
notice adds strings, run `cmake --build build --target update_translations` + refresh all 12
`i18n/daemon-app_*.ts` catalogs and re-run `bash scripts/i18n-drift.sh` (per AGENTS.md i18n policy).
No wasm surface is touched, so the wasm gates are not required.

---

## 6. Cross-track deconfliction (sibling `app-render-honesty`)

Sibling Phase-5 track edits: `mock_daemon_config.cpp`, `DaemonApp/Pages/SafetySettingsSection.qml`,
`tui/interactive_turn_host.cpp`, `DaemonApp/Transcript/Transcript.qml`, and plumbs `allowPermanent`
(a wire change).

Files **this** track will touch (for the coordinator to deconflict):
- `src/core/daemon/cached_session_store.{cpp,h}` — mine only.
- `src/core/daemon/repositories.{cpp,h}` — mine (SessionRepository). *Potential overlap risk:* if the
  render-honesty track adds an approval/`allowPermanent` op to a repository here.
- `src/core/daemon/node_api_codec.h` + `node_api_codec/encode_requests.cpp` — **shared-risk**: the
  render-honesty track's `allowPermanent` plumbing also adds a codec encoder here. **Flag for
  deconfliction** — likely land one track first, rebase the other; both only *append* new encoders so
  conflicts should be additive/mechanical.
- `src/core/daemon/app_service_graph.cpp` + `.h` — **shared-risk**: both tracks may re-touch the daemon
  seam wiring / the `qInfo()` seam log. Flag.
- `src/core/daemon/seam_migration.h` — mine only (doc records).
- `src/core/daemon/CMakeLists.txt`, `tests/unit/CMakeLists.txt`, `tests/unit/tst_*` — additive.
- `src/tui/root_widget_wiring.cpp` — only if D2 (failure toast) is adopted; render-honesty also edits
  `src/tui/interactive_turn_host.cpp` (different file) — low risk.

I do **not** touch any of the sibling's four core files. The one real shared surface is
`node_api_codec.h`/`encode_requests.cpp` (both append encoders) and `app_service_graph.cpp` (both touch
daemon wiring) — flagged above.

---

## 7. Open decisions for reviewer (before I implement)

- **D1 — Optimistic echo vs pure node-authoritative.** I recommend **pure node-authoritative** (A3: no
  local `upsertSession`; UI updates on the Ok-driven refetch / `SessionMetaChanged`). The alternative is
  the `createSession` precedent (optimistic minimal upsert + immediate refetch) for snappier feel at the
  cost of a brief local-truth window. Which do you want? (Pure is the stricter invariant read; it makes
  offline pin/archive/rename inert.)
- **D2 — Surface `metaUpdateFailed` to the user?** A rejected pin/rename (e.g. `Forbidden`) is currently
  invisible. Adding a Kit notice (GUI) + TUI message is the honest choice but adds i18n strings and a
  small per-surface render bit. Include now, or log-only for this track and defer the UI?
- **D3 — `kDaemonApiVersion` bump (27 → 28).** The base branch regenerated the vendored codec to v28,
  but the facade constant `NodeApiCodec::kDaemonApiVersion` is still `27` (governs the connect
  handshake vs a v28 node). `SessionUpdateMeta` itself is v27-safe, so **my feature works at 27**. The
  bump is cross-cutting (the render-honesty `allowPermanent`/v28 wire work and/or a coordinator step).
  I will **not** bump it unless you direct — flagging so it isn't lost.
- **D4 — Part B2 empty-seed.** Do routing/cron/checkpoints currently render mock rows as real in daemon
  mode? If yes, add the `DAEMON_APP_MOCK_*`-gated empty seed; if they already show placeholders, the
  `seam_migration.h` + `qInfo()` honesty records suffice. I'll confirm during implementation unless you
  already know.

## 8. Explicitly out of scope (this track)

- Any daemon-node change (SessionUpdateMeta already exists; routing/cron/checkpoint node ops do not and
  are node-authority work).
- `allowPermanent`, mock-safety-settings removal, tool-output honesty (sibling `app-render-honesty`).
- Codec regeneration / editing `codec/generated` or `codec/vendor` by hand.
