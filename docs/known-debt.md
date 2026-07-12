# Known debt — the mirror architecture program

Status: **living register.** This is the tracked, ongoing debt register of the client mirror
architecture program (spec 09 §13–§15; the `mirror::` data layer described in
`docs/client-sync-architecture.md`). It supersedes the per-package `LEDGER-*.md` files that recorded
the program's build history and the two `HARDENING-PLAN-app-*.md` plans — those have been deleted
now that their content shipped; git history is their archive. Everything worth keeping (the F6
success-metric audit and the honest leftovers) lives here and is updated as the leftovers close.

Each debt item carries a deletion/upgrade condition. None blocks the program's completion; all are
post-program. Post-program cleanup (the roster-edit outbox lane, the redundant feed arms, the dead
transport read-row builder) has since closed D1's two flagged arms, D3 for `roster-edit`, and D4 —
recorded inline below with the residuals those deletions exposed.

---

## F6 audit — end state vs the 07 baselines

The 09§13-M6 gate is "audit shows net deletion (07§1 numbers vs end state)". **07§1's numbers are
STRUCTURAL counts** (seams / repositories / row-shape forks / cache tables / context properties) —
that is the success metric, and every one moved the right way. Raw whole-program LOC did NOT shrink
(see "LOC" below) — the mirror is a substantial NEW substrate — and that is stated plainly here
rather than hidden: F6 is net deletion of the *fracture*, realized in the deletion waves and the
row-shape collapse, not a smaller binary.

The headline table below is the audit as recorded at program completion (`1526fba`, the AD merge);
baselines are `07-client-inventory.md` §1–§6. The counts are reproducible from a daemon-app checkout
with the commands given. A **post-cleanup delta** note after the table records what the loose-ends
cleanup changed relative to that end state.

### Headline table (end state `1526fba`)

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

### Post-cleanup delta (`fa1da60`, after the loose-ends cleanup)

- **Seam surfaces:** the interface *count* is unchanged (still 25 / 29 — the surviving
  `ITransportRegistry` verb seam remains), but its dead read surface was removed at the method level:
  `ITransportRegistry::conversations()` / `refreshConversations()` / `conversationsChanged` and
  `DaemonTransportRegistry::conversations()` + `TransportRepository::cachedConversations()` are
  deleted (D4). This is a method-level surface reduction inside a surviving interface, so it does not
  move the interface tally.
- **Feed arms:** the `SubscriptionManager` lost its `MessagesChanged` and `ContactsChanged` arms and
  `ChatRepository::applyMessagesChanged` was deleted (D1) — no metric-table row, but a real deletion.
- **QML context properties:** now **43** (was 41 at `1526fba`). The `+2` is from the accessibility
  package landed after the program (AT-SPI audit surfaces), NOT from this cleanup — recorded so the
  refreshed `setContextProperty` count reconciles.

### Reproducible commands

Run from a daemon-app checkout root:

```sh
# Seam interfaces: exclude the same class of non-seams 07 excluded (domain/ids.h,
# node_api_codec/internal.h, integration_slice.h) plus the two non-seam i*.h
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

# QML context properties (43 post-a11y; was 41 at program completion):
rg -c 'setContextProperty' src/DaemonApp/App/application.cpp

# Post-cleanup deletions (expect empty — D1/D4 landed):
rg -n 'applyMessagesChanged|cachedConversations' src tests
rg -n 'conversations\(\)' src/core/daemon/daemon_transport_registry.h

# Data mocks deleted (expect empty), i.e. the 07§9.10 census:
git ls-files '*mock_daemonnet*' '*daemon_daemonnet*' '*cached_session_store*' \
  '*in_memory_session_store*' '*mock_fleet_tree*' '*mock_fleet_source*' '*mock_session_roster*' \
  '*mock_transport_registry*' '*mock_presence_service*' '*mock_persons_service*' \
  '*mock_contacts_service*' '*mock_chat_service*'
```

### Row-shape collapse — how 5+6+3 became 1+1+1

The mirror `Conversation`, `Session`, `Contact`/`Person`/`PersonEndpoint` generated entities are now
the single client shape; role maps read them (§8.3). The forking SITES 07§6 enumerated are deleted:
`IDaemonNet::channels` projection + `TransportTreeRow` (M3), the mock roster shapes + `MockSessionRoster`
(A8), the routing-manager pickers re-read the mirror. What remains and is NOT a fork:

