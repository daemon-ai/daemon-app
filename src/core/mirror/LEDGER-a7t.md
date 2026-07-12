# A7T — M4 sub-step 6: transcript blocks re-homed onto the mirror (own gate): TDD ledger

Package A7T of the Mirror Architecture program (spec 09 §13 wave M4, the wave's FINAL,
fidelity-critical sub-step; §4 store/journal/windows §4.6, §5.1 single-writer, §8 projections,
§14 guardrails). Base: `integrations/app` @ `52ebae9` (the A7 merge). Worktree branch `mirror/a7t`.

A7 landed the 6→1 session-consumer unification and deliberately left sub-step 6 behind its own
gate (LEDGER-a7 "Sub-step 6 — NOT attempted; the precise seam"). This package executes that seam:
re-home the turn engine's app-local transcript stream onto the mirror `w_transcript_blocks`
window, behind a separately-gated read path, so `MirrorSessionStore::content()` can eventually
serve the msg-fence projection FROM the mirror with zero consumer churn (every surface already
reads through the one `MirrorSessionStore` facade).

## The single writer being re-homed — engine callsite census (verified by read)

`src/DaemonApp/Turn/daemon_turn_engine.cpp` is the transcript single writer. It coalesces the live
`AgentEvent` stream (Subscribe) and the durable journal (SessionHistory rebaseline) into
`CachedTranscriptBlockRow`s keyed by `(sessionId, seq)`, written through
`DaemonCacheStore::upsertTranscriptBlock` / `clearTranscript`. The write callsites:

1. **Inbound user / delegation-notice Message** (`applyLogPage`, Command/Inbound arm): a
   node-echoed `StartTurn`/`Steer` persists a `Message` block, `role = User` (ordinary) or
   `System` (a `user_msg.notice` delegation completion). seq = the log entry seq.
2. **ToolCall** (`applyLogPage`, Event/ToolStarted arm): a `ToolCall` block carrying
   `callId`, `toolName`, `argsSummary`. The `todo` tool is SUPPRESSED (its call id is tracked so
   the matching result is dropped too — it is the composer status-stack feed, not transcript).
3. **ToolResult** (`applyLogPage`, Event/ToolFinished arm): a `ToolResult` block carrying
   `callId`, `ok`, `summary` (capped at 1 MiChar), `detailKind`, `detailBody`.
4. **Final assistant Message** (`applyLogPage`, Event/TurnFinished arm, non-empty finalText): a
   `Message` block, `role = Assistant`.
5. **Reasoning** (`checkpointReasoningBlock` / `settleReasoningBlock`): reasoning deltas accumulate
   into ONE `Reasoning` block per run, keyed by the FIRST delta's seq. Re-checkpointed (upsert with
   growing text) at each page boundary; settled (closed) by the next content event.
6. **Journal rebaseline** (`onResponse`, journalCorrelation arm): `clearTranscript(session)` then
   re-`upsertTranscriptBlock` every durable block from the paged SessionHistory replay (Message /
   ToolCall / ToolResult / Request / Content / Other), in seq order.

Watermark/cursor writes (`setCursor`) are session-resync bookkeeping, NOT transcript content — out
of scope for the mirror window.

### Coalescing rules as observed (must be preserved bit-for-bit)

- **Reasoning coalescing**: one block per run, keyed by the run's first seq; status-only events
  (Usage/Context) do NOT split it; any TextDelta/ToolStarted/ToolFinished/TurnFinished/Error
  settles it. Re-checkpoint = in-place update of the SAME seq with the fuller text.
- **`todo`-tool suppression**: `ToolCall{name=="todo"}` and its matching `ToolResult` never enter
  the transcript (they drive the TodoListModel via `todoUpdate` instead).
- **ToolResult folding**: a `ToolResult` is never rendered standalone; it is folded into its
  `ToolCall`'s fence (status ok/error + summary/detail), matching the live single patched card.
- **Upsert-by-seq idempotency**: `(session, seq)` is the block identity; re-writing a seq replaces
  it. seqs are monotonic within a session's applied stream (reasoning's first-seq is settled before
  any later block is persisted), so the window is append-with-occasional-same-seq-update.

