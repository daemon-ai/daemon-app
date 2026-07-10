# A5 — M2 integrations vertical (first real consumer on the mirror): TDD ledger

Package A5 of the Mirror Architecture program (spec 09 §13 wave M2; §7, §8.4, §6.7/§6.8, §9,
§12 E1, §14). Base: `integrations/app` @ `bda9580` (A1 substrate + A2 outbox + A3 lenses +
A4 ingestor merged). Worktree branch `mirror/a5`.

## Scope (owned by A5)

1. **Wire the ingestor live** (the A4 seam, §5.1): instantiate the mirror substrate
   (`mirror::MirrorService` — Observe/Store/Persistence/WriteBehind/FetchScheduler/Ingestor) in
   the app service graph in BOTH modes, feed it the node event stream through the daemon bridge
   (`translateNodeEvent`), and drive fetches over a live `mirror::FetchExecutor`:
   - daemon mode: `daemon::DaemonFetchExecutor` over the real `NodeApiClient` + the vendored
     codec's new **decode-to-mirror** helpers (`NodeApiCodec::decode*ToMirror`, calling the G1
     generated mappers — the codec stays confined to the bridge, 07§10).
   - mock mode: a `mirror::SeededFetchExecutor` fed from the existing mock services so mock keeps
     working at M2 (§9); the full seeder reform is A8's — this is a documented seam.
   The write-behind consumer drains to `mirror-<id>.db`. The legacy `SubscriptionManager` +
   repository paths stay ALIVE (dual-write continues, §13 M0–M2 discipline); reads repoint
   per-surface only where the mirror path replaces them.
2. **Migrate the M2 read surfaces onto mirror lenses** (ConversationListModel pattern; GUI+TUI):
   chat conversation list + chat message window (`ChatWindowModel` over `WindowModel<ChatMessage>`
   with `olderRequested`), persons list, contacts list, and the integrations-tree conversation
   rows. Their M-tables become the live read path; the corresponding per-surface read
   caches/row-shapes (07§3/§6: conversation 5→1, contact/person 4→1) are retired on migrated
   surfaces. Rule zero: no new ad-hoc shapes — role maps read the generated entity.
