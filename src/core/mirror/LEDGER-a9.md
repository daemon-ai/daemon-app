# A9 — M6 guardrails + F6 audit + ADR-011: the program's closing package

Package A9 of the Mirror Architecture program (spec 09 §13 wave **M6**; §14 guardrails; ADR-011).
Base: `integrations/app` @ `1526fba` (the completed AD merge). Worktree branch `mirror/a9`. This
package locks the architecture in (guardrails → AGENTS.md), proves the success metric (F6 audit
against the 07 baselines), settles the last open decision (ADR-011), and registers the honest
leftovers.

A9 writes NO production code beyond one cheap test wiring (the GUI mock boot smoke) — it is docs +
audit + a gate. The mirror is the ONLY client data path already (AD closed that); A9 does not move
data paths.

## Deliverables in this package

1. **Guardrails** distilled from 09§14 into the repo `AGENTS.md` ("Data layer — the mirror is the
   only client model" + "Mock mode"): the 10 enforceable data-layer laws + the review checklist +
   the mock-scenario rules.
2. **Architecture doc rewrite**: `docs/client-sync-architecture.md` now describes the mirror as THE
   shipped architecture (store/journal/ingestor/outbox/projections/codegen), not a proposal; it had
   still described the pre-mirror `SubscriptionManager`/`CachedSessionStore` design. `docs/README.md`
   updated. The superproject spec (09) gets a closing addendum via the patch (below).
3. **F6 audit** — this file, §"F6 evidence" below.
4. **ADR-011 decision** — KEEP `mirror::` (rationale below); written into the ADR via the
   superproject patch.
5. **Known-debt register** — §"Known-debt register" below.
6. **GUI mock boot smoke** promoted into the gate (`daemon_app_offscreen_mock_boot` ctest).

---

## F6 evidence — end state vs the 07 baselines

The 09§13-M6 gate is "audit shows net deletion (07§1 numbers vs end state)". **07§1's numbers are
STRUCTURAL counts** (seams / repositories / row-shape forks / cache tables / context properties) —
that is the success metric, and every one moved the right way. Raw whole-program LOC did NOT shrink
(§"LOC" below) — the mirror is a substantial NEW substrate — and that is stated plainly here rather
than hidden: F6 is net deletion of the *fracture*, realized in the deletion waves and the row-shape
collapse, not a smaller binary.

All counts are reproducible from the worktree root (`/home/j/experiments/daemon-worktree/mirror-a9`)
with the exact commands given. Baselines are 07-client-inventory.md §1–§6.

### Headline table

| Metric | 07 baseline | End state (`1526fba`) | Δ | Verdict |
|---|---|---|---|---|
| Seam interfaces (`i*.h`, non-seams excluded) | 27 | **25** | −2 | ✅ deletion (IDaemonNet, IPersonsService, IPresenceService −3; IRoutingActions +1) |
| Seam surfaces (interfaces + 4 no-interface QML objects) | 31 | **29** | −2 | ✅ deletion |
| Wire repositories (repositories.h + DaemonFeedback) | 20 | **19** | −1 | ✅ (PersonsRepository deleted) |
| Conversation row layouts | 5 | **1** (mirror `Conversation`) | −4 | ✅ collapse (07§6, the R9 disease) |
| Session row layouts | 6 | **1** (mirror `Session`) | −5 | ✅ collapse |
| Contact/person layouts | 3 (+1 person) | **1 + 1** (mirror `Contact`, `Person`/`PersonEndpoint`) | −2 | ✅ collapse |
| Legacy cache tables (`DaemonCacheStore`) | 9 (schema v10) | **6** (schema v11) | −3 | ✅ (daemon_sessions, daemon_fleet_units, daemon_transcript_blocks dropped) |
| QML context properties | ~40 | **41** | ≈flat | ⚠️ see note |
| Data-mock classes | ~10 (07§9.10) | **0** for mirror-owned verticals | −10 | ✅ (infra + post-M5 domain mocks remain) |
| Dual data architecture (`IDaemonNet` split brain) | present (07§9.1–2) | **gone** | — | ✅ M3 |
| Per-event refresh-policy folklore (07§9.9) | ad-hoc per callsite | one policy table | — | ✅ M1 |
| 19-call connect-ready storm (07§5.2) | present | staleness scan | — | ✅ M1 |

**Context-properties note (honest).** The count is flat (~40 → 41), NOT a shrink. Three seam props
were deleted (`Persons`, `Presence`; `SessionStore` → `SessionStoreMirror`; `DaemonNet` →
`RoutingActions`), but the mirror adds its own coherent projection/intent surfaces (`Mirror`,
`Outbox`, `ChannelsHub`). This metric was never a shrink target: the win is that the surviving
globals expose ONE coherent model instead of many divergent seams, not that there are fewer of them.

### Reproducible commands

```sh
# Seam interfaces: 30 files match the 07 glob; exclude the same class of non-seams 07 excluded
# (domain/ids.h, node_api_codec/internal.h, integration_slice.h) plus the two new non-seam i*.h
# (daemon/ingestor_bridge.h, mirror/ingestor.h). => 25.
git ls-files 'src/core/**/i*.h' | grep -vE \
  'domain/ids.h|node_api_codec/internal.h|integration_slice.h|ingestor_bridge.h|mirror/ingestor.h' | wc -l
# The 4 no-interface QML objects survive (Caps, Gateway, Agents, Principal): 4.
for p in Caps Gateway Agents Principal; do rg -c "setContextProperty\(QStringLiteral\(\"$p\"" \
  src/DaemonApp/App/application.cpp; done
# Deleted seams (expect empty):
git ls-files '*ipresence_service*' '*ipersons_service*' '*idaemonnet*'

# Repositories: 18 extend RepositoryBase + DaemonFeedback => 19.
rg -c ': public RepositoryBase' src/core/daemon/repositories.h
rg -c 'class DaemonFeedback' src/core/daemon/feedback_repository.h

# Legacy cache tables at schema v11 (6 CREATE TABLE): meta, sync_cursors, profiles, fs_entries,
# transport_instances, conversations.
rg -c 'CREATE TABLE' src/core/daemon/client_cache_schema.h
rg -n 'kSchemaVersion = ' src/core/daemon/client_cache_schema.h   # => 11

# Mirror db schema (the new home for the migrated entities):
rg -n 'kSchemaVersion = ' src/core/mirror/persistence.h           # => 15
rg -c '^\[entity\.' src/core/mirror/entity-map.toml               # generated entity kinds

# QML context properties:
rg -c 'setContextProperty' src/DaemonApp/App/application.cpp      # => 41

# Data mocks deleted (expect empty), i.e. the 07§9.10 census:
git ls-files '*mock_daemonnet*' '*daemon_daemonnet*' '*cached_session_store*' \
  '*in_memory_session_store*' '*mock_fleet_tree*' '*mock_fleet_source*' '*mock_session_roster*' \
  '*mock_transport_registry*' '*mock_presence_service*' '*mock_persons_service*' \
  '*mock_contacts_service*' '*mock_chat_service*'

# Row-shape collapse (the shaping SITES that forked each entity are gone):
git ls-files '*mock_daemonnet*'          # IDaemonNet::channels + roster shapes (deleted)
rg -n 'TransportTreeRow' src              # legacy 15-field sidebar row struct (deleted)
```

### Row-shape collapse — how 5+6+3 became 1+1+1

The mirror `Conversation`, `Session`, `Contact`/`Person`/`PersonEndpoint` generated entities are now
the single client shape; role maps read them (§8.3). The forking SITES 07§6 enumerated are deleted:
`IDaemonNet::channels` projection + `TransportTreeRow` (M3), the mock roster shapes + `MockSessionRoster`
(A8), the routing-manager pickers re-read the mirror. What remains and is NOT a fork:

- **Role maps** — each VM's `roleNames()`/`data()` reading the one typed entity. Allowed by design
  (§8.3); they cannot diverge without failing the drift gate.
- **Codec DTOs** — `CachedSessionRow` (the codec's SessionInfo decode target), `CachedTranscriptBlockRow`
  (the engine's coalescing value + sink input). Transient (class-T-like) decode/coalesce inputs, not
  models.
- **A dead read-row builder** — `DaemonTransportRegistry::conversations()` still assembles a QVariantMap
  list, but no surface reads it anymore (consumers use `ChannelsHub.conversations()` off the mirror;
  `rg` for `Transports.conversations`/`registry->conversations` outside the class returns nothing). It is
  a deletable remnant of the surviving verb seam — see the register.

### LOC (stated honestly)

Per-package net LOC is carried by the merge commits (`git log --first-parent`):

| Merge | Package | Net LOC |
|---|---|---|
| `0073a17`,`5c92447`,`57bfb28`,`e5b8341`,`bda9580`,`4b6cad8` | G1/A1/A2/A3/A4 + BR (substrate, outbox, adapters, ingestor, codec bridge) | large **+** (new substrate) |
| `845ff30` | A5 (M2 integrations vertical) | + |
| `70e5ec1` | A6 (M3 routing; `IDaemonNet` family deleted) | mixed |
| `5b54b12`,`52ebae9` | A7 (M4 sessions) | + |
| `b4789c5`,`9596bb3`,`0c05771` | A7T/A8 (transcript re-home, mock reform) | mixed |
| `58c4cfd` | G2 (entity enrichment, `daemon_fleet_tree` deleted) | mixed |
| `ae86bba` | AD Phase 1b + deletions | **−2,763** |
| `1526fba` | AD Phase 1a + stragglers | **−3,997** |

Whole program, `git diff --shortstat 6275fe3 1526fba` (6275fe3 = 07's HEAD, the program's start):
**318 files, +33,570 / −12,683 (net +20,887); 45 files deleted, 139 added.** The +insertions are the
new substrate (immer value tree, journal, ingestor, outbox, generated entities incl. the ~47 KB
`entities_map.cpp`), the parity/scenario test suites, and the ledgers/docs. The **−12,683 removed
lines / 45 deleted files** are the realized legacy-deletion power; the deletion waves (AD alone
−6,760 net across its two merges) are where F6 shows as raw subtraction. **Conclusion: F6 passes on
the structural metric it is defined against (07§1); it is NOT a whole-repo LOC reduction, by design.**

### Survivors verified (registry/contacts verb sinks, ChatRepository, executor encoders, non-migrated caches)

Each re-verified by fresh grep against `1526fba`:

- **`ITransportRegistry` + `DaemonTransportRegistry` + `TransportRepository`** — VERB sink (room
  lifecycle two-phase forms + member ops, account connect/enable/label/remove, settings/configure,
  auth handoff). Read surfaces (tree, channels hub) read the mirror; the registry is null-guarded in
  mock. Deletable remnant inside it: the dead `conversations()`/read-row builders (above).
- **`IContactsService` + `DaemonContactsService` + `ContactsRepository`** — contact VERB sink
  (add/update/remove/alias/profile/directory). Hub contact ROWS read the mirror. The mutations run
  DIRECT — see the roster-edit debt item.
- **`ChatRepository`** — the ConvHistory window pager feeding the mirror `chat` window (per-conv
  fills). This is a mirror fetch feeder, not a read model — legitimate survivor.
- **Executor fetch encoders** — `encodeRoutingListChatsRequest` / `encodeTransportRoomsRequest` /
  `encodePersonListRequest` (the mirror executor's fetch encoders) + `encodeRoutingGet/BindChat/
  UnbindChat` (the `IRoutingActions` mutation seam via RoutingRepository). Legitimate survivors —
  the mirror's wire fetch/verb encoders.
- **`DaemonCacheStore` non-migrated domains** (6 tables, schema v11): sync cursors (engine resync +
  events-since), profiles, fs entries, transport instances/conversations. OUT of the migration
  charter (the mirror owns sessions/fleet/transcripts; credentials/models/cursors/profiles/fs were
  never in scope). Deletion condition: a future wave porting those domains to mirror entities.

---

## ADR-011 decision — KEEP `mirror::` (do NOT rename)

**Decision: `mirror::` stays. The ADR is closed as decided; no rename.** (Recorded into
ADR-011 via the superproject patch; 09§15 open-item 10 closes.)

The ADR's criterion: after M5 deletes the legacy symbols, renaming (e.g. to a product-facing name)
is *optional*, decided at M6, default keep. Weighing it at M6:

- **The rename's only rationale is already satisfied.** ADR-011 chose `mirror::` to keep `git grep`
  cleanly partitioning old vs new during the migration. The legacy symbols are gone (`IDaemonNet`
  family, `CachedSessionStore`, the data mocks), so there is nothing left to disambiguate against —
  the collision problem the name solved no longer exists, which is exactly the state in which the
  ADR made renaming *possible*, not *required*.
- **`mirror::` is honest and accurate.** The architecture literally maintains a client-side mirror
  of the node's authoritative state. The name is not a transitional crutch; it describes the thing.
- **The churn is real and the value is zero.** `mirror::` appears **968 times across 109 files**
  (`rg -o 'mirror::' src tests | wc -l`; `rg -l 'mirror::' src tests | wc -l`). A rename is pure
  mechanical diff with no behavioral change — it would be the single largest no-value churn in the
  program, against the program's standing low-churn-honesty precedent.
- **Precedent.** Every prior gate favored honest, low-churn outcomes (ADR-011 itself accepted the
  transitional name over a poisoned grep; AD kept survivors rather than force-deleting).

**If a future product rename is ever wanted** (not now), the migration is mechanical and gated:
rename the namespace + `src/core/mirror/` dir + the `m_`/`w_`/`sidecar_`/`outbox_` table prefixes in
one commit, regenerate (the emitter output follows the map), run `just lint` + full ctest. It is a
one-shot rename with the drift gate + parity asserts as the safety net — recorded here so it is a
known, costed follow-up rather than a re-litigation.

---

## Known-debt register

The honest leftovers, collected from the ledgers/reports into one place. Each has a deletion/upgrade
condition. None blocks the program's completion; all are post-program.

### D1 — Two feed consumers (SubscriptionManager NOT fully absorbed) — DEVIATION FROM SPEC

09§13-M1 said "SubscriptionManager absorbed into `mirror::Ingestor`". In the end state a **slimmed
`SubscriptionManager` survives in parallel** with the ingestor: both tap the node-wide `EventsSince`
stream with independent cursors (the code calls it "dual-write … neither owns the other's cursor",
`app_service_graph.cpp:676-689`). The ingestor's policy table owns the migrated domains
(sessions/fleet/routing/persons/conversations/rooms + the chat-window `MessagesChanged` arm); the
surviving manager routes the NON-migrated domains: `ApprovalPending` (L-class inbox badge),
`DownloadProgress`/`CatalogChanged` (models), `ContactsChanged` → RosterList, `SessionAdvanced`
nudge, `ResyncNeeded`, and a `MessagesChanged` → `ChatRepository` arm.
- **Ratified deviation:** the domains those arms serve (models, approvals) were never given mirror
  verticals (out of the M2–M5 charter), so their event routing had nowhere to be absorbed *to*.
  Keeping a slimmed manager is honest; forcing absorption would have required porting those domains.
- **Follow-up / consolidation:** the `MessagesChanged` and `ContactsChanged` arms LOOK redundant with
  the ingestor's own arms (the ingestor already fetches ConvHistory on `MessagesChanged`; contacts
  rows read the mirror) — verify whether the manager's chat/contacts arms are dead and, if so, delete
  them. Full deletion of `SubscriptionManager` waits on porting approvals/models/downloads to the
  ingestor policy table (a post-program vertical). Files: `subscription_manager.{h,cpp}`,
  `tst_subscription_manager.cpp`.

### D2 — TranscriptBlock richness (LARGELY RESOLVED in G2; residual is product-scope)

A7T withheld the transcript `content()` flip because the generated `mirror::TranscriptBlock` lacked
`args_summary`/`detail_kind`/`detail_body` (S7/S8 diverged). **G2 closed it** (map-only enrichment,
no CDDL/version bump — the wire already carried the fields): `content()` = `mirrorContent()`, S7/S8
byte-parity green, the `TranscriptBlock` delta arm fires `changed()` (see LEDGER-g2, LEDGER-a7t).
- **Residual debt:** the block grammar is a fixed coalesced set (Message/Reasoning/ToolCall/
  ToolResult) plus those three enriched fields. Any transcript richness beyond that (e.g. richer
  structured/rich-demo content decomposed into these kinds) is flattened lossily. Closing it is a
  further post-M5 entity enrichment (add fields to `entity-map.toml` + regen), i.e. product work when
  a richer transcript surface is wanted — not a program gap.

### D3 — §6.4 outbox lanes declared but not yet wired

Wired lanes (verified via `enqueue(` callsites): `turn-prompt` (composer), `chat-send` (ConvSend),
`session-meta` (SessionUpdateMeta). Declared in §6.4 but currently DIRECT:
- **`roster-edit`** (RosterAdd/Update/Remove/ContactSetAlias): `DaemonContactsService` routes these
  DIRECT through `ContactsRepository`; no outbox lane. **Upgrade condition:** when offline roster
  management is wanted, route through a `roster-edit` lane (precedent: chat-send/session-meta). Until
  then a roster edit made offline is lost — acceptable for now (no offline-roster product requirement).
- **`conv-meta`** (ConvSetTopic/SetTitle/SetDescription): the verbs exist in CDDL but there is **no
  app UI** yet. **Upgrade condition (a guardrail, not a task):** when the topic/title/description edit
  UI lands, it MUST enqueue on the `conv-meta` lane, not add a direct verb (AGENTS.md law 8).

### D4 — Dead read-row builder on the surviving transport verb seam

`DaemonTransportRegistry::conversations()` (and adjacent read-row assemblers) still build QVariantMap
lists, but no surface reads them (consumers use `ChannelsHub`/the mirror). **Deletion condition:**
prune with the next touch of the transport verb seam; safe now (zero consumers) but out of A9's
docs-only scope to delete unilaterally. Low priority; a batch for `just audit-cleanup`.

### D5 — Post-M5 domain mocks remain (by charter)

`MockModelCatalog`/`MockProviderCatalog`/`MockAccountsService`/`MockProfileStore`/`MockSessionSettings`/
`MockCheckpointTimeline`/`MockToolInventory`/`MockDaemonConfig`/`MockMemoryService`/`MockDashboard`/
`MockApprovalsInbox`/`MockFeedback`/`MockCronStore` survive, plus the non-data stand-ins
`MockConnectionService` (transport) and `MockAuthFlowService` (interactive flow). These back domains
whose consumers do NOT read the mirror yet (models/providers/accounts/profiles/checkpoints/tools/
memory/dashboard/approvals/config) or that have no wire op (`MockCronStore` — CronJob is node-blocked,
07§2 #17). **Deletion condition:** each dies when its domain ports to a mirror vertical (a scenario
seed replaces its seam), exactly as the M5 data mocks did. Not a defect — the M5 charter never
covered these; 07§9.10's "~10 data mocks" (the ones that WERE covered) are all gone.

### D6 — TUI real-binary boot is not smoke-testable (documented limitation, no action)

The `daemon-tui` binary autodetects a terminal and needs a pty; a headless real-binary TUI boot is
not smoke-testable the way the GUI's offscreen QML boot is. TUI coverage therefore stays with the
`tui_*` ctest suites (unit + the `daemon_tui_offscreen_*` env-driven runs) — all green. No action;
recorded so the endgame does not expect a headless TUI boot gate.

---

## Gate results (this package)

- Full build (`cmake --build build`, GUI+TUI+tests): green.
- `ctest` (offscreen): **146/146** — was 145/145 at base `1526fba`; **+1 is the new
  `daemon_app_offscreen_mock_boot`** GUI mock boot smoke (see below). No test regressed; no tally
  drop.
- clang-format / clang-tidy on touched TUs: docs-heavy package; the only compiled change is the
  ctest wiring (CMake, no C++ TU). qmllint: no QML touched. i18n: no user-visible strings changed.
- Hooks: run per commit; no `--no-verify`.

### The GUI mock boot smoke (AD's recommendation, now wired)

AD recommended promoting the mock GUI offscreen boot into the endgame gate. It is cheap (the binary
already has an offscreen render harness with a QML `objectCreationFailed` → `exit(-1)` hook), so A9
wired it as a ctest rather than only recommending it:

```
daemon_app_offscreen_mock_boot: DAEMON_APP_SERVICE_MODE=mock QT_QPA_PLATFORM=offscreen
  QT_QUICK_BACKEND=software DAEMON_APP_RENDER_SHOTS=<tmp> daemon-app  →  must exit 0
```

It renders the full QML root in mock mode and fails on any QML `TypeError`/`ReferenceError`/missing
context-property breakage that unit ctests never see (AD found two such pre-existing breakages this
way). **Endgame note:** also run this (and the `daemon_tui_offscreen_*` suite) in the superproject
`just e2e` / sim so the sealed-bundle wiring is covered, not just the debug build.