## Read projection grammar — `CachedSessionStore::content()` (the byte-stable contract)

Blocks in seq order → msg-fence markdown that the BlockEditor re-parses into the SAME cards as the
live document. The exact grammar (byte-stable; ids are `cache-<seq>` so re-projection is stable):

- **Message marker** opening a bubble: `` ```msg\n{"id":"cache-<seq>","role":"<role>"}\n```\n\n ``
  (compact JSON, keys as emitted; role lowercased). A fresh Message row opens a new bubble; the
  turn's final Assistant Message CONTINUES an assistant group opened by a preceding agent block.
- **Message body**: the raw text followed by `\n\n`.
- **Reasoning fence**: opens/continues an assistant group, then
  `` ```reasoning\n{"body":"<text>","status":"complete"}\n```\n\n `` (compact JSON, **keys sorted**
  by QJsonObject → `body` before `status`).
- **ToolCall fence**: opens/continues an assistant group, then a `` ```tool `` fence whose compact
  JSON body is `{argsSummary,callId,name,status[,summary][,detailBody][,detailKind]}` (QJsonObject
  sorts keys alphabetically). `status` = `running` (no result) | `ok` | `error`. The folded result
  adds `summary` (when non-empty) and `detailKind`+`detailBody` (when a structured detail was
  present). `todo` calls are skipped.
- Trailing whitespace is `.trimmed()` off the whole document.

## THE ENTITY-FIELD GAP (decisive fidelity finding) — see "Flip decision" below

The generated mirror `TranscriptBlock` entity
(`generated/entities_gen.h`, source of truth = `daemon-api.cddl` + `entity-map.toml`, regenerated
ONLY via the superproject/node `just update-codec`; hand-editing forbidden) models the
transcript-block CDDL union with the discriminator + **common** fields only:

```
session, seq, kind, role, text, call_id, name, summary, ok, origin_op
```

The legacy projection ALSO consumes three fields the entity does NOT carry:

- `argsSummary` (ToolCall args) — the legacy `` ```tool `` fence ALWAYS emits an `argsSummary` key.
- `detailKind` + `detailBody` (ToolResult structured detail: diffs, listings, typed payloads) —
  folded into the fence when present.

`entity-map.toml [entity.TranscriptBlock]` maps only `transcript-block-tool-call.{call_id,name}`
and `transcript-block-tool-result.{summary,ok}`; it does NOT map the tool-call `args` nor the
tool-result `detail`. The wire/journal DO carry them (`DecodedTranscriptBlock.argsSummary`,
`.detailKind`, `.detailBody`), so this is a modeling gap in the generated entity, not a wire gap.

Consequence: a mirror projection can reproduce the legacy bytes EXACTLY for
- user/system/assistant **Message** bubbles,
- **Reasoning** disclosures,
- **ToolCall** blocks whose `argsSummary` is empty AND whose result carried no structured detail
  (plain-text `summary` only),

but CANNOT for the common case of a ToolCall with non-empty `argsSummary` or a ToolResult with a
structured `detailKind`/`detailBody`. Closing that gap requires enriching the `TranscriptBlock`
entity (`args_summary`, `detail_kind`, `detail_body`) via `entity-map.toml` + `daemon-api.cddl` +
`just update-codec` — a node-contract / superproject change, OUT of A7T's charter (standalone
daemon-app worktree; cannot touch the node repo or run the codegen; generated files are
hand-edit-forbidden and drift-gated).

## Parity scenario list (the harness)

Drive the SAME coalesced block stream into (legacy) `DaemonCacheStore`→`CachedSessionStore` and
(mirror) `Ingestor`→`w_transcript_blocks`, assert `content()` byte-equality:

- S1 single user message — PARITY
- S2 user + assistant final message (bubble grouping) — PARITY
- S3 reasoning coalesced + settled + assistant message (assistant-group continuation) — PARITY
- S4 tool call, empty args, plain-text result (status ok) — PARITY
- S5 tool call, running (no result yet) — PARITY
- S6 `todo` tool call+result (suppressed) — PARITY
- S7 tool call with NON-EMPTY argsSummary — DIVERGES (entity gap)
- S8 tool result with structured detailKind/detailBody — DIVERGES (entity gap)
- S9 rebaseline clear + replay — PARITY (window cleared + re-fed)

