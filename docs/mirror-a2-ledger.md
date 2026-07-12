# A2 — durable outbox + `local-<id>.db` module — TDD ledger

Package A2 of the Mirror Architecture program (spec `09-specification.md` §6, §3.4; ADR-005,
ADR-006). Worktree `mirror/a2` off `integrations/app`. This ledger is the pre-code contract; the
final report (chat) carries the evidence.

## Scope (spec anchors)

1. **`local-<id>.db` module** — per-identity, versioned + **migrated, NEVER dropped** (unlike the
   disposable `mirror-<id>.db`/`daemon_cache.db`); owns the outbox schema (§6.1) + the sidecar
   tables (§3.4). Schema-version scaffold + a real migration path (v1→v2) with a migration test.
2. **Outbox core** (§6.1–§6.5): pending-op rows keyed by UUIDv7 op-ids (§6.2), lanes with
   FIFO-per-lane, retry with exponential backoff, per-lane drain gates (§6.3), the per-verb-class
   table (§6.4), and the rejection contract — pause-on-rejection + retry/edit/discard +
   send-remaining-anyway (§6.5).
3. **Crash-safety** (§6.6): durable enqueue before send; boot inflight→pending; two-phase landing
   + boot reconciliation as clearly-marked seams (full reconciliation needs the mirror journal +
   api/39).
4. **No auto-replay** (§6.8): replay is manual-drain only here. Auto-replay is an explicit,
   hard-disabled gate; a test asserts it stays off against api/38 AND >= api/39 (the BR bridge
   flips it later), while manual drain always works.
5. **PendingOpsModel** — a `QAbstractListModel` lens over the outbox with per-lane / per-scope
   filtering hooks for A5's pending strip. Exact row ops, no resets outside initial population
   (guardrail §14.8).
6. **Composer turn-prompt lane swap** (§6.7 step 1): swap `ComposerQueueModel`'s STORAGE onto the
   outbox turn-prompt lane, roles/signals UNCHANGED. Existing composer tests stay green unmodified
   (they are the regression harness).
7. **origin_op / null handling** (ADR-006, §6.6): against api/38 there is no op-id echo; the state
   machine supports confirm-by-op-id AND expiry/orphan handling; null origin = "unattributed".

## Division with A1 (concurrent)

