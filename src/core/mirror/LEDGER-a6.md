# A6 — M3 routing vertical (routing onto the mirror; IDaemonNet seam deleted): TDD ledger

Package A6 of the Mirror Architecture program (spec 09 §13 wave M3; §5, §8, §14; 07§9.1/§9.2).
Base: `integrations/app` @ `080d9cf` (A1 substrate + A2 outbox + A3 lenses + A4 ingestor + A5 M2
vertical + BR api/39 flips merged). Worktree branch `mirror/a6`.

## Scope (owned by A6)

1. Give routing a mirror read path: the `DaemonFetchExecutor` fulfils the ingestor's already-queued
   `FetchOp::RoutingListChats` / `FetchOp::TransportRooms` jobs — encode → decode-to-mirror
   (`map_route_pin`/`map_room`) → `ingestor.deliverRoutePins/deliverRooms` → the store's
   `route_pins` / `rooms` tables. The `routing`/`rooms` collections join the reconnect scan +
   visibility path (§5.6/§5.8). Single writer preserved (§5.1): only the ingestor writes.
2. Port the routing manager + pin dialog + TUI routing hub onto store projections: the shared
   `RoutingManagerController` reads the mirror `Store` (`route_pins`/`rooms`/`transport_accounts`/
   `sessions`) instead of `IDaemonNet`, re-derives on `Store::committed`, declares visibility, and
   drives mutations through the node-authoritative routing seam (RoutingBindChat/Unbind), never a
   local re-derivation. GUI `RoutingPage`/`RouteDialog` + the TUI routing hub read the SAME VM.
3. Delete the legacy `IDaemonNet` seam family entirely — `IDaemonNet`, `MockDaemonNet`,
   `DaemonDaemonNet`, `transportsTree()`, and the legacy sidebar integrations path — with a
   zero-reference gate over `src/` + `tests/`.
4. Keep sessions/fleet surfaces (A7's M4 wave) working through their EXISTING non-IDaemonNet
   paths: the mock fleet/roster/session seed moves out of the deleted `IDaemonNet` class into a
   plain `daemonnet::MockFleetSource` (no IDaemonNet, no routing/transports/patch-bay), which
   `MockSessionRoster`/`MockFleetTree`/`InMemorySessionStore` consume unchanged in behaviour.

## Port map — old `IDaemonNet` read/mutation → new source (per consumer)

| Consumer | Old (IDaemonNet) | New |
|---|---|---|
| RoutingManagerController.routes | `net->routes()` | mirror `route_pins` table |
| RoutingManagerController.bindable | pins + `transportsTree()`/`transportRooms()` | `route_pins` + `rooms` tables |
| RoutingManagerController.accounts | `net->accountsAgents()` | mirror `transport_accounts` (bound_profile) |
| RoutingManagerController.rules | `net->bindingRules()` (empty in daemon) | empty (no wire read — unchanged) |
| RoutingManagerController.sessions | `net->seed().sessions` | mirror `sessions` table |
| RoutingManagerController.delivery | `net->deliveryTargets()` | SessionRepository SessionGet (A7 seam; daemon-only) |
| resolve()/explain() | `net->resolve()` | client-side pin-table lookup (Pin / else Default) |
| bindChat/unbind/handover/bindAccount | `net->*` | RoutingRepository wire ops + mirror re-fetch |
| SidebarModel integrations section | `net->transportsTree()` | DELETED (IntegrationsTreeModel is the only tree) |
| TUI routing hub | `deps.daemonNet` | shared RoutingManagerController (mirror-backed) |
| RoutingTopology.qml (patch-bay) | `DaemonNet.nodes()/edges()` | DELETED (mock-only demo graph) |
| mock fleet/roster/sessions | MockDaemonNet seed | `daemonnet::MockFleetSource` (non-IDaemonNet) |

## Invariants under test (spec refs)

- §5.4 routing fetch: a queued `RoutingListChats`/`TransportRooms` job reaches the executor,
  decodes to `RoutePin`/`Room`, and lands as journal deltas in the store's `route_pins`/`rooms`
  tables (page-looped, replace-and-prune over the accumulated union).
- §5.1 single writer: the ingestor is the only writer of `route_pins`/`rooms`; a mutation is a wire
  intent whose node-acked result re-fetches into the mirror — never a client-side row write.
- §8 projection: `RoutingManagerController` re-derives its rows on `Store::committed` (not a lens
  callsite poll); resolve() answers Pin (pinned origin) else Default — the node owns the lower
  precedence rungs (no client re-derivation).
- §14 no split brain: reads come from the ONE store; the legacy `RoutingRepository` in-memory cache
  is no longer a read source (residual: the repo still round-trips the mutation wire op; its cache
  is dead — flagged for a follow-up, not a new read path).
- zero-reference gate: `rg -n "IDaemonNet|MockDaemonNet|DaemonDaemonNet|transportsTree" src tests`
  returns only historical prose in LEDGER/docs — zero code references.

## Degradations (documented, honest thin-client)

- Mock mode: `mirrorService` is null at M2 (A5 precedent), so the routing manager renders EMPTY in
  mock (like chat). A8's seeder feeds the mirror in mock. The routing precedence fiction
  (rule/account-bound demo profiles) that `MockDaemonNet::resolve()` computed is NOT reproduced —
  the node owns precedence; the client shows Pin/Default only.
- `accounts` (account→agent baselines) read the mirror `transport_accounts.bound_profile`; that
  table is fed by TransportChanged patches today, so the panel is populated only as transports land
  in the mirror (transports collection is A7's). Delivery targets read SessionRepository's
  SessionGet cache (daemon-only), unchanged from the DaemonDaemonNet behaviour.
- The RoutingTopology patch-bay (mock-only `nodes()/edges()` demo graph) is removed with the seam;
  it had no daemon-mode data and no TUI counterpart.

## Seams left for siblings

- A7 (M4 sessions/fleet): `MockFleetSource` + `MockSessionRoster`/`MockFleetTree`/
  `InMemorySessionStore` stay on the pre-mirror mock/daemon paths; the sessions/fleet/transports
  collections are not yet the mirror read path. Delivery targets stay on SessionRepository.
- A8 (M5 seeder): mock-mode mirror feeding — restores the routing manager (and chat) in mock.
- Residual debt: the `RoutingRepository` in-memory cache (07 RoutingRepository cache) is no longer a
  read source but still exists as the mutation wire-op sender; retire it once mutations move to the
  outbox lane.
