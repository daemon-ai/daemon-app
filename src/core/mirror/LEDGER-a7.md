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

## Status (all gates green at each step)

- **Sub-step 0 (fetch/ingest vertical) — DONE.** `sessions`/`fleet_units` fed end-to-end in daemon
  mode beside the legacy repos (dual-write). ctest 136→137.
- **Parity infrastructure — DONE.** `parity::sessionKeys`/`fleetUnitKeys` + the dual-decoder
  parity assert (same SessionPage bytes through both paths). ctest →138.
- **Mechanism — DONE.** `MirrorSessionStore` + `storeMirror`/`SessionStoreMirror` composition
  binding + scoped roster fetches + SessionPage wire fix + bootstrap "roster"→"sessions" key map.
- **Sub-gate 1 (roster list) — DONE.** SessionsList.qml + ArchivedSection.qml + TUI m_list +
  wireSessionList actions on the mirror store. `tst_sessions_list_mirror`. ctest →139.
- **Sub-gate 2 (sidebar) — DONE.** Sidebar.qml + TUI m_sidebar. Counts + Fleet membership
  (ByProfile leaves) project mirror rows. `tst_sidebar_mirror`. ctest →140.
- **Sub-gate 3 (pickers/tab chips) — DONE.** Session.qml (creator/_titleFor/dialogs) + TUI tab
  title resolution, /title + /save, notification titles. `tui_tab_title_mirror_tests`. ctest →141.
- **Sub-gate 4 (detail pane) — DONE.** TranscriptPage.qml + Participants.qml + TUI
  TabSessionManager + m_participants. Content renders via the DELEGATED legacy transcript source.
- **Sub-gate 5 (transcript chrome) — DONE.** Transcript.qml loadTranscript + Main.qml meta toast +
  exporter (mirror title + delegated content composed through ONE store).
- **Ops-hub roster — DONE.** DaemonSessionRoster (GUI Fleet/Sessions page + TUI ops hub) projects
  storeMirror; constructed after the mirror block. Fully mirror-served row shape (no degradation).

Result: ZERO daemon-mode session-row consumers read the legacy CachedSessionStore. Every session
row/count/title/pin on every surface (GUI + TUI) projects the ONE generated mirror::Session — the
6→1 unification of 07§9.3. Mock mode renders unchanged through the composition fallback.

## Legacy machinery: deleted vs retained (the exact A8/A9 boundary)

RETAINED (each with a live, load-bearing role in the dual-write window):
- `CachedSessionStore` — no longer a row source for ANY consumer; retained as (a) the DELEGATED
  transcript-content source (`content()` msg-fence projection over cache transcript blocks — dies
  with sub-step 6), (b) the delegated node-authoritative mutation path (pin/archive/rename/create →
  SessionRepository wire ops), (c) the parity baseline + rollback path (spec §13: keep the legacy
  path one more sub-wave; rebinding a consumer back to `SessionStore` is a one-line revert).
- `SessionRepository` — the wire mutation sender + the legacy cache feeder (dual-write baseline).
  Its cache feed must outlive the parity window; retire with CachedSessionStore.
- `SubscriptionManager` session/roster arms — NOT retired: they feed the legacy cache that the
  parity baseline + content delegation still read. Retiring them starves the dual-write baseline;
  they die together with CachedSessionStore/SessionRepository after sub-step 6 + the mutation
  outbox lane land.