- **Role maps** — each VM's `roleNames()`/`data()` reading the one typed entity. Allowed by design
  (§8.3); they cannot diverge without failing the drift gate.
- **Codec DTOs** — `CachedSessionRow` (the codec's SessionInfo decode target), `CachedTranscriptBlockRow`
  (the engine's coalescing value + sink input). Transient decode/coalesce inputs, not models.

### LOC (stated honestly)

Whole program, `git diff --shortstat 6275fe3 1526fba` (6275fe3 = 07's HEAD, the program's start):
**318 files, +33,570 / −12,683 (net +20,887); 45 files deleted, 139 added.** The +insertions are the
new substrate (immer value tree, journal, ingestor, outbox, generated entities incl. the ~47 KB
`entities_map.cpp`), the parity/scenario test suites, and the ledgers/docs. The **−12,683 removed
lines / 45 deleted files** are the realized legacy-deletion power; the deletion waves (AD alone
−6,760 net across its two merges) are where F6 shows as raw subtraction. **Conclusion: F6 passes on
the structural metric it is defined against (07§1); it is NOT a whole-repo LOC reduction, by design.**
(The post-program cleanup adds a further small net deletion — the D1 arms, D4 read-row builder, and
the shipped `LEDGER-*`/`HARDENING-PLAN-*` files — not re-tallied here.)

---

## Shipped since the audit — the two hardening plans landed

The two `HARDENING-PLAN-app-*.md` files (render-honesty, session-meta) are deleted because their
content shipped and is verified in the code: the **session-meta lane** (pin/archive/title routed as
`SessionUpdateMeta` intents on a durable outbox lane in `MirrorSessionStore`), the **safety-section
honesty rewrite**, **tool-ok fabrication removal** (the app never fabricates a tool-ok / approval
offer), and **allow-permanent plumbing end-to-end** (the operator's "allow permanently" choice rides
the wire's `allow_permanent` / host-offered `allow_permanent_offered`, surfaced only when the node
actually offered it). These are no longer plans — they are the shipped behavior.

---

## Known-debt register

### D1 — SubscriptionManager: two redundant arms deleted; the slimmed manager survives — RESOLVED (post-program cleanup)

09§13-M1 said "SubscriptionManager absorbed into `mirror::Ingestor`". In the end state a **slimmed
`SubscriptionManager` survives in parallel** with the ingestor: both tap the node-wide `EventsSince`
stream with independent cursors (the code calls it "dual-write … neither owns the other's cursor").

The two arms flagged as redundant with the ingestor's policy table were verified dead and deleted:

- **`MessagesChanged`** — the ingestor pages `ConvHistory` into the mirror `chat` window;
  `ChatConversationController` reads the mirror `ChatWindowModel`. The legacy `IChatService` rows the
  arm fed were a bare-test fallback with no production reader, so the arm and
  `ChatRepository::applyMessagesChanged` (consumer-free) were deleted.
- **`ContactsChanged`** — the ingestor pages `RosterList` into the mirror `contacts`; `ChannelsHub`
  is the sole reader of contact rows. The manager's `refreshContacts` arm was redundant and deleted.

The slimmed manager survives routing only the genuinely non-migrated domains: `ApprovalPending`
(L-class inbox badge), `DownloadProgress` / `CatalogChanged` (models), `SessionAdvanced` (nudge),
`ResyncNeeded`, `ProfilesChanged`, and the transport `Conversations` / `Membership` /
`TransportChanged` refetch arms.

- **Remaining condition:** full removal of `SubscriptionManager` awaits porting approvals / models /
  downloads to the ingestor policy table (a post-program vertical) — the domains those surviving arms
  serve were never given mirror verticals, so their event routing has nowhere to be absorbed *to*.
  Files: `subscription_manager.{h,cpp}`, `tst_subscription_manager.cpp`.

### D2 — TranscriptBlock richness (LARGELY RESOLVED in G2; residual is product-scope)

A7T withheld the transcript `content()` flip because the generated `mirror::TranscriptBlock` lacked
`args_summary`/`detail_kind`/`detail_body` (S7/S8 diverged). **G2 closed it** (map-only enrichment,
no CDDL/version bump — the wire already carried the fields): `content()` = `mirrorContent()`, S7/S8
byte-parity green, the `TranscriptBlock` delta arm fires `changed()`.

- **Residual debt (product-scope):** the block grammar is a fixed coalesced set
  (Message/Reasoning/ToolCall/ToolResult) plus those three enriched fields. Any transcript richness
  beyond that (e.g. richer structured/rich-demo content decomposed into these kinds) is flattened
  lossily. Closing it is a further post-M5 entity enrichment (add fields to `entity-map.toml` +
  regen), i.e. product work when a richer transcript surface is wanted — not a program gap.

### D3 — Outbox lanes: `roster-edit` wired (CLOSED); `conv-meta` guardrail-only

Wired lanes (verified via `enqueue(` callsites): `turn-prompt` (composer), `chat-send` (ConvSend),
`session-meta` (SessionUpdateMeta), and now **`roster-edit`**.

- **`roster-edit` — CLOSED (post-program cleanup).** The four roster verbs
  (`RosterAdd` / `RosterUpdate` / `RosterRemove` / `ContactSetAlias`) now enqueue on the per-transport
  `roster-edit` durable outbox lane in `DaemonContactsService`, following the `session-meta`
  precedent: no direct-send fallback, §6.5 rejection surfaced via `contactOperationFailed`, op-id on
  the wire's optional `op_id`. Covered by `tests/unit/tst_roster_edit_lane.cpp`.
  - **Caveat recorded:** the contacts read path carries **no `origin_op` provenance** — there is no
    provenance field on the `Contact` entity and no `origin_op` arm on `ContactsChanged`. So an acked
    roster op rests in `accepted` and is disposed by the §6.6 grace sweep rather than by a provenance
    echo; rows still refresh promptly via the ingestor's `ContactsChanged` → `RosterList` arm. Full
    provenance landing needs a node-side CDDL addition (future wire work).
- **`conv-meta` — guardrail only.** The `ConvSetTopic`/`SetTitle`/`SetDescription` verbs exist in
  CDDL but there is **no app UI** yet. **Guardrail (not a task):** when the topic/title/description
  edit UI lands, it MUST enqueue on the `conv-meta` lane, not add a direct verb (AGENTS.md law 8).

### D4 — Dead transport read-row builder deleted (CLOSED); a write-only residual remains

`DaemonTransportRegistry::conversations()`, the `ITransportRegistry` conversations read surface
(`conversations()` / `refreshConversations()` / `conversationsChanged`), and
`TransportRepository::cachedConversations()` had zero read consumers (surfaces read `ChannelsHub` off
the mirror) and were **deleted (post-program cleanup)**.

- **D7 — residual (write-only-dead).** `TransportRepository::refreshConversations` + its
  `daemon_conversations` cache write + the `conversationsRefreshed` signal survive as write-only-dead:
  nothing reads what they persist, but they are still *invoked* by the surviving manager
  `Conversations` / `Membership` / `TransportChanged` arms and the auth-succeeded handler. Unwinding
  them ties to retiring those manager arms — i.e. it is part of the **D1 completion condition**, and
  should be pruned in the same batch that removes `SubscriptionManager`.

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

### D7 — Write-only-dead transport conversations plumbing

See D4 (recorded there as the residual the D4 deletion exposed): `TransportRepository::refreshConversations`
+ `daemon_conversations` cache write + `conversationsRefreshed`. Prune together with the
`SubscriptionManager` arms (D1 completion).

### Residual from the roster-edit lane (candidate prune)

`ContactsRepository`'s mutation methods (`addContact` / `updateContact` / `removeContact` / `setAlias`
+ the `repo/roster-*` correlation handlers) are now **production-unused** — the service enqueues on
the `roster-edit` lane and the graph dispatcher encodes the request directly — but they remain
exercised as wire-seam utilities by the unit tests. They are a candidate for a later small-batch
prune, the same class of write-only/test-only seam residual as the D4/D7 transport plumbing.

---

## ADR-011 — KEEP `mirror::` (decided, no rename)

The ADR is closed as decided: **`mirror::` stays.** The rename's only rationale (clean `git grep`
partitioning of old vs new during the migration) is already satisfied now that the legacy symbols are
gone; `mirror::` is honest and accurate (the architecture literally maintains a client-side mirror of
the node's authoritative state); and a rename would be the single largest no-value churn in the
program (`mirror::` appears ~968 times across ~109 files). If a future *product* rename is ever
wanted, it is a mechanical, gated one-shot (rename the namespace + `src/core/mirror/` dir + the table
prefixes, regenerate, run `just lint` + full ctest), recorded here as a known, costed follow-up
rather than a re-litigation.
