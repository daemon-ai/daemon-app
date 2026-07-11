# A8 — M5 mock reform (`mirror::Seeder`, scripted outcomes, api/<N> Hello, provenance landing): TDD ledger

Package A8 of the Mirror Architecture program (spec 09 §13 wave M5; §9, §6.6, §6.8, §5.6, §5.1,
§12; 07§9.5/§9.10). Base: `integrations/app` @ `52ebae9` (the A7 merge). Worktree branch
`mirror/a8`. Sibling A7T works in parallel on the transcript/turn-engine re-homing seam
(`daemon_turn_engine.cpp`, `cached_session_store` content projection, `mirror_session_store`
content delegation, ingestor transcript-deliver methods) — A8 keeps its ingestor edits ADDITIVE
and store/outbox-side so the two merge cleanly.

## Scope (owned by A8)

1. **Provenance-landing seam** (deferred to A8 by BR and A7 — the first landed slice): report every
   provenance-stamped apply `(origin_op)` from the single-writer commit path to the outbox
   (§5.1 read-side coupling) and flip `Outbox::setProvenanceCapable(true)` for api/39 IN THE SAME
   CHANGE, so `accepted` ops await the §6.6 provenance-keyed landing instead of lingering.
2. **`mirror::Seeder`** (§9): a mode-agnostic feeder that writes canonical entities + windowed
   items through the SAME `mirror::Store` apply/commit pipeline the ingestor uses, journal
   `origin = seeder`, so mock/daemon parity holds by construction. Scenario = declarative seed set
   + scripted timeline (events over time). Deterministic (seeded RNG if any).
3. **Scripted verb outcomes** (§9, MANDATORY rejection scripts): a scenario declares per-verb
   outcomes (`ok` / `reject(kind,message)` / `timeout(ms)` / `delay(ms)` / provenance-echo delay) so
   the outbox rejection path (§6.5) is exercised in mock and in tests — the path Sink shipped
   unfinished (03§3.3). Scripted outcomes live ONLY at the verb/wire boundary; nothing mock-only
   exists below the seam (§14 rule 5).
4. **Configurable `api/<N>` mock Hello** (§9 last bullet): the mock connection advertises a
   scenario-chosen N (38 or 39) so BOTH ingestor modes (§5.6 degraded refetch_diff vs full
   wire_delta w/ Bootstrap + since_rev) and the auto-replay gate (§6.8 off@38 / on@39) run in mock.
5. **Delete the ~10 per-seam data mocks** (07§9.10) as the seeder serves their surfaces in GUI+TUI;
   `DAEMON_APP_MOCK_INTEGRATIONS` collapses into seed scenarios (compat alias only if consumed).
6. **Mock e2e parity** (§13 M5 gate): the same scenarios render equivalently in mock and daemon.

## Mock census (07§9.10 — what dies, what survives, and why)