- `daemon_fleet_tree` (IFleetTree) — still cache-fed: mirror `FleetUnit` carries no parent/children
  edges (the §3.5 `unitsByParent` relation index needs an entity-map/codegen change — G-series,
  `just update-codec`, out of A7's charter). The fleet TREE stays legacy; the fleet MEMBERSHIP
  view (sidebar) is mirror-served.
  > **G2 update: DELETED.** The FleetUnit entity now carries the `child_ids` edge (+
  > engine/engine_agent); `fleet::MirrorFleetTree` projects the mirror table into the same
  > IFleetTree rows (GUI + TUI bind unchanged) and `daemon_fleet_tree.{h,cpp}` + its test are
  > GONE. `FleetRepository` survives as the control seam (pause/resume/scale); its now
  > reader-less tree feed (`cachedUnits`/`syncFleetUnits`/`daemon_fleet_units`) is AD's — see
  > LEDGER-g2 "Fleet legacy left for AD".
- `MockFleetSource` + `InMemorySessionStore` — mock mode's seed/store; the composition fallback
  binds them as `storeMirror` in mock. A8's seeder replaces this aliasing with a seeded mock mirror
  and can then delete the fallback.
- The `SessionStore` context property stays registered (zero QML readers) as the rollback binding.

DELETED: nothing this package — by design. Deletion is gated on sub-step 6 (transcript re-homing)
plus the mutation outbox lane; deleting the legacy store now would sever the delegated transcript
content and the parity baseline (spec: never leave a consumer sourceless).

## Sub-step 6 (transcript re-homing) — NOT attempted; the precise seam

Assessed and deliberately left behind its own gate (the parent-sanctioned partial). The seam:

- WRITE: `DaemonTurnEngine` (src/DaemonApp/Turn/daemon_turn_engine.cpp) is the transcript single
  writer — ~6 callsites append `CachedTranscriptBlockRow`s (Message/Reasoning/ToolCall/ToolResult
  variants) to `DaemonCacheStore::upsertTranscriptBlock`, with live coalescing rules (reasoning
  deltas accumulate into ONE block per run, settled by the next content event; todo-tool blocks
  suppressed; ToolResults folded into their call's card).
- READ: `CachedSessionStore::content()` projects those rows into byte-stable msg-fence transcript
  markdown (~100 lines: cache-seq-derived ids, assistant-turn grouping, tool-status folding) that
  the BlockEditor re-parses into the SAME cards as the live document.
- RE-HOMING = a NEW writer seam: unlike every mirror write so far (wire fetch/event decode via the
  ingestor), the engine is an app-local stream consumer. It must hand blocks to the ingestor (the
  single mirror writer, §5.1) as `TranscriptBlock` window items (`w_transcript_blocks`, scope =
  session, window key = seq), `map_transcript_block` gets a real body, and the content() projection
  re-implements the msg-fence serialization over mirror windows — byte-stable, or every reload
  reflows the user's transcript. Plus offline-reload parity (mirror.db) and the existing turn/
  transcript suites. That is a package-sized, fidelity-critical vertical; landing it at the tail of
  this package risks the one surface the spec ring-fenced. The 6→1 consumer unification (this
  package's deliverable) does not depend on it: content() already flows through the ONE store
  facade, so sub-step 6 later swaps the facade's delegation for a mirror projection with zero
  consumer churn.

## Provenance-landing decision

Deferred to A8 (unchanged): this package's writes are read-path deliveries (ingestor); nothing
crossed the outbox landing seam (`Outbox::onProvenanceStamped`/`setProvenanceCapable`).

## Notes for A8 (mock/seeder) and A9 (audit)

- A8: seed the mock mirror, then flip the composition fallback (`graph.storeMirror` in mock) to a
  `MirrorSessionStore` over the seeded store; the legacy `InMemorySessionStore` + `MockFleetSource`
  become deletable when the mock transcript path is also seeder-fed.
- A8/A9: the bootstrap rev-key map normalizes only `"roster"`→`"sessions"`; the per-transport
  `conv:<t>`/`contacts:<t>` keys still fall through to no-op (pre-M4 posture) — wire them when the
  per-transport delta reads join.
- A9: the deletion order once sub-step 6 + the mutation lane land: engine re-home → content()
  mirror projection → drop the CachedSessionStore delegation (facade reads go pure-mirror) →
  retire the SubscriptionManager roster arms → delete CachedSessionStore + the SessionRepository
  cache feed → drop the `SessionStore` context property.

> **G2 update (deletion-order progress):** the first THREE rungs are DONE — the engine re-home
> (A7T), the content() mirror projection (A7T), and the delegation drop (G2's flip: facade
> transcript reads are pure-mirror; mutations still delegate by design). Newly deletable by AD,
> in order: (1) the engine's legacy cache dual-write + `DaemonCacheStore::transcriptBlocks` +
> `CachedSessionStore::content()`'s projection — together, since the parity harness consumes all
> three as its baseline; (2) the SubscriptionManager roster arms; (3) CachedSessionStore +
> SessionRepository cache feed + the `SessionStore` context property; (4) the FleetRepository
> tree feed + `daemon_fleet_units` (G2 deleted its reader — see LEDGER-g2), keeping the control
> ops. The mutation-outbox lane remains the gate for (2)/(3) (unchanged).