A1 owns `mirror.db` + the immer store + journal under `src/core/mirror/`. A2 owns **only** the
`local-<id>.db` module. New code lives in a dedicated module `src/core/local/` (namespace
`mirror::`, matching the spec's data-layer namespace) so there are no file-level collisions with
A1/G1's `src/core/mirror/`. A2 does not touch `flake.nix` (A1 vendors libs there) and creates no
store/journal code.

## Module layout (`src/core/local/`, namespace `mirror`)

| File | Contents |
|---|---|
| `uuidv7.{h,cpp}` | RFC 9562 UUIDv7 minting (`generateUuidV7()`, injectable ms overload) |
| `local_database.{h,cpp}` | `local-<id>.db`: QSQLITE conn mgmt, per-identity namespacing, versioned schema + forward migrations (never drop), outbox row CRUD, sidecar accessors |
| `outbox_types.h` | `PendingOp`, `OpState`, `VerbClass`, `LaneKind`, `ErrorClass` |
| `verb_class.{h,cpp}` | §6.4 per-verb-class table: verb→class→lane→gate/lane-kind |
| `outbox.{h,cpp}` | `mirror::Outbox` state machine: enqueue/drain/gates/backoff/rejection/confirm/boot |
| `pending_ops_model.{h,cpp}` | `mirror::PendingOpsModel` QAbstractListModel lens + filters |
| `outbox_composer_queue_storage.{h,cpp}` | adapter binding `ComposerQueueModel` storage to the turn-prompt lane |

Composer module change (behavior-preserving): `ComposerQueueModel` gains an abstract
`ComposerQueueStorage*` hook (default null = today's in-memory `QList<Entry>` — existing tests
unchanged). When a storage is attached the same signals/roles are emitted while entries persist.

## State machine (§6.3/§6.5/§6.6)

`OpState`: `Pending(0) | Inflight(1) | Rejected(2) | Accepted(3)`.

- enqueue → durable INSERT `Pending` (commits before any send: the durability point).
- drain step (manual): gate passes → `Pending`→`Inflight` (one in-flight per lane, global cap 4,
  round-robin) → `sendRequested(op)` (the caller owns the wire).
- ack → `Accepted` (awaiting provenance). transient failure → `Pending`, attempts+1, backoff
  `min(1s·2^n, 60s)`. rejection → `Rejected`, **lane pauses**.
- provenance-stamped delta (`origin_op == op_id`) → landed → row deleted (two-phase; boot
  reconciliation deletes ops already in persisted provenance — mirror-journal scan is a seam).
- rejection affordances: `retry` (same op_id, unpause), `edit` (`takeForEdit` returns payload +
  deletes; caller enqueues a NEW op_id), `discard`, `sendRemainingAnyway(lane)`.
- accepted disposition: api<39 → terminal success (delete on ack, v38 behavior); api>=39 → grace /
  covered-by-delta (seam; A2 keeps the pre-provenance behavior since auto-replay is disabled).

Lane kinds: all §6.4 wire-verb lanes are **mutation lanes**; turn-prompt is a **dispatch lane**
(entries consumed at drain into Submit; not awaiting read-path visibility).

## Error classes (§6.5)

`Unauthenticated` → pause all wire lanes (not a rejection); connection loss/timeout → transient
(backoff, order kept); `Unsupported`/`Conflict`/`Forbidden`/`UnknownSession`/`Other` → rejection.
Anything unknown defaults to rejection (fail visible).

## TDD steps

Ledger → RED (failing tests committed) → GREEN → refactor + gate. Never weaken a test.

RED test inventory (one executable `tst_outbox`, plus a composer durability case + the existing
composer suites unchanged):

- `uuidv7`: version nibble == 7, variant == 0b10, monotonic-by-time sortability, uniqueness.
- `local_db_migration`: a hand-built v1 DB migrates to current on open — outbox rows survive, new
  sidecar column exists, `schema_version` bumped, DB file never dropped.
- `local_db_sidecar`: sidecar upsert/read for conversation + session keyed by canonical key.
- `outbox_fifo`: strict FIFO within a lane; independent lanes; at most one in-flight per lane;
  global in-flight cap 4 (ported: sink messagequeuetest ordering/concurrency).
- `outbox_backoff`: `backoffMs` doubles capped at 60s; transient failure re-queues keeping order
  (ported: sink synchronizertest transient path).
- `outbox_rejection`: rejection pauses the lane; retry/edit/discard/send-remaining
  (ported: sink synchronizertest permanent path).
- `outbox_confirm`: ack→accepted; provenance→deleted; null origin unattributed; confirm-by-op-id;
  provenance-keyed uniformly across a mutation + a dispatch lane (one contract, no per-verb).
- `outbox_bounds`: 64/lane + 1024 global enqueue past bound fails visibly (never silent drop).
- `outbox_crash_injection`: durability across enqueue|send|confirm phases; boot inflight→pending;
  replay-after-restart FIFO (ported: akonadi changerecordertest restart-replay contract).
- `outbox_no_auto_replay`: gate stays disabled vs api/38 AND api/39; manual drain still drains.
- `pending_ops_model`: exact insert/remove/dataChanged (QAbstractItemModelTester), lane/scope
  filters, no reset outside init.
- `composer_durable_queue`: attaching the outbox storage persists queued prompts across a
  simulated restart while roles/signals/count are byte-identical to the in-memory model.

Ported-test license discipline: Sink/Akonadi are (L)GPL — behaviors are re-expressed in our own
words and code; NO source text copied.

## Seams left (documented in code + report)

- BR auto-replay gate (§6.8) — enabled per connection at api>=39 by the BR bridge; A2 hard-off.
- A4 confirm-matching heuristics for null `origin_op` — A2 only carries the state (confirm-by-op-id
  + expiry/orphan); heuristic attribution is A4/A5.
- A5 pending strip — consumes `PendingOpsModel` (filters ready).
- Two-phase boot reconciliation vs the mirror journal retention tail (§6.6) — A2 does the local
  half (delete ops whose op_id is in a supplied persisted-provenance set); the journal scan is A1's
  surface.

## Open questions

- Emitter-generated sidecar DDL: the entity map declares 6 `sidecar.*` fields but
  `mirror_schema_gen.sql` intentionally omits them (sidecar lives in `local-<id>.db`). A2
  hand-writes the sidecar DDL per the brief. Whether a future emitter should also emit sidecar DDL
  (keeping the map the single source) is noted for G-series.
- `draft_refs` / `updated_ms` sidecar columns (§3.4 DDL sketch) are storage bookkeeping not present
  as `sidecar.*` entity-map fields; A2 includes them as columns. Flagged in case the map should
  own them.
