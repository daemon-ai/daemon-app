# AD — the deletion package: make the mirror the ONLY data path (verified plan + honest boundary)

Package AD of the Mirror Architecture program (spec 09 §7, §6.4, §10.3, §13 M4/M5, §14; the
program's closing act). Base: `integrations/app` @ `4e69f2f` (the G2×A8 merge). Worktree branch
`mirror/ad`. Directive (human, standing): **clean refactor — no dead code left behind, no
migration/compatibility with pre-mirror versions, no legacy paths kept as "insurance" once their
replacement is proven.**

## STATUS — Phase 1b COMPLETE + every deletion it unlocks LANDED; Phase 1a not started

Executed this resume (each sub-step RED→GREEN, full GUI+TUI+tests build, ctest zero failures,
format/tidy, committed; trajectory 147 → 146, the −1 being the deleted `tst_store`):

| commit | sub-step | net LOC |
|---|---|---|
| `81df15d` | **1b.1** session-meta OUTBOX lane (SessionUpdateMeta durable, op_id §10.3, §6.5 rejection relay GUI+TUI, §6.6 provenance landing incl. page-side `origin_ops` + event `origin_op` carriers, daemon accepted-expiry sweep) | +1,043 |
| `7f4fd8f` | **1b.2** direct SessionCreate seam (repo relay daemon / scripted host answer mock); the legacy ISessionStore DELEGATE PARAMETER IS GONE; node-owned no-verb ops aligned as no-ops in both modes; scoped refreshes ingestor-only | +18 |
| `d4a0606` | **1b.3** engine transcript-cache retirement — the sink is the SINGLE write path; mirror `w_transcript_blocks` PERSISTENCE added (write-behind upserts + rebaseline scope-clears + boot reload; schema v12→v13) so the mirror path carries the durability the cache leg provided; ring-fenced tests re-expressed STRONGER (cold-reboot reload asserts) | +233 |
| `2dfd0c9` | **P2-B1** substrate UNCONDITIONAL (ratified): `DAEMON_APP_HAVE_MIRROR_SUBSTRATE` deleted everywhere; Dependencies.cmake FATALs without the immer pin; null-mirror fallbacks + ChatConversationController/Routing substrate-less forks deleted | −134 |
| `2b962a4` | **P2-B2** engine roster reads re-homed (subagentTitle/childEndReason via sink read shims); FleetUnit gains `end_reason` (map-only regen off `unit-state-finished`, drift gate green) | +46 |
| `58c5593` | **P2-B3** legacy READ machinery deleted: `CachedSessionStore`, `InMemorySessionStore`, `MockFleetSource` (bundle → `daemonnet::defaultSeedBundle()`), `MockFleetTree` (mock tree = the SAME `MirrorFleetTree`); `graph.store` + `SessionStore` context property gone; parity legacy legs → GOLDEN literals + pipeline-integrity; i18n −4 obsolete entries | −2,123 |
| `460d285` + `99cb820` | **P2-B4** legacy FEEDERS retired: SessionRepository slimmed to create/submit/detail; FleetRepository = control seam only (`controlAcked` → mirror Tree refetch); RoutingRepository = mutation seam only (`mutationApplied` → mirror pin refetch; the LEDGER-a6 in-memory cache residual DELETED); SubscriptionManager migrated arms retired; DaemonCacheStore drops daemon_sessions/daemon_fleet_units/daemon_transcript_blocks (cache schema v10→v11); connect-ready storm slimmed | −1,318 |

Branch totals vs base: **114 files, +2,349 / −4,958 (net −2,609 LOC)**; ctest 146/146 green;
i18n drift green; clang-format/clang-tidy clean on all touched TUs.

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

## Deletion census (F6 evidence — deleted vs survivors, as of `99cb820`)

DELETED (classes/files):
- `CachedSessionStore` (.h/.cpp), `InMemorySessionStore` (.h/.cpp), `MockFleetSource` (.h/.cpp),
  `MockFleetTree` (.h/.cpp), `tst_store.cpp`; `graph.store` + the `SessionStore` QML context
  property (zero QML readers, verified); the null-mirror composition fallback.
- `DAEMON_APP_HAVE_MIRROR_SUBSTRATE` (option + every gate) and the substrate-less code forks.
- SessionRepository: the whole roster read path/cache feed + `updateSessionMeta` (+
  `sessionsRefreshed`/`metaUpdateFailed`/`transportSessionsResolved` signals, page loops, L4
  delta/prune). FleetRepository: the tree feed (`refreshTree`/`cachedUnits`/`syncFleetUnits`/
  `handleTreeResponse`/`treeRefreshed`). RoutingRepository: the in-memory routes/rooms cache
  (the LEDGER-a6 residual) + `refreshChats`/`refreshRooms`/`getRoute`/`bindChat`/`unbindChat`/
  `setRoute` + their signals. SubscriptionManager: the roster debounce + roster/fleet/routing
  refetch arms + the FleetRepository/RoutingRepository members.
- DaemonCacheStore: `daemon_sessions` + `daemon_fleet_units` + `daemon_transcript_blocks` tables
  (schema v11 drop-and-rebuild) + their upsert/read/clear methods + `latestTranscriptSnippets` +
  `CachedFleetUnitRow`. The engine's legacy cache transcript dual-write leg + roster/fleet reads.
- The dual-write parity legacy legs (`tst_mirror_session_store` dual-decoder,
  `tst_mirror_transcript_parity` legacy oracle, `tst_mock_daemon_parity` legacy-baseline asserts,
  the mock scenario's derived legacy-baseline content leg); the seed bundle's dead
  tags/participants members (+ their 4 i18n entries across 12 catalogs).

SURVIVORS (with reason):
- `SessionRepository` (direct SessionCreate + operator Submit + SessionGet detail — §7 seams).
- `FleetRepository` CONTROL seam (pause/resume/scale; `controlAcked` → mirror Tree refetch).
- `RoutingRepository` MUTATION seam (IRoutingActions; `mutationApplied` → mirror pin refetch).
- `DaemonCacheStore` non-migrated domains: sync cursors (engine resync + events-since), profiles,
  fs entries, transport instances/conversations (out of program scope / Phase-1a territory).
- `CachedSessionRow` (the codec's SessionInfo DTO), `CachedTranscriptBlockRow` (the engine's
  coalescing value shape + sink input). `parity::sessionKeys`/`fleetUnitKeys`
  (mirror-vs-mirror feeder-parity extractors) + `projectTranscriptBlocks` (load-bearing).
- Mirror-internal invariants kept: journal-replay ≡ snapshot-diff, mock-vs-daemon feeder parity,
  the S1-S9 grammar now pinned as GOLDEN literals + sink→window→projection pipeline-integrity.

Straggler-sweep candidates recorded for the NEXT batch (not yet deleted; each needs a QML/UX or
facade-boundary judgment): the codec facade's now-orphaned `decodeSessionPage`/`decodeTree`/
`encodeRoutingSetRequest`/`decodeChatRoutes`/`decodeRooms`/`encodeRoutingListChatsRequest`/
`encodeTransportRoomsRequest` arms; the permanently-empty Participants section + Tags sidebar
section and `ISessionStore`'s dead `tags()`/`participants()`/`unitChildren()`/`unit()`/
`createUnit()`/`createTag()` interface members (+ SidebarModel's Unit/Tag arms) — these are
user-visible surface removals; `SessionController::appendUserText` is now presentation-local
(§8.5) and the mock live-turn transcript is editor-local by design (seeded transcripts render
from the mirror; an ad-hoc mock demo turn is not durable). The generated `TODO(mirror-map)` stub
mappers belong to un-landed verticals (1a), not dead code.

## Phase-1a boundary (NOT started — the next resume's work)

Unchanged from the map below: adapter-catalog/instances + PersonEndpoint fetch/deliver arms,
room-lifecycle verbs per §7, integrations tree + TUI channels hub onto mirror projections in both
modes (scenario seed extended), then delete `MockTransportRegistry`/`MockPresenceService`/
`MockPersonsService`/`MockContactsService` and the remaining Phase-2 items they gate
(`TransportRepository`/`PersonsRepository`/`ContactsRepository` read paths where the tree/hub
was their last consumer, the `daemon_conversations`/`daemon_transport_instances` cache domains
if the mirror takes them, and the SubscriptionManager transports/contacts/persons arms).
