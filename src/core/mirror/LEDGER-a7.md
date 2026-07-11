# A7 — M4 sessions/fleet unification (six session shapes → one `Session`): TDD ledger

Package A7 of the Mirror Architecture program (spec 09 §13 wave M4; §4, §5, §8, §14; 07§9.3/§9.8).
Base: `integrations/app` @ `70e5ec1` (the A6 merge). Worktree branch `mirror/a7`. This is the
HIGHEST-BLAST-RADIUS wave: it unifies the six divergent session row shapes onto the ONE generated
`Session` entity, unifies `FleetUnit`, and re-homes the transcript blocks LAST behind their own
sub-gate. Worked consumer-by-consumer with per-consumer sub-gates — never a big-bang cutover.

## Seam census at base (verified by grep)

Legacy session/fleet machinery (`src/core/daemon/`, `src/core/persistence/`, `src/core/fleet/`):

- `persistence::ISessionStore` (`isession_store.h`) — the data seam the GUI/TUI session consumers
  bind to; produces `domain::Session` / `domain::UnitNode` / `domain::Tag`.
- `CachedSessionStore : persistence::ISessionStore` (daemon mode) + `InMemorySessionStore` (mock).
- `SessionRepository : RepositoryBase` (`repositories.{h,cpp}`), `daemon_session_roster.{h,cpp}`,
  `daemon_fleet_tree.cpp`, `daemon_model_catalog.{h,cpp}`, `subscription_manager.{h,cpp}`
  (SessionRepository consumer; mirror ingestor runs DUAL-WRITE beside it since A5).