3. **Pending strip** (§8.4): `PendingOpsModel` filtered per-conversation (`chat-send␟transport␟
   conv`), rendered BESIDE the chat timeline (GUI + TUI). Pending ops render distinct from node
   rows (R2: never a mirror row); rejection states surface retry/edit/discard (A2's affordances).
4. **ConvSend through the outbox lane** (§7/§6.8): `mirror::ConvSendController` enqueues ConvSend
   to the `chat-send` lane; manual drain against api/38 (auto once api/39 seen — that gate stays
   OFF; asserted). Optimistic render via the pending strip, never a faked mirror row
   (outbox-not-mirror-echo, ADR-003/-005).
5. **Offline render asserts**: cold boot with no connection renders migrated surfaces from
   `mirror-<id>.db` (boot-load path); E1 parity per §12 (last-known state + pending ops).
6. **Parity asserts stay on** (A4's `parity::compareKeys`) for migrated surfaces in dev/CI.

## Architecture — MirrorService is the app-graph composition root

A4 shipped the ingestor DECOUPLED (a `FetchExecutor` seam + `deliver*()`, unit-tested with a
recording mock). A5 composes the live graph:

```
NodeApiClient.streamItem ─▶ decodeEventsPage ─▶ translateNodeEvent ─▶ Ingestor.ingest
                                                                          │
Ingestor ─▶ FetchScheduler ─▶ FetchExecutor.execute(job)                  ▼
  (daemon: DaemonFetchExecutor → NodeApiClient wire op → decode*ToMirror → deliver*)
  (mock:   SeededFetchExecutor → mock service rows → mapped entities → deliver*)
                                                                          │
                                                    Store.commit ─▶ Journal ─▶ WriteBehind ─▶ mirror.db
                                                                          │
                                                    lenses (TableModel/WindowModel) ◀─ committed
```

`MirrorService` is mode-agnostic; the executor is the only mode-specific piece. It boots from
`mirror-<id>.db` (offline render), starts write-behind flush-on-commit, and exposes `Store&`,
`Ingestor&`, and the `Outbox` for the lens/lane consumers.

## TDD protocol

Ledger → RED (failing tests committed, QSKIP-guarded where a suite must stay green-but-skipping)
→ GREEN → refactor + gate. Tests never weakened. Committed in coherent RED/GREEN slices.

## Invariants under test (spec refs)

- §5.1 live wiring: an event on the stream reaches `Store` through the ingestor; a fetch job
  reaches the executor; the executor's decoded entities land as journal deltas → write-behind →
  `mirror.db`. One writer (ingestor); dual-write leaves the legacy repositories untouched.
- §4.5/§12 offline render: after a write-behind flush, a fresh `MirrorService` boot (no
  connection) reloads the M tables + chat window from `mirror.db`; the lens rows match.
- §6.4/§6.8 ConvSend lane: `send()` enqueues to `chat-send␟t␟c` (durable BEFORE any wire call);
  a pending row appears in the per-conversation strip; manual `drain()` emits `sendRequested`;
  auto-replay stays OFF for api/38 AND api/39 (asserted); a rejection pauses the lane and
  surfaces retry/edit/discard.
- §6.6/R2 no fake rows: a pending ConvSend is NEVER a `ChatMessage` row; the timeline composes
  the window model + the pending model as two sources; the confirmed line only appears when the
  node's MessagesChanged → ConvHistory delta lands (provenance-keyed, uniform).
- §8.1/§8.3 lenses: `ChatWindowModel`/`PersonListModel`/`ContactListModel` read typed entity
  fields via curated role maps (no QVariantMap shape); exact row ops under
  `QAbstractItemModelTester`; `olderRequested` on the chat window demand-page.
- §12 parity: at quiesce the migrated surfaces' mirror row-set ≡ the legacy repository row-set
  (key parity via `parity::compareKeys`).

## Ported-test provenance (license-disciplined re-expression)

- E1/E4 scenario parity (spec §12, ours): a scripted decoded-reply stream → the executor →
  the store → lenses; the same stream replayed offline (boot from `mirror.db`) yields the same
  rows. No Sink/Akonadi source text.

## Deletions (mapped to 07 baselines — staged under dual-write)

- Conversation 5 layouts → 1 (07§6): the integrations-tree/registry conversation row shapes give
  way to `Conversation` entity role maps on migrated read surfaces.
- Contact/person 4 layouts → 1 (07§6): `Contact`/`Person` entity role maps.
- Per-surface read caches of Chat/Persons/Contacts (07§3 "not cached offline") are replaced by the
  mirror's persisted M-tables + chat window on the migrated surfaces; the repositories' WRITE
  paths (fetch cores) remain as A4's dual-write feeders until A6+ removes them.

## Completion notes (second pass — parent rulings applied)

- **Offline chat timeline**: `Persistence::loadInto` rebuilds each scope's persisted CONTIGUOUS
  TAIL from `w_chat_messages`/`window_meta` (newest-first, bounded by the §4.6 policy max,
  reversed into cursor order); `MirrorService::open` seeds the per-conv ConvHistory cursors from
  the reloaded `newest_cursor` so the reconnect top-up resumes AFTER the tail (no duplicate /
  disordered appends). E1 offline render includes the timeline (tst_mirror_e1_offline).
- **Per-identity live mirror**: the daemon graph opens `mirror-<sha256[:16]>.db` under the LAST
  authenticated identity (persisted `mirror/lastIdentity`), same hash convention as
  DaemonCacheStore + LocalDatabase; on AuthOk it persists the key and — only on an actual identity
  CHANGE — reopens both durable stores (journal REBASE: tail dropped, consumer watermarks jump to
  the new head). E1 holds in the REAL graph (tst_app_service_graph). Mid-run identity-switch lens
  re-priming is A6's (rare path; tabs are per-conversation and per-identity).
- **Render bindings, both surfaces**: GUI ChatPage.qml + TUI (ChatPendingStripView +
  root_widget wiring) bind the pending strip beside the timeline, route the composer through
  `ConvSendController::send()`+`drain()` (durable enqueue → explicit user tap; §6.8 auto-replay
  asserted OFF), read the timeline from `ChatWindowModel` via the shared controller (visibility
  declaration → ingestor top-up, §5.8), and expose load-earlier (GUI button / TUI Ctrl+PgUp,
  end-of-history surfaced per §4.6 v38 semantics).
- **Mock mode at M2**: `Mirror`/`Outbox` are null in mock — the chat surfaces fall back to the
  legacy IChatService path unchanged (§9 "mock keeps working"). A8's seeder feeds the mirror in
  mock and deletes the fallback (the ledgered `SeededFetchExecutor` idea is superseded by this
  null-fallback: strictly less mock-only machinery below the seam).
- **Canonical-shape degradation (documented)**: the entity-map ChatMessage carries
  author/text/error/edited — the legacy system/notice/action styling flags are not entity fields,
  so mirror-mode markdown renders them as plain lines. Extending the map is a G-series
  entity-codegen change (wire fields exist; needs `just update-codec`), not an A5 hand-edit.

## Seams left for siblings

- A6 (M3 routing): routing/`IDaemonNet` deletion + sidebar legacy path — untouched here. Plus:
  lens re-prime on mid-run identity switch (journal rebase publishes the adopted snapshot; live
  TableModel instances re-derive rows only on their next delta).
- A8 (M5 seeder): mock-mode mirror feeding (`mirror::Seeder` through the same apply pipeline) +
  deletion of the legacy IChatService fallback in the chat surfaces.
- BR: `wire_delta` + auto-replay flip on api/39 — the gate stays OFF here and is asserted OFF;
  `before_cursor` backward windows replace the v38 "cold-fill-only" requestOlder semantics.
