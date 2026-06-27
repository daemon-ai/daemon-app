# core/daemon — the daemon-backed seam

This is where the thin client meets the node. In `ServiceMode::Daemon`
(`app_service_graph.cpp`) the UI service seams are backed by the real daemon over
a Unix socket; in `ServiceMode::Mock` they fall back to the in-memory fixtures
(kept for offline dev, unit tests, and offscreen-render coverage). The shipped
bundle defaults to `daemon` (set in the superproject `flake.nix` wrapper); bare
dev builds and the test harnesses default to `mock` unless they opt in.

## The adapter recipe

Every daemon-backed seam follows the same three-layer shape. Add a new one by
mirroring an existing example rather than inventing a new pattern:

1. **Codec facade** — `node_api_codec.{h,cpp}`. Add `encode<Op>Request(...)` and
   `decode<Resp>(...)` that wrap the generated zcbor functions (never hand-roll
   CBOR). Decoders produce plain `Decoded*` structs / typed rows. The generated
   code is drift-checked by the superproject (`just codec-drift`); the encoder is
   compiled with `ZCBOR_CANONICAL` so maps are definite-length (the daemon's serde
   rejects indefinite-length maps for enum variants).

2. **Repository** — `repositories.{h,cpp}`. A `RepositoryBase` subclass that owns
   no UI. It `sendRequest(encode..., "repo/<op>")` on the shared `NodeApiClient`
   and filters `responseReady`/`failed` by its correlation id (the client is
   single-in-flight; all repositories share one broadcast bus). On a decoded
   response it updates its in-memory rows (or writes `DaemonCacheStore` for the
   durable session slice) and emits a `*Refreshed` / `operationFailed` signal.

3. **Service adapter** — a `Daemon*` class implementing the UI interface
   (`I*Service` / `I*Store`), living **here** (not in the seam's own lib) to avoid
   a library cycle, since it depends on the repository. It projects the
   repository's rows into the `VariantListModel` / typed shape the GUI/TUI bind,
   and routes UI actions back to repository calls. It is swapped in for the mock
   in the `ServiceMode::Daemon` branch of `createAppServiceGraph`, and refreshed
   on `IConnectionService::ready()`.

## Landed examples

| Surface | Codec | Repository | Service adapter |
|---------|-------|------------|-----------------|
| Sessions (list) | `decodeSessionPage` | `SessionRepository` -> `DaemonCacheStore` | `CachedSessionStore` (`ISessionStore`) |
| Credentials / accounts | `*Credential*` | `CredentialRepository` | `DaemonAccountsService` |
| Models | `*Models*` / `ModelCurrent` | `ModelRepository` | `DaemonModelCatalog` |
| Profiles | `*Profile*` | `ProfileRepository` | `DaemonProfileStore` |
| Chat turn | `encodeSubmit*` / `decodeLogPageEntries` | (engine-owned) | `DaemonTurnEngine` (`ITurnEngine`, in `DaemonApp/Turn`) |

## Still mock (no daemon adapter yet)

`fs` (dev local disk), `daemonConfig`, `memory`, fleet
(`roster`/`fleetTree`/`approvals`/`dashboard`), `routing`/`cron`,
`transports`/`presence`, `daemonNet`, `sessionSettings`, `checkpoints`. Each has a
full wire surface in the contract already; wiring one is a matter of following the
recipe above. Tool/usage HUD (CHA-3), inline approvals/clarify (CHA-4/5), and
interrupt/steer (CHA-6) extend `DaemonTurnEngine` (the codec already decodes those
AgentEvent / HostRequest arms).