- `daemonnet::MockFleetSource` (A6's non-IDaemonNet mock seed) feeding
  `MockSessionRoster`/`MockFleetTree`/`InMemorySessionStore`.
- `src/DaemonApp/Turn/daemon_turn_engine.cpp` — the transcript single-writer, re-homed LAST.

Session/fleet CONSUMERS (the sub-gate targets):
1. roster list — `SessionsList/sessions_list_model.{h,cpp}` (GUI) + `src/tui/session_list_view.h`.
2. sidebar — `Sidebar/sidebar_model.{h,cpp}` (GUI) + TUI `tui_page_hub` / `root_widget*`.
3. pickers — session pickers (finder/attach, tab pickers).
4. detail pane — `Session/session_controller.{h,cpp}`, `Participants/participants_model.{h,cpp}`.
5. transcript chrome — transcript export/render surfaces.
6. transcript blocks / engine stream — `daemon_turn_engine.cpp` (own sub-gate, LAST).

Generated entities ready in the store: `Session` (key `session`) + `FleetUnit` (key `id`) in
`generated/entities_gen.h`, mapped in `entities_map.cpp` (bodies were TODO stubs at base), tables
`sessions` / `fleet_units` in `model.h`. Policy arms already declared (A4): `RosterChanged`→
`SessionsQuery`, `SessionMetaChanged`→`SessionGet`, `FleetChanged`→`Tree`. `sessions`/`fleet`
collections already in `SyncState::allCollections()` + `scopeToCollections("roster")`.

## Sub-step 0 — the fetch/ingest vertical (parity infrastructure; unblocks every consumer)

Before any consumer can read the mirror, the `sessions` / `fleet_units` tables must actually be
fed. A4 declared the policy arms; A5/A6 built the executor for chat/routing but LEFT sessions/fleet
falling to the executor's default no-op (dual-write: legacy repos still served them). Sub-step 0
completes the vertical end-to-end, following the A5/A6 pattern (encode → mirror_decode → deliver →
store), landing beside the legacy repos (dual-write, §13 M4 discipline):

- `map_session(::session_info)` / `map_fleet_unit(::unit_node)` bodies (were TODO stubs) — the
  single wire→canonical point (§3.3), per `entity-map.toml` provenance; enum→string helpers named
  once (session-state, lifecycle, session-role, unit-kind, unit-state, delegation-lifetime).
- `decodeSessionsToMirror` (SessionsQuery→`response_sessions`, full list),
  `decodeFleetUnitsToMirror` (Tree→`tree_report`, page-looped, carries `rev`),
  `decodeSessionDetailToMirror` (SessionGet→`session_detail`, hydrates model+checkpoints onto the
  base `session_info`) — all confined to the daemon codec/bridge (07§10).
- ingestor `deliverSessions` (GLOBAL replace-and-prune over `sessions`, §5.3 PageLoop),
  `deliverFleetUnits` (replace-and-prune over `fleet_units`, records `rev` for the FleetChanged
  rung-1 skip-gate), `deliverSession` (single keyed upsert = SessionGet hydration merge). Single
  writer preserved (§5.1); diff-before-write suppression (§5.3) rides the substrate.
- `DaemonFetchExecutor` arms for `SessionsQuery` / `Tree` (page loop) / `SessionGet`.

Invariants under test (`tst_mirror_sessions_fleet`):
- §3.3 decode: a wire Sessions/Tree/SessionDetail reply maps into typed `Session`/`FleetUnit`
  entities (enum strings, nullable-opt handling, hydration merge).
- §5.3 PageLoop: `deliverSessions`/`deliverFleetUnits` replace-and-prune (a later list dropping a
  key tombstones it); `deliverSession` upserts without pruning siblings.
- §5.5 rev-gate: `deliverFleetUnits` records the tree `rev` so a rev-unchanged FleetChanged skips.
- §5.1 single writer: only the ingestor writes `sessions`/`fleet_units`.

## Sub-gate protocol (M4 discipline)

Order: (1) roster list, (2) sidebar, (3) pickers, (4) detail pane, (5) transcript chrome,
(6) transcript blocks/engine re-homing (own sub-gate, LAST). Per sub-step: RED → port (GUI+TUI) →
build + ctest green → parity assert vs the legacy source stays clean (dual-write keeps both live)
→ commit. A failed sub-step is revertible without unwinding prior sub-steps. Legacy deletion only
when a cache/repo/store's LAST reader is ported and its parity assert has been green.

## The consumer-cutover mechanism (parent-approved design)

- **`MirrorSessionStore : persistence::ISessionStore`** (`src/core/daemon/mirror_session_store.*`):
  session ROWS/counts/title/pinned are PURE projections of the mirror `sessions` table (deterministic
  order: pinned, last-activity desc, id; re-derived on `Store::committed` filtered to Session/
  FleetUnit journal deltas above a registered watermark). `content()`/`setContent()` DELEGATE to the
  legacy store (transcript re-homing is sub-gate 6). Mutations + `requestNewSession` DELEGATE to the
  legacy store (ONE node-authoritative wire path; `sessionCreated`/`metaUpdateFailed` relayed).
  Scoped refreshes hit BOTH sides (legacy cache + ingestor mirror fetch — dual-write).
- **Mock fallback = composition-time aliasing (the least-magic option, chosen per parent ruling):**
  `AppServiceGraph.storeMirror` is the ported consumers' binding. Mock mode / substrate-less stacks:
  `storeMirror = store` (the legacy in-memory store) — zero per-consumer conditionals, mock keeps
  rendering (§9) until A8's seeder feeds the mock mirror. Daemon+substrate: the distinct
  `MirrorSessionStore`. GUI context property `SessionStoreMirror` beside `SessionStore`; TUI reads
  `m_services.storeMirror`. Asserted in tst_app_service_graph.
- **Scoped roster reads on the mirror path**: `Ingestor::refetchSessionsForProfile/-Archived/
  -ForTransport` → `FetchOp::SessionsQuery` with scope `"profile␟<id>"`/`"archived"`/
  `"transport␟<id>"` → additive delivery (`deliverSessionsAdditive`; ByTransport also emits
  `transportSessionsResolved` — the store projects the node-resolved id set, B4). The TopLevel
  replace-and-prune SPARES archived rows (the legacy pruneSessionsMissingFrom rule, F6).
- **Wire fixes vs sub-step 0**: `decodeSessionsToMirror` now decodes **SessionPage** (SessionsQuery's
  actual reply per dispatch.rs; the bare `Sessions` arm answers the old Sessions verb) with
  next_cursor page-loop + rev + removed; the executor page-loops full reads, applies since_rev
  deltas via `deliverSessionsDelta` (rev-went-backwards ⇒ node's unservable fallback ⇒ replace,
  the legacy applySessionPage rule). `Ingestor::onBootstrap` normalizes the node's `"roster"` rev
  key → `"sessions"` so the M4 delta read fires on an api/39 connect (the per-transport
  `conv:<t>`/`contacts:<t>` keys remain unmapped — pre-M4 posture, A8/A9).
- **Dual-decoder parity (the §13 M4 assert)**: tst_mirror_session_store feeds the SAME SessionPage
  bytes through the legacy path (decodeSessionPage → DaemonCacheStore → CachedSessionStore) and the
  mirror path (decodeSessionsToMirror → ingestor → MirrorSessionStore) and asserts row-set equality
  (`parity::sessionKeys`) + view equality (ids/counts/titles/pinned/pinned-floats-first).

## Status / boundary (updated as sub-steps land)

- **Sub-step 0 (fetch/ingest vertical) — DONE, gated green.** `sessions`/`fleet_units` mirror tables
  are fed end-to-end in daemon mode, beside the legacy repos (dual-write). ctest 136→137.
- **Parity infrastructure — DONE, gated green.** `parity::sessionKeys` / `parity::fleetUnitKeys`
  extract the mirror row-set for comparison against the legacy `CachedSessionStore` /
  `SessionRepository` / fleet-tree row-set (spec §13 M4 "parity asserts vs legacy roster until
  deletion"). Covered in `tst_mirror_parity`.

### Precise boundary handed to the next A7 continuation / A8

The consumer cutover sub-gates (1 roster list → 2 sidebar → 3 pickers → 4 detail → 5 transcript
chrome → 6 transcript-engine re-homing) are NOT yet landed. They are deliberately deferred rather
than rushed: this is the highest-blast-radius surface and a half-finished cutover would break a
large surface (the task's "honest partial with clean gates beats a broken total"). The precise
seam that makes them a large, discrete step:

- **Single shared store injection.** All session consumers bind to ONE context property
  `SessionStore` (`application.cpp:253`, `engine.rootContext()->setContextProperty("SessionStore",
  m_services.store)`) which is the legacy `CachedSessionStore`/`InMemorySessionStore`. The GUI QML
  and the TUI both read the same object, so a per-consumer sub-gate CANNOT flip one consumer by
  reassigning the shared property. The mechanism for the sub-gate protocol is: introduce a
  mirror-backed `persistence::ISessionStore` (reads `Store::snapshot().sessions` + `fleet_units`,
  re-derives on `Store::committed`), expose it as a SECOND context property, and rebind ONLY the
  ported consumer's `store:` (QML) + its TUI wiring — keeping the legacy `SessionStore` live for the
  unported consumers (dual-write) and for the parity assert. Delete the legacy store only when its
  LAST reader is ported.
- **Shape gaps the mirror Session does NOT carry (documented degradations, thin-client correct).**
  The legacy `domain::Session` fused node facts with client-local + transcript data: `content`
  (markdown snippet — that is the transcript/chat window, not a roster fact), `tagIds` (client-local
  cross-cutting labels — sidecar, no node concept), and `unitId` (unit membership — derivable from
  `fleet_units.session`). A mirror-backed roster renders snippet/tags empty until a client-local
  sidecar (tags) and the `unitsByParent`/`sessionsByProfile` §3.5 relation indexes are added; this
  matches the A5 canonical-shape degradation precedent. Mock mode has a null `Mirror` (A5/A6
  precedent) so a ported surface renders empty in mock until A8's seeder feeds the mock mirror.
- **Transcript re-homing (sub-step 6, LAST).** The turn-engine (`daemon_turn_engine.cpp`) is still
  the single writer of transcript blocks; `TranscriptBlock`'s `map_transcript_block` remains a TODO
  stub and the `w_transcript_blocks` legacy table is frozen (entity-map.toml note). Untouched here —
  its own gate, per spec.

### Provenance-landing decision

Deferred (left for A8, as A6's LEDGER permits). This package did not touch the outbox landing seam
(`Outbox::onProvenanceStamped` / `setProvenanceCapable`) — the session/fleet fetch vertical is a
read path (ingestor deliver), it does not naturally cross the outbox provenance seam, so per the BR
worker's instruction it is correctly left for A8.
