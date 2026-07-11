# G2 — entity enrichment: transcript flip + fleet-tree port (M5): TDD ledger

Package G2 of the Mirror Architecture program (spec 09 §3.3/§3.5/§3.6, §13). Base:
`integrations/app` @ `b4789c5` (A7T merge; includes A8's M5 foundations) and
`integrations/node` @ `0dbd720` (WireVersion 39). Worktree branches `mirror/g2` (app) +
`mirror/g2-node` (node). Charter: close the two PROVEN entity gaps blocking the program's final
deletions — the `TranscriptBlock` args/detail gap (A7T's withheld flip) and the `FleetUnit`
parent/children gap (A7's retained legacy fleet tree) — through the CODEGEN pipeline, then land
the two unblocked ports.

## Path decision per gap — BOTH MAP-ONLY (no CDDL change, no version bump)

Established by inspecting the v39 contract (`daemon-api.cddl` in the g2-node worktree) BEFORE
touching anything:

- **TranscriptBlock**: the wire ALREADY carries every missing field.
  `transcript-block-tool-call = { "call_id", "name", "args_summary", ? "detail" }` and
  `transcript-block-tool-result = { "call_id", "ok", "summary", ? "detail" }` with
  `tool-detail = { "kind", "body" }` (`byte-array = bstr`). A7T's ledger had already proven this
  ("the wire/journal DO carry them — a modeling gap in the generated entity, not a wire gap").
- **FleetUnit**: `unit-node` ALREADY carries `"children": [* unit-id]` (the ordered edge) and
  `? "engine": (engine-selector / null)` (v29). The tree needs no parent pointer on the wire:
  the root is the unit no child list names; parent/depth are derivable at projection time.

Consequence: `mirror/g2-node` is UNTOUCHED (zero commits; stays at `0dbd720`). No WireVersion
bump, no conformance/fixture churn, no `NodeApiCodec::kDaemonApiVersion` change, no codec regen.

## Regen mechanics (established, exact commands)

The emitter is the SECOND update-codec emitter (ADR-004): pure Python stdlib, hermetic, living in
the contract-owning repo — `daemon-node/crates/contracts/daemon-api/mirror-codegen/`. The
superproject flake wires it (`daemon-mirror-entities` derivation + the `codec-drift` check + the
`update-codec` app), but it runs directly against worktree files (no superproject mutation, no
recipe change needed — so NO g2-superproject.patch):

```
# regenerate the 4 vendored artifacts (byte-identical, no timestamps):
python3 <node>/crates/contracts/daemon-api/mirror-codegen/entity_codegen.py \
  --cddl <node>/crates/contracts/daemon-api/daemon-api.cddl \
  --map  <app>/src/core/mirror/entity-map.toml \
  --out  <app>/src/core/mirror/generated

# the drift gate (the entity half of `just codec-drift`):
python3 <node>/crates/contracts/daemon-api/mirror-codegen/entity_drift.py \
  --cddl <node>/.../daemon-api.cddl --map <app>/src/core/mirror/entity-map.toml \
  --generated-dir <app>/src/core/mirror/generated \
  --map-cpp <app>/src/core/mirror/entities_map.cpp \
  --types-header <app>/src/core/daemon/codec/generated/daemon_api_client_types.h
```

Gate result at landing: `mirror entity artifacts + mapper signatures match the generated output`
(byte-identical regen + provenance grounding + mapper-signature check all green).

**Pipeline finding (recorded for the next entity worker):** the emitter infers each mapper's
source DTO as the MODAL rule across the entity's field wire-paths. Adding the two
`transcript-block-tool-result.*` fields flipped `map_transcript_block`'s inferred source from
`journal-record` to `transcript_block_tool_result` (drift gate caught it as a signature
mismatch). Fixed by pinning `source = "journal-record"` in `[entity.TranscriptBlock]` — the
supported explicit override; pin the source whenever a union-modeled entity gains variant fields.

## Gap 1 — TranscriptBlock enrichment + the content() flip

- `entity-map.toml [entity.TranscriptBlock]`: + `args_summary` (`transcript-block-tool-call.
  args_summary`), `detail_kind` (`transcript-block-tool-result.detail#kind`), `detail_body`
  (`transcript-block-tool-result.detail#body`, byte-array decoded as UTF-8 — the exact transform
  the legacy fence applied at render time), + the `source = "journal-record"` pin.
- `MirrorTranscriptSink::toMirrorBlock` (the app-local engine feed): carries the three fields
  (args verbatim; `detail_body = QString::fromUtf8(row.detailBody)`).
- `map_transcript_block` (the wire-decode/offline-reload feed; was A7T's documented stub): full
  body over `::journal_record` — seq + origin_op off the envelope, the Block payload's union arm
  mapped per variant (Message role/text via the new `transcriptRole` helper; ToolCall
  call_id/name/args_summary; ToolResult ok/summary + tool-detail kind/body; Request/Content =
  discriminator only). `session` stays caller-stamped (the SessionHistory request scope, §3.6
  merging rule, like map_chat_message). A Management/Chat record maps to a kind-less row the
  caller skips. Covered by direct-DTO mapper tests in `tst_mirror_transcript_parity` (there is
  deliberately NO SessionHistory→mirror decode wrapper yet: the offline path is served by the
  engine's journal-rebaseline replay through the SAME sink; the wire-decode vertical joins when a
  consumer needs it — the mapper is ready).
- `projectMirrorTranscript`: emits `argsSummary` from the row (the `""` hardcode deleted) and
  folds `detailKind` + `detailBody` when present — byte-for-byte the legacy grammar (QJsonObject
  key sort, insert order irrelevant).
- **THE FLIP**: `MirrorSessionStore::content()` = `mirrorContent(id)` (A7T's one-line swap,
  executed). `onCommitted()` gained the TranscriptBlock delta arm: a transcript delta fires
  `changed()` WITHOUT a session-row rebuild (§8.1 discipline), so the detail pane / exporter
  refresh on engine writes and offline reloads through the mirror path.
- **Parity evidence**: `tst_mirror_transcript_parity` S7 (non-empty args) and S8 (structured
  detail) flipped from pinned-divergence (`QVERIFY(p.mirror != p.legacy)`) to byte-equality
  (`QCOMPARE(p.mirror, p.legacy)`) — no scenario weakened, S1–S6/S9 + the coalescing/out-of-order
  cases unchanged and green. Full matrix now byte-identical, 17 test functions green.

### What the flip orphaned — deleted vs retained (references-verified)

`CachedSessionStore::content()`'s msg-fence projection lost its LAST PRODUCTION reader (the
facade no longer delegates; every surface reads through `MirrorSessionStore`). It is **retained,
not deleted**, for exactly one load-bearing role: it IS the parity baseline the harness compares
the mirror projection against (`tst_mirror_transcript_parity` drives both paths; deleting the
legacy leg would gut the byte-parity gate the program mandates stays unweakened). It dies with
`CachedSessionStore` wholesale in AD, together with:

- the engine's cache dual-write (`DaemonCacheStore::upsertTranscriptBlock` / `clearTranscript`
  callsites in `daemon_turn_engine.cpp`) — the parity harness's legacy feed;
- `DaemonCacheStore::transcriptBlocks()` (its only reader is the legacy projection);
- `MirrorSessionStore::setContent()`'s legacy delegation (a no-op on the daemon store — the
  engine owns transcript writes; kept only as interface compliance).

Also retained (unchanged from A7): the legacy `changed()` relay (mutation echoes through the
legacy cache still refresh promptly during the dual-write window) and the mutation delegation
(ONE node-authoritative wire path). `tst_mirror_session_store`'s three delegation tests were
re-expressed post-flip (content projects the mirror, ZERO legacy content reads asserted;
mutations still delegate; exporter composes mirror title + mirror content).

## Gap 2 — FleetUnit edges + the fleet-tree port (+ DELETION)

- `entity-map.toml [entity.FleetUnit]`: + `child_ids` (`unit-node.children`, the ORDERED child
  list joined with 0x1f — the §3.5 unitsByParent relation, kept verbatim so the render preserves
  the node's listed child order), `engine` (`unit-node.engine`, "Core"/"Foreign"/""),
  `engine_agent` (`unit-node.engine#agent`). `child_count` retained (the sidebar reads it).
- `map_fleet_unit`: joins the children array, decodes the v29 engine-selector (Core / Foreign +
  agent). Round-trip covered in `tst_mirror_sessions_fleet` (tree fixture grew a foreign-engine
  child; asserts the 0x1f edge order + engine fields + empty-edge leaf).
- **`fleet::MirrorFleetTree : IFleetTree`** (`src/core/daemon/mirror_fleet_tree.{h,cpp}`): the
  tree consumer ported onto the mirror. Rows are a PURE projection of `snapshot().fleet_units`:
  roots = units no child list names (sorted for a deterministic multi-root render), pre-order DFS
  assigns depth, children pop in listed order (the legacy flattenTreeNodes rule), cycle-guarded.
  Row shape is IDENTICAL to the legacy rows (`depth,id,name,kind,status,model,engine,engineAgent,
  lifetime,sessionId,role`) so FleetPage.qml + the TUI ops hub bind UNCHANGED (both consume the
  `IFleetTree` interface — zero QML / zero `src/tui` diffs; GUI+TUI parity by construction).
  Re-derives on FleetUnit journal deltas only (watermarked consumer, §8.1). The client-local
  paused overlay + controlRejected revert behavior preserved bit-for-bit.
- **Mutations stay node-authoritative**: pause/resume/scale delegate to `FleetRepository` (the
  wire control seam). `refresh()` = `Ingestor::refetchFleet()` (NEW: the on-demand `FetchOp::Tree`
  enqueue — the A4/A7 fetch arm + `FleetChanged` policy row already existed). A control-Ok's
  `treeRefreshed` is bridged to `refetchFleet()` so the post-ack re-render reaches the READ model
  deterministically (belt-and-braces beside the FleetChanged event; the scheduler coalesce-key
  dedups the overlap).
- **Wiring** (`app_service_graph.cpp`): the mock fleet tree is deleted and replaced with
  `MirrorFleetTree` INSIDE the daemon+substrate mirror block (it needs the mirror store). Mock
  mode is untouched (MockFleetTree stays; A8's cutover swaps it when the seeder feeds
  fleet_units — the coordinate is the generated entity's `child_ids` 0x1f shape ONLY). A
  substrate-less daemon build (hypothetical — every shipped config pins immer) keeps the mock
  tree; documented degradation, not worth a third implementation.
- **DELETED**: `daemon_fleet_tree.{h,cpp}` (the legacy consumer), its CMake entries, and
  `tst_daemon_fleet_tree.cpp` (+ its CMake block). Zero-reference grep clean: the only remaining
  `DaemonFleetTree` mentions are historical comments/ledgers. Still-relevant behaviors
  re-expressed on the mirror path in `tst_mirror_fleet_tree` (7 tests: seeded pre-order render,
  enrichment projection, delta-only live update, cycle guard, refresh + control-ack refetch,
  Unsupported rejection reverting the optimistic pause via the WireMux fixture).
- Stale-comment sweep: `seam_migration.h` IFleetTree entry rewritten (MIRRORED, G2),
  `client_cache_schema.h` v4 note (reader deleted; feed = dual-write baseline → AD),
  `daemon_session_roster.h` / `daemon_transport_registry.h` / `daemon_profile_store.cpp` /
  `daemon_turn_engine.cpp` cross-references updated.

### Fleet legacy left for AD (references-verified, recorded)

`FleetRepository`'s TREE FEED is now reader-less but still DRIVEN (SubscriptionManager's
FleetChanged arm + control-Ok both call `refreshTree`, which fetches Tree pages and writes the
`daemon_fleet_units` cache): `cachedUnits()`, `treeRefreshed` (one consumer left: the
MirrorFleetTree→refetchFleet bridge), the `syncFleetUnits` cache write, the `daemon_fleet_units`
table + `client_cache_schema.h` v4 DDL, and `CachedFleetUnitRow`. Deleting the feed now would
either sever the control-ack ack→refetch bridge or reach into SubscriptionManager (A7's ledger
keeps its arms retained as the dual-write baseline). AD deletes the feed + table + row struct and
re-points the ack bridge (or drops it — by then FleetChanged-only is proven); the CONTROL half of
FleetRepository (pause/resume/scale + controlFailed) must survive AD.

## Persistence

`mirror_schema_gen.sql` gained the three `m_fleet_units` columns → `Persistence::kSchemaVersion`
11 → 12 (disposable cache: mismatch drops-and-rebuilds; no migration by design). Window payloads
(TranscriptBlock) serialize via the generic Q_GADGET dump — the new fields ride automatically;
old persisted rows decode with defaulted new fields (and the fleet table is rebuilt on the
version bump anyway). NOTE: fleet_units M-rows are not yet in the write-behind's persisted set
(only Conversation/Contact/Person/ChatMessage are — pre-existing A1/A5 posture, "other M tables
wired as verticals land"); the fleet tree re-fetches on connect (FleetChanged/staleness scan), so
offline-first parity with the legacy cache tree is the documented degradation until the
write-behind grows the fleet arm (AD or the next persistence package).

## Gates at landing

- Entity drift gate: byte-identical green (command above).
- App: full GUI (`daemon-app`) + TUI (`daemon-tui`) + tests build green; **ctest 145/145**
  (baseline 145 → −1 `tst_daemon_fleet_tree` deleted, +1 `tst_mirror_fleet_tree` = 145; the
  parity/store/sessions-fleet suites extended in place).
- clang-format + clang-tidy clean on every touched TU; no QML touched (no qmllint delta); no
  user-visible strings (no i18n delta; seam_migration/ledger text is dev-facing, not tr()'d).
- Node: untouched (no cargo gates owed).

## Coordination notes (parent merge, alongside A8)

- Files touched that A8 also owns NOTHING of: seeder untouched (`upsertPage` is generic over the
  entity; the FleetUnit field additions compile through). A8's mock cutover should seed
  `fleet_units` with `child_ids` = 0x1f-joined ordered ids (+ optional engine/engine_agent) and
  swap the mock `graph.fleetTree` to a `MirrorFleetTree` over the seeded store — the daemon-side
  class takes (store, ingestor, control) and tolerates null ingestor/control (refresh/pause
  become no-ops, matching MockFleetTree's inert refresh today).
- `ingestor.{h,cpp}`: ONE new method (`refetchFleet`), appended after `refetchRooms` — additive,
  no existing-method edits (same discipline as A7T's transcript methods).
- LEDGER-a8.md line 65 ("the fleet TREE stays legacy") is now stale — left untouched (A8 owns
  that file and a sibling worker is live in it; the correction is recorded HERE and in
  LEDGER-a7's deletion order).
- `entities_gen.h` / provenance / schema / `entities_map_gen.h` regenerated — any sibling that
  regenerates must do so from THIS entity-map.toml or the drift gate flags it.
