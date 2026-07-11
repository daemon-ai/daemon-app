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

## Status / boundary (updated as sub-steps land)

- Sub-step 0 (fetch/ingest vertical): see commits below.
