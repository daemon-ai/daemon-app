# A3 — generic adapter + stable ids + role maps + observe-seam finalization: TDD ledger

Package A3 of the Mirror Architecture program (spec 09 §8, §4.6, §12, §14.8/§14.9; ADR-008).
Base: `integrations/app` @ `0073a17` (A1 mirror substrate + A2 outbox + G1 entities merged).
Worktree branch `mirror/a3`.

## Scope (owned by A3)

1. `mirror::TableModel<Entity>` — a generic `QAbstractListModel` that consumes its kind's journal
   deltas ABOVE ITS WATERMARK and translates them into EXACT row ops
   (`beginInsertRows`/`beginRemoveRows`/`dataChanged`/`beginMoveRows`) through a maintained
   sort-key index. NEVER `beginResetModel` outside initial population (§14.8). Sorted-projection
   support (§8.1 ordering contract: node-supplied ordering keys, presentation sort only).
2. 64-bit stable-id interning: entity keys → monotonic `int64`, never reused, bijective for the
   adapter lifetime — fixes Sink's 32-bit `qHash` collision defect
   (`references/architecture/sink/common/modelresult.cpp:32-41`, anti-model). Survives resets;
   bounded-growth policy documented.
3. `mirror::WindowModel<Entity>` — the class-W (windowed) adapter over A1's boxed window items
   (`immer::flex_vector<immer::box<T>>`) + `window_meta`; the demand-paging "request more" signal
   seam (`olderRequested`, `canFetchMore`/`fetchMore`) — A4 owns the fetch, A3 owns the seam.
4. Per-VM role maps (§8.2/§8.3): the generic adapter auto-exposes Q_GADGET properties as roles;
   the base + ONE curated demonstrator (`ConversationListModel`, a read-only list M2 will
   formalize) show the ~30–60-line curated-role-map pattern. Pattern demonstrator + tests, NOT a
   consumer migration (that is A5+).
5. Observe-seam finalization (ADR-008): re-run gate 1 (compile cost) on a REPRESENTATIVE VM TU
   set (≥5 TUs incl. one QML-heavy + one TUI consumer; clean + incremental). Confirm A1's default
   (coarse) or contradict it (lager). Delete the loser; retarget glitch-freedom /
   transactional-visibility invariants at the surviving seam; dispose of the lager/zug flake
   inputs per the outcome.
6. Model-tester harness: every `TableModel`/`WindowModel` instantiation runs under
   `QAbstractItemModelTester` in tests (§12).

## Concurrency division

Sibling A4 builds the Ingestor (`src/core/mirror/ingestor*`) and is the store's single WRITER.
A3 only READS (journal deltas + snapshots) and registers journal CONSUMER watermarks (§4.4 — not
a mirrored-state write). A3 does not touch `subscription_manager` or create ingestor code.

## TDD protocol

Ledger → RED (failing tests committed) → GREEN → refactor + gate. Tests never weakened. Hooks
reject `#if 0`/commented tests, so RED lands with `QSKIP("RED: <reason>")` markers (QtTest's
DISABLED_ equivalent); GREEN removes the skips.

## Commits (planned)

1. `ledger` — this file.
2. `RED` — `tst_mirror_stable_id`, `tst_mirror_table_model`, `tst_mirror_window_model` land
   failing (QSKIP-guarded) with the invariants written first; CMake wiring for the (empty) lens
   target so the suite links.
3. `GREEN stable-id` — `StableIdInterner` (monotonic int64, never-reused, reset-surviving).
4. `GREEN adapter` — `TableModelBase` (Q_OBJECT: journal-consumer watermark, delta→row-op
   dispatch, stable-id invokables, demand-paging signal) + `TableModel<Entity>` template
   (sorted projection, auto Q_GADGET role map, exact insert/remove/move/dataChanged).
5. `GREEN window + role map` — `WindowModel<Entity>` (boxed items, `window_meta`, demand-paging
   seam) + `ConversationListModel` curated-role-map demonstrator.
6. `observe` — gate-1 re-run on the representative VM TU set (numbers recorded); delete the loser
   + retarget invariants + dispose flake inputs per outcome.
7. `refactor + gate` — clang-format/tidy, qmllint (to log), build GUI+TUI, ctest.

## Invariants under test (spec refs)

- §8.1 delta path: adapter consumes `journal.since(watermark, kind)`; upsert→insert-or-move-or-
  dataChanged; tombstone→remove; ordered. NO reset after initial population (§14.8) — asserted by
  a `modelAboutToBeReset` counter that stays 0 post-construction.
- §8.1 sorted projection: rows stay in the comparator order; a sort-key change moves the row to
  its new sorted position (exact `beginMoveRows`, never a reset).
- Property style: random upsert/tombstone delta sequences ⇒ the model's rows exactly mirror the
  store table (sorted keys + field values); `QAbstractItemModelTester` attached throughout.
- Stable ids: interning is monotonic, never reused, stable across value updates and across a
  remove→re-add of the same key; survives a model repopulation; distinct from row index.
- §4.6 windows: `WindowModel` reads boxed items in cursor order; append→insert at tail;
  eviction→remove at head; `canFetchMore`/`fetchMore` emit `olderRequested(scope, n)`.
- §8.3 role maps: default adapter roleNames = one role per Q_GADGET property + a StableId role;
  the curated demonstrator overrides with a renamed subset; both read typed fields (no
  QVariantMap row shapes, §14.10).
- ADR-008 gate 1: representative VM TU compile-cost delta recorded (clean + incremental).

## Ported-test provenance (license-disciplined re-expression)

- Sink (LGPL — behaviors re-expressed, NEVER text):
  - live incremental add/remove reaching the model: `tests/querytest.cpp:903-943`.
  - delta→row-op dispatch precedent (add/modify/remove into begin*/end* on a sorted index):
    `common/queryrunner.cpp:294-341`.
  - the 32-bit `qHash(resourceId+entityId)` id collision that our monotonic int64 interning
    fixes: `common/modelresult.cpp:32-41` (anti-model).
- immer (BSL-1.0): boxed window items + flex_vector semantics as consumed by `WindowModel`
  (behaviors already exercised in A1's `tst_mirror_store`).

## Integration points left for siblings

- A4 (Ingestor): fulfils `olderRequested(scopeKey, count)` by issuing a backward-window fetch
  (`before_cursor`, §10.2) / forward-fill on v38; sets `WindowModel::setHasMoreOlder(false)` when
  a scope reaches cursor 0. A4 is the single writer; A3 adapters connect to `Store::committed`.
- A5 (M2 vertical): migrates real consumers onto `TableModel`/`WindowModel` subclasses following
  the `ConversationListModel` curated-role-map pattern; wires GUI (QML) + TUI (`DisplayRoleAdapter`).
  A3 ships the GUI-free shared layer + the pattern only.
