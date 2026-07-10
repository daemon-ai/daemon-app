# A1 — mirror substrate: TDD ledger

Package A1 of the Mirror Architecture program (spec 09 §4, §11; ADR-002/-001/-008).
Base: `integrations/app` @ `57bfb28` (G1 entity codegen merged). Worktree branch `mirror/a1`.

Scope (owned by A1): vendored immer/zug/lager subset; `mirror::MirrorModel` root; the
journal ACTIVE in degraded mode; `mirror-<id>.db` persistence (schema from
`mirror_schema_gen.sql`) with watermarks + compaction; the observe-seam spike (ADR-008);
WASM smoke gates. A2 owns `local-<id>.db` (outbox/sidecar) — A1 exposes a DB-path/handle
provider integration point only.

## TDD protocol

Ledger → RED (failing tests committed) → GREEN → refactor + gate. Tests are never weakened.
Because the hooks reject `#if 0`/commented-out tests, RED lands with `QSKIP("RED: <reason>")`
markers (the QtTest DISABLED_ equivalent) so the suite is green-but-skipping until GREEN
removes the skips.

## Commits (planned)

1. `ledger` — this file.
2. `vendor` — immer/zug/lager flake inputs + `Dependencies.cmake` wiring + `IMMER_*` ODR
   flags in one preset; a compile-only probe target proving immer+lager build in-tree.
3. `RED` — `tst_mirror_store`, `tst_mirror_journal`, `tst_mirror_persistence`,
   `tst_mirror_observe`, ported `tst_mirror_immer` / `tst_mirror_lager` / `tst_mirror_sink`
   land failing (QSKIP-guarded) with the invariants written first.
4. `GREEN store` — MirrorModel root + Store apply/commit two-phase + journal (deque + rev
   mint + watermarks + compaction floor).
5. `GREEN persist` — mirror.db writer/loader (one txn: rows + journal + watermark + cursor),
   schema-version + migration scaffold, compaction sweep.
6. `GREEN observe` — both `mirror::Observe` implementations (lager headers; coarse signals).
7. `refactor + gate` — clang-format/tidy, qmllint (to log), build GUI+TUI, ctest, wasm gates.

## Invariants under test (spec refs)

- §4.1 root value: `immer::table` per M/L kind (key via `entity.key()`); W kinds = per-scope
  cursor-ordered windows. Page ingest = transient bulk insert + ONE commit.
- §4.2 commit discipline: one commit per applied batch; every observer sees one atomic
  snapshot + one notification wave. Single writer.
- §4.3 journal: `(rev, kind, key, op∈{upsert,tombstone}, origin∈{wire_delta,refetch_diff,
  seeder}, origin_op?, at_ms)`; store-wide monotonic in-memory rev, seeded from persisted
  MAX(rev); every commit appends journal records. Degraded = `refetch_diff` origin.
- §4.4 watermarks: every consumer holds a watermark; compaction floor = min(all live
  watermarks) minus retention tail (4096 rows / 24h). journal replay ≡ snapshot diff.
- §4.5 persistence: open/create mirror.db from `mirror_schema_gen.sql`; write-behind txn =
  rows + journal + watermark + cursor atomically; boot load via transients; schema version.
  local-db is A2's — path/handle provider only.
- §11: single-threaded; `IMMER_NO_THREAD_SAFETY` + exception flags set once per preset.

## Ported-test provenance (license-disciplined re-expression)

- immer (BSL-1.0, adapt+attribute): table upsert/patch `test/table/generic.ipp:134-147,
  268-302,496-571`; diff `test/algorithm.cpp:72-98` + `test/table/generic.ipp:549-571`;
  transient bulk `test/table_transient/generic.ipp:35-93`; fuzz-shape `extra/fuzzer/
  fuzzer_input.hpp:22-50`.
- lager (MIT, adapt+attribute): transactional visibility `test/state.cpp:51-74,102-141`;
  glitch-free two-phase `test/detail/nodes.cpp:36-88,138-164`; cursor `test/cursor.cpp:81-107`.
- Sink (LGPL — behaviors re-expressed, NEVER text): journal ordered add/modify/remove
  `tests/entitystoretest.cpp:33-137`; write-behind visibility `tests/storagetest.cpp:481-504`;
  pipeline replay `tests/pipelinetest.cpp:238-284,333-363`. Floor math re-expressed from
  `common/listener.cpp:308-320`, `common/genericresource.cpp:163-173`; sweep from
  `entitystore.cpp:361-421`.
- §4.4 property test (ours): journal replay over prev snapshot ≡ new snapshot (immer::diff).

## Integration points left for siblings

- A2: `local-<id>.db` (outbox/sidecar). A1 persistence takes a `DbPathProvider` — the parent
  wires A2's local-db beside mirror.db.
- A3: `mirror::TableModel<Entity>` consumes journal deltas above a watermark (the watermark
  consumer contract + delta accessor are provided here).
- A4: the ingestor is the single Writer; write-behind is its durable consumer (this package
  provides the transactional writer API + watermark registry).