Verified by `rg -l 'class .*Mock' src/core src/DaemonApp`. Category by whether the seeder can serve
the surface through the mirror apply pipeline (mock mirror live) vs. seams with no mirror read path
yet (A7's documented degradations / node-blocked / no wire op).

DATA MOCKS the seeder REPLACES (mirror-served in both modes — the 07§9.10 census, deleted as each
surface's mirror read path is seeder-fed and parity-green):

| Mock | Surface | Mirror table the seeder feeds |
|---|---|---|
| `MockChatService` | chat timeline | `chat` window (W) + `conversations` |
| `MockPersonsService` | persons tree | `persons` |
| `MockContactsService` | contacts | `contacts` |
| `MockTransportRegistry` / `MockPresenceService` | channels tree, presence | `transport_accounts` + `rooms` |
| `MockFleetSource` (+ `InMemorySessionStore`, `MockSessionRoster`, `MockFleetTree` seed) | roster/sidebar/fleet | `sessions` / `fleet_units` (A7 mechanism) |

SURVIVORS (retained, each with a stated reason — NOT part of the M5 data-mock census, or blocked):
- `MockConnectionService` — the transport stand-in, NOT a data mock; A8 EXTENDS it with the
  configurable api/<N> Hello. Survives (there is no real node in mock mode).
- `MockAuthFlowService` — interactive-auth state-machine stand-in (a T-class flow, no mirror table);
  survives.
- `MockCronStore` — CronJob has NO v38 wire op (07§2 #17; rendered EMPTY in daemon); its demo seed
  is a mock-only illustration with no mirror read path. Survives until the node grows the op.
- `MockModelCatalog`/`MockProviderCatalog`/`MockAccountsService`/`MockProfileStore`/
  `MockSessionSettings`/`MockCheckpointTimeline`/`MockToolInventory`/`MockDaemonConfig`/
  `MockMemoryService`/`MockDashboard`/`MockApprovalsInbox`/`MockFeedback` — these back domains
  whose mirror verticals are NOT in the M5 charter (models/providers/profiles/accounts/checkpoints/
  tools/memory/dashboard/approvals are post-M5 or L-class-freshness-critical). They stay on the
  legacy mock seam; the seeder feeds the mirror for the M2–M4 verticals (chat/persons/contacts/
  routing/sessions/fleet) that the mirror already owns. Deleting the rest is out of A8's charter
  (their consumers do not read the mirror yet) — flagged for the wave that ports them.
- A7's shape gaps kept as documented degradations (NOT fixed here): mirror Session has no
  content/tagIds/unitId; the fleet TREE stays legacy (`daemon_fleet_tree`; FleetUnit lacks
  parent/children edges — a G-series codegen change).

## Provenance-landing state machine (§6.6, landed FIRST)

The single writer stamps journal records with `origin_op` on the read path (BR landed chat
`origin_op`; conversations/sessions delta reads carry it). A8 wires the §5.1 read-side coupling:

```
enqueue ──▶ pending ──drain(gate ok)──▶ inflight ──onAck──▶ [provenanceCapable?]
                                             │                     │ yes            │ no
                                             │                     ▼                ▼
                                       onRejection            accepted          (deleted: v38 terminal)
                                             │                     │
                                             ▼           provenanceStamped(origin_op==op_id)
                                       lane PAUSED                 │
                                     (retry/edit/discard,          ▼
                                      §6.5)                     landed (row deleted)
                                                                   ▲
                                        expireAcceptedOps(grace) ──┘  (ack-without-echo: superseded
                                                                       or provenance-less path, §6.6)
```

- `MirrorService::provenanceStamped(QString origin_op)` — emitted once per non-empty-`origin_op`
  journal record on each commit (scans `journal().since(revFrom)` in the `Store::committed` slot).
  The substrate stays free of any outbox dependency; the graph connects the signal to
  `Outbox::onProvenanceStamped`.
- Graph: `setProvenanceCapable(api >= 39)` on AuthOk (same predicate as `autoReplayEnabled`), and
  boot reconciliation over the persisted journal tail (`Persistence` origin_ops) →
  `Outbox::reconcileLandedOps` (idempotent two-phase cleanup, §6.6).
- Tests (`tst_provenance_landing`, links mirror_ingestor + local): ack→echo→landed;
  rejection→paused lane; ack-without-echo→`expireAcceptedOps` disposal; api/38 ack is terminal.

## Seeder / scenario design (§9)

- `mirror::Seeder` holds a `Store::Writer` (via `mirror::Store&`), builds canonical entities, and
  applies them in ONE batch per seed step with `JournalOrigin::Seeder` — the SAME `Batch`
  upsert/appendWindow path the ingestor uses (§4.2 one-commit-per-batch preserved). No bespoke mock
  apply; parity by construction (§9 "same store, same journal, same lenses, different feeder").
- `mirror::Scenario`: a declarative value type — `seed` (initial entity/window sets) + `timeline`
  (a list of `{ at_ms, step }` events: presence flips, message arrivals, roster changes) +
  `apiVersion` (38/39) + `verbScript` (per-verb outcomes). Deterministic: a seeded `std::mt19937`
  for any jitter; the timeline is played on an injectable clock/QTimer so tests advance it.
- The seeder is the mock `mirror::Writer` (§5.1: Ingestor xor Seeder). A debug fingerprint marks
  the writing component; the mock composition installs the Seeder as the writer, the ingestor's
  fetch executor becomes a scripted pager for demand-paging (§9 "history deeper than the window").

## Script grammar (§9 scripted verb outcomes)

A `VerbScript` maps `(verb, matcher)` → `Outcome`:
- `ok` — the verb acks `Ok`; a `MutationEffect` may schedule a provenance-stamped seeder apply
  after `echoDelayMs` (the node's read-path echo → §6.6 landing).
- `reject(kind, message)` — a typed `api-error` (closed v38 set); drives the outbox lane pause.
- `timeout(ms)` — no reply within the drain timeout (transient; backoff).
- `delay(ms)` — ack after a delay (latency simulation).
Matcher = verb name + optional arg predicate (e.g. `ConvSend` whose message == "boom" rejects).
Rejection scripts are MANDATORY fixtures per §9.

## TDD protocol

Ledger → RED (QSKIP-guarded where a suite must stay green-but-skipping) → GREEN → refactor + gate.
Tests never weakened; hooks pass per commit. Landed in coherent slices: (1) provenance landing,
(2) seeder core, (3) verb script, (4) api/<N> Hello + both-modes tests, (5) mock composition flip +
data-mock deletion, (6) mock e2e parity.

## Coordination notes (A7T / parent)

- A8 ingestor edits are ADDITIVE (new methods, no edits to A7T's transcript-deliver seam);
  provenance landing is store/outbox-side. Expect the parent to resolve CMakeLists adjacency.
- Do not touch `daemon_turn_engine.cpp`, `cached_session_store` content projection, or
  `mirror_session_store` content delegation (A7T owns content re-homing). Mock transcript content
  keeps flowing through the legacy/mock content source regardless of the storeMirror flip.
- Do not rename `mirror::` (ADR-011 is A9's).

## Status

- Ledger — this file.
- **Provenance-landing seam — DONE.** `MirrorService::provenanceStamped` re-emits each commit's
  journal `origin_op`; graph wires it to `Outbox::onProvenanceStamped`, flips
  `setProvenanceCapable(api>=39)` in the SAME change, and runs boot reconciliation over
  `Persistence::persistedOriginOps()` (§6.6 two-phase local cleanup). `tst_provenance_landing`
  (8 cases) + `tst_mirror_service::provenanceStampedRelaysOriginOp`. ctest 141→142.
- **Seeder core + scenario/script types — DONE (additive; composition flip pending).**
  `mirror::Seeder` writes every kind through the store apply/commit pipeline (journal
  origin = Seeder, one batch), `Scenario`/`SeedSet`/`ScenarioPlayer` play a declarative seed +
  deterministic scripted timeline, and `VerbScript` resolves ok/reject/timeout with an arg matcher
  (the MANDATORY rejection fixtures). `tst_mirror_seeder`: seed+timeline stamp Seeder only, a lens
  renders seeded rows (parity by construction), verb-script outcomes. ctest 142→143. NOT yet wired
  into the mock composition (the flip + data-mock deletion are the remaining M5 slices).
- **Both-ingestor-modes + auto-replay gate in mock — DONE.** `tst_mock_both_modes` drives a
  Seeder-fed stack (Store+Seeder+Ingestor+Outbox) from `Scenario::apiVersion` (the configurable
  api/<N> Hello): full(39) → Bootstrap probe + auto-replay drain fires; degraded(38) → staleness
  scan + no Bootstrap + lanes hold (manual tap only). Proves both §5.6 modes and the §6.8 gate run
  in mock. ctest 143→144.

## The mock-composition cutover (second checkpoint — design + verified census)

**Program directive applied**: clean refactor — no dead code, no migration/compat with pre-mirror
versions, scenarios are the only seed source, no aliases.

### `DAEMON_APP_MOCK_INTEGRATIONS` — already collapsed on this branch (verified)

`rg DAEMON_APP_MOCK_INTEGRATIONS` over this repo returns ONLY ledger prose — the env var, its
demo-tree opt-in, and its `tst_app_service_graph` cases were deleted in the earlier integration
waves (the M3 `IDaemonNet` deletion took the demo tree with it). The references that remain live
in the SUPERPROJECT's pinned (older) `daemon-app` submodule copy and its architecture docs — not
in this repo, and the superproject is out of A8's write scope. Nothing to alias; nothing to keep.
The scenario selector (`DAEMON_APP_MOCK_SCENARIO`, below) is the replacement mechanism.

### Composition design (slices 1–3)

- `daemonnet::defaultSeedBundle()` — the canned fleet/session/tags/participants demo content
  MOVES out of `MockFleetSource::buildSeed()` into a free function; `MockFleetSource` keeps its
  default ctor (delegating) so its surviving consumers and tests are untouched. The bundle is the
  single source for the session/fleet DOMAIN data; the default scenario derives its mirror rows
  from it (one declaration → two projections; ids provably agree, which the delegated
  `content()` join requires).
- `daemonapp::daemon::MockScenario` (`mock_scenario.{h,cpp}`, substrate-gated): `{name, bundle,
  mirror::Scenario}`. Built-ins: `default` (api/39, full seed, scripted timeline + verb script
  incl. the MANDATORY rejection rule), `api38` (same data, api/38 — degraded mode + auto-replay
  hold at app level), `empty` (nothing seeded — onboarding/empty-state demos).
  `DAEMON_APP_MOCK_SCENARIO` selects; unknown names warn + fall back to `default`.
- `daemonapp::daemon::MockScenarioHost` (`mock_scenario_host.{h,cpp}`): the mock-mode driver.
  Owns `mirror::Seeder` + `mirror::ScenarioPlayer`; plays the seed at construction; a QTimer +
  QElapsedTimer advance the timeline deterministically. Answers `Outbox::sendRequested` from the
  scenario's VerbScript (ok → delayed ack + provenance ECHO through the seeder with
  `origin_op = op_id`; reject → `onRejection` → lane pause §6.5; timeout → delayed
  `onTransientFailure` → backoff §6.3). Installs a scripted-pager `FetchExecutor` that records +
  async-completes every job (the mirror is seeder-authoritative; the recording is the app-level
  §5.6 mode evidence). `onConnectionReady()` = the mock Hello: `ingestor().onConnected(api,
  true)` + `setProvenanceCapable(autoReplayEnabled(api))` + auto-replay drain at 39 — the same
  wiring the daemon branch does from AuthOk.
- Graph mock branch (`app_service_graph.cpp`): `MirrorService::openInMemory()` (mock state is
  throwaway — nothing persisted, no migration surface, per the directive); `LocalDatabase
  (":memory:")` + `Outbox` (gate: mutation lanes require `connection->ready()`); the
  provenance-landing wiring (same as daemon); `storeMirror = MirrorSessionStore(mirror store,
  ingestor, legacy InMemorySessionStore)` — the A7 aliasing fallback DELETED. content()/mutations
  keep delegating to the legacy store (A7T's seam, untouched). Mock-ready/lost transitions drive
  the host.

### Mock census — the verified truth (per-consumer grep, this branch)

DELETED this checkpoint (last consumer flips with the deletion, same commit):
- `MockChatService` — read path: the chat surfaces bind `Mirror` (non-null in mock now) → the
  window lens; send path: `ConvSendController.laneActive` (outbox non-null in mock now) → the
  chat-send lane + scripted outcomes. The graph was its last app consumer (it was never even
  seeded there — mock chat rendered EMPTY pre-cutover; the scenario seeds real timelines).
  Its unit-test uses move to a test-local `IChatService` fixture (the controller's legacy path
  stays COMPILED for substrate-less stacks + the daemon dual-write window; A9 deletes it with
  the legacy seams).
- `MockSessionRoster` — the ops-hub roster in mock becomes `DaemonSessionRoster` over the seeded
  `storeMirror` (the same projection daemon mode uses — 6→1 in BOTH modes). MockDashboard keeps
  observing the interfaces (rebuild order honors the dashboard-observes-deleted-roster SIGSEGV
  precedent).

SURVIVORS (each with its blocking dependency, re-verified by consumer grep):
- `MockTransportRegistry`/`MockPresenceService`/`MockPersonsService`/`MockContactsService` — the
  integrations tree (`IntegrationsTreeModel` composes DIRECTLY from ITransportRegistry +
  IPersonsService — its header says so), the TUI channels hub, and the TUI contact/room flows
  read the legacy seams in BOTH modes (daemon serves them via the Daemon* adapters over the
  repositories). The mirror cannot serve those surfaces yet: `Adapter`/instances have NO fetch
  arm (TransportChanged patches only), person ENDPOINTS (the tree's per-transport person
  sections) have no deliver path, and the registry mocks carry the room-lifecycle/member VERB
  flows (join/create/kick/ban) the mirror write path doesn't own. The enabling work is a
  consumer port + ingestor fetch arms — a post-M5 vertical (it was never in the M2–M4 waves).
  The 07§9.10 census assumed these would be mirror-served by M5; the executed waves left the
  tree/channels on the seams. NOT deletable without breaking both modes' rendering.
- `MockFleetSource` + `InMemorySessionStore` — the delegated transcript-content + mutation source
  behind `MirrorSessionStore` (A7T is re-homing content in parallel; forbidden seam) and the
  fleet TREE's mock seed. They die with A7T + the mutation outbox lane (A9's deletion order).
- `MockFleetTree` — the fleet TREE cannot be mirror-served: `FleetUnit` carries no
  parent/children edges (G-series codegen change; A7's documented degradation, not mine to fix).
- `MockConnectionService` (the transport stand-in — A8 drives the ingestor's mode select from
  the scenario at its ready transition), `MockAuthFlowService`, `MockCronStore` (no wire op),
  `MockDashboard` (derives counters from interfaces), and the post-M5 domain mocks
  (models/providers/accounts/profiles/session-settings/checkpoints/tools/config/memory/
  approvals/feedback) whose consumers don't read the mirror yet.

### Aligned degradations (mock now matches daemon — the fork IS the deletion)

- Mock participants/tags panes render like daemon mode renders them (MirrorSessionStore returns
  empty tags()/participants() — the legacy mock-only seeded rows were a mock/daemon shape fork).
- Mock transcript content still flows through the legacy delegate (A7T's seam) — unchanged.
