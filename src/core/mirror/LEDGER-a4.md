# A4 — ingestor: TDD ledger

Package A4 of the Mirror Architecture program (spec 09 §5, §4.6, §13 M1; ADR-009).
Base: `integrations/app` @ `0073a17` (A1 substrate merged). Worktree branch `mirror/a4`.

Scope (owned by A4): `mirror::Ingestor` (absorbs the `SubscriptionManager` event-policy role);
the 16-arm event→action policy table as DATA with compile-time exhaustiveness; fetch jobs
(dedup by (op,scope), priority bands, max-4 in flight, PageLoop-style accumulation); cursor +
epoch + node-rev lifecycle; reconnect BOTH modes (degraded staleness scan replacing the 19-call
storm; full mode seamed for BR); ResyncNeeded scoping; visibility-declaration staleness; the
ConvHistory cursor fix (rung-0 value); chat windows v1 + window_meta; the write-behind consumer
(one transaction: rows + journal + watermark + cursor); dual-write parity asserts; mapper bodies
for the entities A4's arms touch (Conversation, ChatMessage, Contact, Person).

## Architecture — the ingestor is decoupled from the wire

The ingestor core (`da_mirror_ingestor`) depends only on Qt Core + the mirror substrate +
generated entities. It consumes a decoupled `mirror::NodeEvent` and drives two seams:

- `mirror::FetchExecutor` — issues a `FetchJob` (op + scope + correlation + priority); the
  ingestor is later fed the decoded entities via `deliverFetch*`. Tests use a recording mock;
  the daemon bridge wraps `NodeApiClient` + the vendored codec + the mappers.
- The event input `Ingestor::ingest(const NodeEvent&)`. The daemon bridge translates the
  codec's `DecodedNodeEvent` → `mirror::NodeEvent` (a pure, tested function).

This keeps the policy table + scheduler + reconnect + windows fully unit-testable with synthetic
E4 streams (spec §12) and no live transport, and keeps the wire codec confined to the bridge
(07§10 "codec boundary" preserved).

## QCoro decision (ADR-009 conditional) — NOT adopted for A4

Rationale: the fetch scheduler is a bounded (max-4) band-ordered FIFO whose only async edge is
the transport (already async at the `NodeApiClient` seam); completions resume on the main loop
via a callback. Introducing `QCoro::Task<>` would add a vendored pre-1.0 dependency and an
unproven WASM `co_await` path (spec §11 hard gate #2 has zero upstream wasm evidence) for no
structural benefit at this layer. ADR-009 explicitly permits "plain-Qt state machines … if
cleaner given WASM constraints — decide and justify." The `FetchExecutor` seam is shaped so a
future `QCoro`-backed executor is a drop-in (issue → resume-with-result), should profiling ever
justify it.

## TDD protocol

Ledger → RED (failing tests committed, QSKIP-guarded where a suite must stay green-but-skipping)
→ GREEN → refactor + gate. Tests are never weakened. Committed in coherent RED/GREEN slices per
sub-area (policy table, scheduler, ingestor dispatch+windows+cursor, reconnect+resync+visibility,
write-behind, parity) rather than one giant pair, so the ledger stays honest.

## Invariants under test (spec refs)

- §5.1 single writer: the ingestor is the sole `mirror::Writer`; every apply funnels one
  stamp/commit path; a debug fingerprint marks the writing component.
- §5.2 policy table: all 16 `NodeEvent` arms declared as DATA (action interim/full, coalesce
  lane + debounce, skip-gates); exhaustive over the arm enum at COMPILE time (a `switch` with no
  `default` under `-Werror=switch`) + a runtime test asserting a row for every arm.
- §5.3 diff-before-write: refetch applies diff against the table; unchanged keys stamp nothing
  (already enforced by `mirror::Batch`; the ingestor relies on it and asserts no-op suppression).
- §5.4 fetch jobs: dedup one in-flight per (op,scope) coalesce key; a re-request in flight is
  absorbed; a newer reason re-queues once; priority bands visible>prefetch>reconcile; max 4
  concurrent, band-ordered FIFO; PageLoop-style accumulation (replace-vs-additive).
- §5.5 cursor+epoch: per-collection node-rev + fetched_at + state; feed cursor + epoch; two
  revision spaces (node vs mirror) meet only here.
- §5.6 reconnect: DEGRADED = resume EventsSince(cursor) + staleness scan of stale collections
  (the 19-call storm replaced); FULL = Bootstrap probe → per-collection rev compare → since_rev
  delta reads (seamed; gated on api≥39 + rev-field presence for BR).
- §5.7 ResyncNeeded: scope → collection set → mark stale → scan (never a blind storm).
- §5.8 visibility staleness: lenses declare beginObserving/endObserving; the ingestor's
  per-collection max-age-while-observed policy decides whether observation triggers a fetch —
  freshness is store-metadata-driven, never a lens callsite.
- §4.6 chat windows v1: W-kind ChatMessage ingestion via the store window + window_meta.
- §13 M1 ConvHistory cursor fix: MessagesChanged issues ConvHistory(after_cursor = stored
  per-conv cursor), appending only the new tail — never a full-transcript refetch from 0.
- write-behind (B7): drain the store commit stream → one sqlite transaction per batch carrying
  entity rows + journal rows + watermark + cursor together.
- dual-write parity: at quiesce, mirror table row-set ≡ legacy repository row-set; log-on-
  mismatch in dev (§13 M1 exit criterion).

## Ported-test provenance (license-disciplined re-expression)

- Akonadi (LGPL — behaviors re-expressed, NEVER text): bounded-pipeline / max-in-flight cap and
  burst compression `references/architecture/akonadi/src/core/monitor_p.cpp:959-971,648-751,
  856-936` → the fetch scheduler's max-4 band-ordered FIFO + coalesce-in-flight tests.
- Sink (LGPL — behaviors only): `modifyIfChanged` diff-before-write suppression
  `common/synchronizer.cpp:184-202` → the ingestor no-op-suppression test (rides the substrate's
  value-equality gate).
- E4 (spec §12, ours): synthetic decoded NodeEvent streams + fetch responses replayed
  deterministically to a canonical store dump.

## Integration points left for siblings / BR

- BR (`wire_delta` activation): §5.5/§5.6 full-mode seam — `SyncState::selectMode(apiVersion,
  hasRevFields)` + the `since_rev`/Bootstrap fetch ops are declared; the bridge activates them
  when the regenerated api/39 codec lands. Degraded mode is the shipped default against v38.
- A5 (consumer migration): the live `app_service_graph` still runs `SubscriptionManager` + the
  connect-ready storm (dual-write discipline, rule zero). A4 provides the ingestor + bridge that
  replace them on the mirror path; flipping the live storm OFF and repointing readers is A5's
  vertical-port step. The `SubscriptionManager` external surface remains as a facade meanwhile.
- A3 (TableModel): reads journal deltas only; A4 creates no table models.