## Design (single writer, §5.1; dual-write, §13 M4)

- `mirror::Batch::upsertWindow<T>` / `clearWindow<T>` (store.h substrate): cursor-ordered
  upsert-by-window-key (in-place replace on same key; diff-before-write) + per-scope window reset —
  the coalescing engine's `(session, seq)` upsert / `clearTranscript` twins.
- `mirror::Ingestor::deliverTranscriptBlock` / `clearTranscriptBlocks` (NEW methods — the ingestor
  stays the single mirror writer, §5.1; the engine feeds it, never the store directly).
- `daemonapp::daemon::ITranscriptMirrorSink` (daemon layer; the engine sees only
  `CachedTranscriptBlockRow` + `QString`, keeping `DaemonApp/Turn` free of mirror types) +
  `MirrorTranscriptSink` (substrate-gated adapter → the ingestor). Injected into
  `DaemonTurnEngine` via the factory/graph. The engine dual-writes at all 6 callsites.
- `MirrorSessionStore::mirrorContent(id)`: the mirror msg-fence projection (ported from
  `CachedSessionStore::content()`). `content()` KEEPS delegating to legacy (rollback = the facade
  delegation stays; the flip is the one-line swap of `content()`'s body to `mirrorContent(id)`).

## Status — DUAL-WRITE + PARITY HARNESS LANDED; FACADE FLIP WITHHELD (honest partial)

All gates green at the landing (full GUI+TUI+tests build; `ctest` 141→142, +1 =
`tst_mirror_transcript_parity`; clang-format + clang-tidy clean on every touched TU; no QML → no
qmllint; no user-facing strings → no i18n).

- **Write vertical — DONE.** `mirror::Batch::upsertWindow` / `clearWindow` (cursor-ordered
  upsert-by-key + per-scope reset, the coalescing engine's `(session, seq)` upsert / clearTranscript
  twins) + `Ingestor::deliverTranscriptBlock` / `clearTranscriptBlocks` (the single writer §5.1 —
  the engine feeds the ingestor, never the store). `map_transcript_block` left a stub (the
  wire-decode/offline-reload feed via SessionHistory is a distinct vertical AND equally blocked by
  the entity gap; the live app-local stream is the "single writer being re-homed" and is done).
- **Engine dual-write — DONE.** `ITranscriptMirrorSink` (mirror-free, engine-facing) +
  `MirrorTranscriptSink` (substrate-gated ingestor adapter), injected via the turn-engine factory /
  app service graph. `DaemonTurnEngine::persistTranscriptBlock` funnels every one of the 6
  callsites to cache + mirror; the rebaseline clear wipes both. Inert to the user (nothing reads
  the mirror transcript until the flip). Null (no-op) in mock / substrate-less / test stacks.
- **Read projection — DONE (unwired).** `MirrorSessionStore::mirrorContent()` — the msg-fence
  projection FROM the mirror window, ported grammar-for-grammar from `CachedSessionStore::content()`.
  `content()` STILL delegates to the legacy cache (rollback = the delegation stays; the flip is the
  one-line swap of `content()`'s body to `mirrorContent(id)`).
- **Parity harness — DONE.** `tst_mirror_transcript_parity` drives the SAME coalesced block stream
  through both paths via the PRODUCTION sink and asserts `content()` byte-equality. Result matrix:
  - PARITY (byte-identical): S1 user msg, S2 user+assistant, S3 reasoning+assistant-group,
    S4 tool call empty-args + plain result, S5 tool call running, S6 todo suppressed,
    S9 rebaseline clear+replay, reasoning re-checkpoint in-place, out-of-order sorted insert.
  - DIVERGES (pinned): S7 tool call with NON-EMPTY args, S8 tool result with structured detail.

### Flip decision — WITHHELD (byte-parity cannot be proven on tool scenarios)

The facade flip is NOT landed. Proven root cause (S7/S8): the generated `mirror::TranscriptBlock`
entity carries no `argsSummary`, `detailKind`, or `detailBody`, so the mirror `` ```tool `` fence
cannot reproduce the legacy fence for the COMMON case of a tool call with args or a structured
result. Flipping `content()` to `mirrorContent()` now would reflow/strip tool-args + detail cards on
every reload — a regression to the one surface §13 ring-fenced. Per the M4 blast-radius rule (an
honest clean-gated partial beats a broken total), the legacy delegation stays; the write path +
projection + harness land so the eventual flip is one line.

**Exact unblock (out of A7T's charter — node-contract change):** enrich the `TranscriptBlock`
entity with `args_summary`, `detail_kind`, `detail_body` in `daemon-api.cddl` +
`src/core/mirror/entity-map.toml` (map `transcript-block-tool-call.args` and
`transcript-block-tool-result.detail`), regenerate via the superproject `just update-codec`, then:
extend `MirrorTranscriptSink::toMirrorBlock` + `map_transcript_block` to carry the three fields,
extend `projectMirrorTranscript` to emit them (delete the `argsSummary=""` hardcode + re-add the
detail fold), change `tst_mirror_transcript_parity` S7/S8 from `!=` to `QCOMPARE` (they flip to
PARITY — the green signal), then flip `MirrorSessionStore::content()` to `mirrorContent(id)` and
add a `TranscriptBlock`-delta arm to `onCommitted()` so `changed()` fires on transcript writes once
the legacy `changed()` relay is retired.

> **EXECUTED by G2 (M5) — see LEDGER-g2.** One correction to the plan above: NO CDDL change was
> needed (the wire already carried `args_summary` + `detail` on both tool arms — the gap was
> map-only), so the enrichment was `entity-map.toml` + emitter rerun, no version bump. Every other
> step landed as written: sink + `map_transcript_block` (real body) + projection fold, S7/S8
> flipped to `QCOMPARE` and went byte-identical-green, `content()` = `mirrorContent(id)`, and the
> `TranscriptBlock` delta arm fires `changed()` (the legacy relay stays for the dual-write window;
> both firing is a benign double-refresh). `CachedSessionStore::content()` is RETAINED solely as
> the parity-harness baseline — it dies with the store in AD.

## What this landing unlocks (deletion order, extending LEDGER-a7 §"deletion order")

A7's order was: engine re-home → content() mirror projection → drop the CachedSessionStore
delegation → retire SubscriptionManager roster arms → delete CachedSessionStore + SessionRepository
cache feed → drop the SessionStore context property. A7T advances the FIRST TWO in code but gates
their ACTIVATION on the entity enrichment:

- The engine write path IS re-homed (dual-write live) and the content() mirror projection EXISTS
  and is parity-proven for the representable kinds — so once the entity carries args/detail, the
  flip + the `w_transcript_blocks` read is a one-line/one-arm change (documented above), with the
  parity harness already in place as the gate.
- Until the flip lands, `CachedSessionStore::content()` remains the transcript source and the
  parity baseline; `CachedSessionStore` / `SessionRepository` / the `SubscriptionManager` roster
  arms stay RETAINED (unchanged from LEDGER-a7). A7T deletes nothing — deletion still waits on the
  flip AND the mutation-outbox lane (A8/A9), per LEDGER-a7.

> **G2 update:** the flip is LANDED (full parity matrix byte-identical). `content()` no longer
> delegates; `CachedSessionStore::content()`'s remaining role is parity baseline ONLY (its last
> production reader is gone). The AD deletions this advances are recorded in LEDGER-a7's
> deletion-order note and LEDGER-g2.

## Coordination notes for the parent (merging alongside A8)

- Ingestor surface: added ONLY two new methods (`deliverTranscriptBlock`, `clearTranscriptBlocks`)
  appended after `deliverFleetUnits` — no edits to existing ingestor methods. A8's ingestor.cpp
  provenance-landing edits should not overlap; conflicts (if any) are additive-adjacent.
- `store.h`: added `Batch::upsertWindow` / `clearWindow` (new templates; existing `appendWindow`
  untouched). `tests/unit/CMakeLists.txt`: one new test block appended after `tst_mirror_session_store`.
- Mock mode is untouched: no sink is constructed in mock (`transcriptMirrorSink` stays null), so
  `graph.storeMirror` aliasing + mock transcript rendering are unaffected — A8 owns the mock seam.
