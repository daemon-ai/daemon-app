# AD — the deletion package: make the mirror the ONLY data path (verified plan + honest boundary)

Package AD of the Mirror Architecture program (spec 09 §7, §6.4, §10.3, §13 M4/M5, §14; the
program's closing act). Base: `integrations/app` @ `4e69f2f` (the G2×A8 merge). Worktree branch
`mirror/ad`. Directive (human, standing): **clean refactor — no dead code left behind, no
migration/compatibility with pre-mirror versions, no legacy paths kept as "insurance" once their
replacement is proven.**

## STATUS — assessment complete; implementation NOT landed (honest boundary at Phase 1)

This ledger is the package's **evidence + execution map**, not a final-counts artifact. No code was
deleted or ported in this pass. The reason, stated plainly and backed by the code census below: AD
has **no cleanly-isolated, low-risk slice**. Every deletion thread is gated on one of two large
Phase-1 verticals or pulls the §13-ring-fenced transcript surface, and completing either vertical to
the repo's green-gate standard (GUI+TUI+tests + clang-tidy + i18n + byte-parity) is a dedicated
effort. Per the program rule invoked throughout the sibling ledgers — *an honest clean-gated partial
beats a broken total* — and the AD brief's own instruction to "land the honest boundary with clean
gates and report it precisely," this pass delivers the verified map so the resume is efficient and
correct, rather than a rushed partial that would regress the one surface the program ring-fenced.

Base gate re-confirmed green at handoff: **147/147 ctest**, GUI (`daemon-app`) + TUI (`daemon-tui`)
+ all tests build clean (only pre-existing `-Wconversion` warnings in `fetch_scheduler.h`).

## Settled decisions (verified this pass — do NOT re-litigate)

### Substrate-less build story → DELETE the fallbacks; make the substrate UNCONDITIONAL

Verified: **every shipped configuration pins immer.** `flake.nix` exports `-DIMMER_SOURCE_DIR`
in the sealed build (line 257), the default devShell (1113), AND the wasm devShell (1134, with the
explicit note "so the WASM smoke probe … can resolve it"). `cmake/Dependencies.cmake:317-341` sets
`DAEMON_APP_HAVE_MIRROR_SUBSTRATE ON` whenever `IMMER_SOURCE_DIR` resolves, OFF only when it is
absent — which no shipped preset does.

- `rg DAEMON_APP_HAVE_MIRROR_SUBSTRATE` over `src`/`tests` returns **only CMake + this/ADR prose —
  zero `#if` code guards.** "Substrate-less" is therefore NOT a compile configuration; it is the
  RUNTIME null-mirror fallback (`graph.storeMirror = legacy` when `mirrorService == nullptr`, A7's
  composition aliasing) plus the CMake `else` that skips building the mirror library.
- Per the directive (no insurance fallback): AD makes the substrate mandatory —
  `Dependencies.cmake` `FATAL_ERROR` when `IMMER_SOURCE_DIR` is unset (drop the "library skipped"
  branch), and deletes the runtime null-mirror fallback in `app_service_graph.cpp`. That deletion is
  ENTANGLED with the Phase-1 mock/legacy removals (the fallback binds `InMemorySessionStore` etc.),
  so it lands WITH Phase 1, not as a standalone step.

### Parity plumbing → legacy-leg comparisons die WITH their legacy sources; keep mirror-internal

- Delete: `tst_mirror_transcript_parity`'s legacy leg (`CachedSessionStore::content()` oracle),
  `parity::sessionKeys`/`fleetUnitKeys` legacy legs, the derived-legacy-baseline assertions in
  `tst_mock_daemon_parity` (`graph.store->content` / `daemonLegacy->content` comparisons) and the
  seam-fix's derived legacy baseline in `mock_scenario.cpp`.
- Keep: `projectTranscriptBlocks` (load-bearing — the mirror projection itself), and mirror-internal
  invariants (journal-replay ≡ snapshot-diff §4.4; mirror-vs-mirror feeder parity, i.e. the
  Seeder-vs-Ingestor equality in `tst_mock_daemon_parity` that does NOT reference a legacy source).

## The two Phase-1 verticals that gate EVERYTHING (verified seam census)

### Phase 1a — integrations tree + TUI channels hub onto mirror projections (the A8 census blocker)

This is a NEW fetch/deliver/verb vertical, not a re-bind. `IntegrationsTreeModel`
(`src/DaemonApp/Sidebar/integrations_tree_model.h`) composes DIRECTLY from `ITransportRegistry` +
`IPersonsService` (its header says so); the TUI channels hub + contact/room flows read the same
seams in BOTH modes. The mirror model already HAS the tables (`model.h`: `adapters`,
`transport_accounts`, `persons`, `person_endpoints`, `rooms`, `contacts`, `conversations`,
`conversation_members`) — the GAP is the FEED and the VERBS:

- **Adapters (catalog)**: no `FetchOp`, no `decodeAdaptersToMirror`, no `deliverAdapters`. The
  "Add channel" picker (`availableAdapters()`) is connect-only today (`TransportRepository::
  refreshAdapters` → `m_adapters`, never cached). Add the fetch arm (ProfileList-shaped: small
  rev-gated list) + deliver (global replace-and-prune over `adapters`).
- **Transport instances (accounts)**: `transport_accounts` is fed ONLY by `TransportChanged`
  PatchInPlace (`ingestor::patchTransportAccount`), never a full list. Add a `TransportInstances`
  `FetchOp` + `decodeTransportInstancesToMirror` + `deliverTransportAccounts` (global replace-and-
  prune) so the tree's account roots have a full baseline on connect, not only patched deltas.
- **Person ENDPOINTS**: `deliverPersons` feeds `persons` but NOT `person_endpoints` (the tree's
  per-transport person sections, `PersonEndpoint` table). `map_person`/`decodePersonsToMirror` must
  also emit the endpoints (the wire `person` carries `endpoints: [* person-endpoint]`), and a
  `deliverPersonEndpoints` (or extend `deliverPersons` to write both tables under one batch) with
  the `endpointsByPerson` §3.5 relation index the tree groups by.
- **Room lifecycle (create/join/leave/member*)**: these are VERBS, not reads. Route per §7 — direct
  verb seams (ConvCreate/ConvJoin/Member* are in 06G4's op-id priority set, §6.4: direct + op_id,
  NOT outboxed) unless the §6.4 census declares a lane. Reads (room lists) already live on the mirror
  since A6 (`rooms` table, `deliverRooms`).
- Then port `IntegrationsTreeModel` + TUI channels hub + contact/room flows to read the mirror
  projections (re-derive on `Store::committed`, watermarked §8.1) in BOTH modes; mock feeds via the
  Seeder scenario (extend `SeedSet`/`mock_scenario.cpp` for adapters/instances/endpoints if a kind
  is unseeded).
- **Then DELETE** `MockTransportRegistry`, `MockPresenceService`, `MockPersonsService`,
  `MockContactsService` (+ headers, CMake in `src/core/transports/CMakeLists.txt`, tests). Re-express
  still-relevant behaviors on the mirror path. `DaemonTransportRegistry`/`DaemonPersonsService`/etc.
  survive as the wire fetch/verb feeders (the mirror's daemon-mode source), not as read seams.

  Effort: HIGH — a post-M5 vertical the executed M2–M4 waves deliberately never covered (A8's
  survivor rationale: "the enabling work is a consumer port + ingestor fetch arms — a post-M5
  vertical"). Fidelity: the tree render (spaces/rooms/DMs hierarchy, presence, capability-governed
  sections) must match on both surfaces.

### Phase 1b — mutation re-homing off the legacy session machinery + engine transcript dual-write

`MirrorSessionStore` mutations (`src/core/daemon/mirror_session_store.cpp:269-345`) ALL delegate to
`m_legacy` (daemon: `CachedSessionStore` → `SessionRepository` wire ops; mock: `InMemorySessionStore`).
Move each to its §7 path:

| Mutation | §6.4 / §7 disposition | Wire verb |
|---|---|---|
| `setPinned` / `setArchived` / `renameSession` | OUTBOXED — `session-meta` lane (§6.4), latest-wins per (verb, session) | `SessionUpdateMeta` |
| `newSession` / `requestNewSession` | DIRECT + op_id (not an outbox lane) | `SessionCreate` |
| `deleteSession` | DIRECT + op_id | `SessionDelete` (verify verb name in codec) |
| `moveSession` | client-local reorder over pinned/last-activity — no wire verb (confirm; the projection order is deterministic already) |
| `createUnit` / `createTag` | daemon: unsupported / client-local sidecar (return empty as today) |

Required plumbing (verified gaps):
- **op_id on the encoders**: `SessionRepository::updateSessionMeta`/`createSession` build the request
  WITHOUT `op_id` today. §10.3 adds `? "op_id"` to `session-update-meta` + `SessionCreate`. Mint per
  §6.2/§10.3 and thread through `NodeApiCodec`.
- **Outbox lane sender**: the `Outbox` (`src/core/local/outbox.h`) is a pure state machine —
  `enqueue(verb, scope, payload, display)` → on `drain()` emits `sendRequested(op)`; the OWNING verb
  seam must perform the wire call and report `onAck`/`onRejection`/`onTransientFailure`. A8 wired the
  `chat-send` lane (`ConvSendController.laneActive`) as the precedent; the `session-meta` lane needs
  its own `sendRequested` handler dispatching `SessionUpdateMeta` and a §6.5 rejection surface
  (GUI toast + TUI notice + the outbox lens) with GUI+TUI parity.
- **Provenance landing for sessions**: `deliverSessions`/`deliverSessionsDelta` carry NO `origin_op`
  (unlike `deliverConversations`). §10.3 gives `session-page.origin_ops` + `SessionMetaChanged.
  origin_op`. Either thread it (so `MirrorService::provenanceStamped` lands the op) OR accept the
  §6.6 accepted-state grace fallback (degraded, never heuristic) and document it. Threading is the
  clean-refactor outcome.
- **The engine transcript CACHE dual-write** (`daemon_turn_engine.cpp`): retire the legacy leg so the
  mirror sink is the single write path (byte-parity-proven since G2). Sites:
  - `persistTranscriptBlock` (l.977-979): drop `m_cache->upsertTranscriptBlock(block)`; keep
    `m_mirrorSink->deliverBlock`.
  - `applyLogPage` rebaseline (l.825-827): drop `m_cache->clearTranscript`; keep `m_mirrorSink->clear`.
  - `checkpointReasoningBlock` (l.989) gates on `m_cache == nullptr`; re-gate on session/sink so a
    substrate stack persists reasoning via the sink (audit `settleReasoningBlock` for the twin gate).
    `m_cache` STAYS for cursor watermarks (`sessionWatermarkScope`/`sessionEpochScope`) + child
    title/endReason reads (`m_cache->sessions()`/`fleetUnits()`) — those are separate legacy reads
    that die with the session-machinery deletion, not here.
  - **Ring-fenced test rewrites REQUIRED in the same change**: `tst_sync_resync`
    (`toolFinished…`/`journalReplayCarriesToolDetail`) construct `DaemonTurnEngine(&client, &cache)`
    with NO mirror sink and assert `cache.transcriptBlocks("s1")`. Post-retirement these MUST drive
    the engine WITH a `MirrorTranscriptSink` and assert the mirror window
    (`MirrorSessionStore::mirrorContent`). This is the §13-M4-ring-fenced surface — treat as
    fidelity-critical.

## Phase 2 — the deletion sweep (gated on Phase 1; the accumulated order, reference-verified)

Each behind a `rg -n "<Symbol>" src tests` gate (only ledger/doc prose may remain), small verified
batches. Dependencies noted.

1. **Session machinery** (gated on 1b mutation re-homing):
   - `CachedSessionStore` (`cached_session_store.{h,cpp}`) — daemon-mode mutation delegate + the
     (now production-dead, parity-only) `content()` projection. Its ISessionStore `content()` is
     interface-mandated, so it cannot be gutted while the class lives — it dies WHOLE once mutations
     re-home. Last row-reader already gone (A7); last content-reader is the parity harness.
   - `SessionRepository` (`repositories.{h,cpp}`) — the wire mutation sender + legacy cache feeder.
     Its READ path (`refreshSessions*`, `cachedSessions`, page loops) dies; its op-sending core is
     what the outbox `session-meta` handler + the direct `SessionCreate`/`SessionDelete` seams
     replace (move the encode/decode, delete the cache write).
   - `daemon_session_roster.{h,cpp}` legacy remnants (verify against `DaemonSessionRoster`'s current
     mirror-served state — A7 flipped its projection to `storeMirror`).
   - **SubscriptionManager arms** (`subscription_manager.{h,cpp}`): retire arms whose consumers are
     ALL mirror-served. Verify EACH against the routing-table census in the header/07§5.1. If ALL
     arms are ported, delete `SubscriptionManager` itself. NOTE the entanglement: its
     `MembershipChanged` self-removal arm (l.244) calls `m_routing->refreshChats()/refreshRooms()`
     — repoint to `ingestor.refetchRouting()/refetchRooms()` (SubscriptionManager needs the ingestor,
     or route via a graph-wired signal) as part of the routing-cache deletion below.
   - construction-order patch-ups that die with the repos (07§9.8).
   - `InMemorySessionStore` — mock legacy store; dies with the null-mirror fallback (substrate
     decision above) once the Seeder feeds mock (A8 already flipped mock `storeMirror` to a real
     `MirrorSessionStore`; `InMemorySessionStore` remains only as its delegate — see A8 survivors).

2. **Transcript cache** (gated on 1b engine retirement):
   - `DaemonCacheStore::transcriptBlocks()` / `upsertTranscriptBlock()` / `clearTranscript()` +
     `client_cache_schema.h` transcript-table DDL + `CachedTranscriptBlockRow` uses. Blast radius
     (verified `rg`): rewrite `tst_mirror_transcript_parity` (drop legacy leg → mirror golden +
     mirror-internal invariants), `tst_subscription_manager` (the `transcriptBlocksRenderFromCache`/
     `clearTranscriptWipesSession` block → re-express on the mirror), `tst_sync_resync` (see 1b).
     KEEP the non-transcript `daemon_cache_store` regions with live readers (credentials/models/fs/
     sessions cursors — OUT of program scope; delete only what the mirror replaced).

3. **FleetRepository tree feed** (G2 residual; independent of 1a): delete `cachedUnits()`,
   `syncFleetUnits()`, the `daemon_fleet_units` table + `client_cache_schema.h` v4 DDL,
   `CachedFleetUnitRow`, `treeRefreshed`'s dead-cache warm. The control seam SURVIVES
   (`pause`/`resume`/`scale` + `controlFailed`). Re-point the control-ack refetch: `MirrorFleetTree`
   already calls `ingestor.refetchFleet()`; drop the `treeRefreshed→refetchFleet` bridge (by then
   FleetChanged-only is proven) or keep it firing `refetchFleet` directly.

4. **RoutingRepository dead in-memory cache** (LEDGER-a6 residual): delete `m_routes`/`routes()`/
   `refreshChats()`/`getRoute()`/`setRoute()`/`routeResolved`/`m_rooms`/`roomsFor()`/
   `refreshRooms()`/`roomsRefreshed`/the page loops + response handlers. KEEP `routingBindChat`/
   `routingUnbindChat` (`IRoutingActions` — the direct mutation seam). ENTANGLEMENT: `refreshChats()`
   is the "dead-cache warm that fires `routesRefreshed` → the graph forwards to
   `ingestor.refetchRouting()`" trigger (`app_service_graph.cpp:444-447,707`) AND is called from
   `subscription_manager.cpp:244`. Replace the trigger with a DIRECT `ingestor.refetchRouting()` from
   the mutation `Ok` handler + the connect-ready path (the staleness scan already fetches routing);
   remove the double-fetch. `getRoute`/`setRoute`/`routeResolved` are already ZERO-reference (no
   caller in `src` or `tests`) — the safest first micro-batch of this deletion.

5. **Substrate-less fallbacks** (see settled decision): `Dependencies.cmake` FATAL when immer
   absent; delete the runtime null-mirror fallback + `MockFleetSource` (its `.content` seeds are
   derived; when the legacy delegate dies, the seeder's derived-baseline leg + these classes go) +
   `MockFleetTree`. Substrate becomes unconditional.

6. **Parity plumbing** (see settled decision): delete legacy-leg comparisons; keep
   `projectTranscriptBlocks` + mirror-internal invariants.

7. **Straggler sweep**: `rg "TODO.*(mirror|legacy|A[0-9])"`, dead includes, helpers orphaned by the
   deletions. Qt signals/slots + QML-invoked methods can look unreferenced — check QML usages before
   deleting. NO pre-mirror migration/compat anywhere (no `daemon_cache.db` → mirror import, no
   old-schema shims, no env-var aliases).

## Dependency graph (why the order is forced)

```
Phase 1a (tree/hub on mirror) ─────────────▶ delete Mock{Transport,Presence,Persons,Contacts}
Phase 1b mutation re-homing ──┬────────────▶ delete CachedSessionStore + SessionRepository read/feed
                              ├────────────▶ retire SubscriptionManager session/roster arms
                              └── engine ───▶ delete DaemonCacheStore transcript methods + parity legacy leg
FleetRepository tree feed (G2) ── independent of 1a; re-point control-ack refetch
RoutingRepository dead cache  ── independent of 1a; but re-point trigger (touches SubscriptionManager arm)
Substrate unconditional       ── entangled with the null-mirror fallback → lands WITH Phase 1
Parity plumbing               ── dies WITH each legacy source it compares
```

## End-of-package evidence owed to A9 (F6 audit vs 07 baselines: 31 seams / 20 repos / 5+6 layouts)

When the phases land, this ledger must record: deleted classes/files/LOC; survivors + reason
(FleetRepository CONTROL seam; the non-transcript `daemon_cache_store` domains out of program scope —
credentials/models/fs; the Daemon* transport/persons feeders as the mirror's wire source; the
post-M5 domain mocks A8 listed as blocked); zero-reference gate outputs per batch; the substrate
decision (settled: unconditional); parity-plumbing disposition (settled). Ctest trajectory will move
DOWN as legacy suites delete and MIRROR suites absorb their still-relevant assertions — track and
explain each step.

## Handoff note for the parent / next resume

Start with Phase 1b's mutation re-homing (it gates the largest deletions and is more self-contained
than 1a): land the `session-meta` outbox lane + `SessionUpdateMeta` op_id + the `sendRequested`
handler + §6.5 rejection UX (GUI+TUI) FIRST, gate green, commit; then the direct
`SessionCreate`/`SessionDelete` op_id seams; then the engine transcript retirement WITH the
`tst_sync_resync`/`tst_subscription_manager`/`tst_mirror_transcript_parity` rewrites, gate green,
commit. Phase 1a (tree vertical) is the larger, later effort. Every commit stays in this worktree
(`mirror/ad`); the parent merges.
