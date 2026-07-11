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
