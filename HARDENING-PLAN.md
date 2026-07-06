# HARDENING-PLAN.md — Phase 5 / daemon-app "render honesty" (`hardening/app-render-honesty`)

Track: `app-render-honesty` (Phase 5, daemon-app). Worktree: `/home/j/experiments/daemon-worktrees/app-render-honesty`,
branch `hardening/app-render-honesty` off `hardening/app-v28-codec`.

Guiding invariant (daemon-app AGENTS.md): *the node decides, the app renders node state + sends intents.*
The app must never re-derive, fabricate, or imply domain/security state the node owns.

**Status: PLAN ONLY — no source touched. Awaiting review + the node-side `allowPermanent` wire addition before implementing item #3.**

Items #1 and #2 are pure app-side (no node dependency). Item #3 is **blocked** on a node-side wire addition (spec'd precisely below).

---

## Item #1 — Remove/label mock-only safety settings

### What I found
`IDaemonConfig` is **deliberately `MockOnly`** and is *not* a NodeApi-config seam. `src/core/daemon/seam_migration.h`
(`kConfigMigration`, `kTargets` `IDaemonConfig` row) records the Phase-4 decision: wire v9 collapsed runtime config into
`ProfileUpdate`/`SetSessionOverlay`; the deprecated `ConfigGet`/`ConfigSet` ops were retired and the daemon trait returns
`Unsupported`. The only implementation is `MockDaemonConfig` (`src/core/config/mock_daemon_config.cpp`), which reads/writes a
**local JSON cache** (`daemon_config.json`) and seeds believable defaults. **Nothing reads `safety/*` back to gate node behavior**
(grep: `safety/` appears only in `mock_daemon_config.cpp`, `SafetySettingsSection.qml`, `tui/pages/hub_settings.cpp`).

The three "Safety" settings are therefore all cosmetic — the app shows them as if they gate approval / filesystem / network
policy, but the connected node never sees them:

| key | rendered as | node-enforced? | real node home (if any) |
|---|---|---|---|
| `safety/approvalPolicy` | "Approval policy" combo (Never/On request/On failure/Always) | **No** (mock JSON only) | **Yes, but elsewhere:** per-session `ISessionSettings::approvalMode` → `SetSessionMode` → `SessionOverlay.approval_mode`, surfaced in `SessionSettingsPopover.qml`. The Safety row is a redundant, cosmetic duplicate of a control that *is* wired. |
| `safety/sandbox` | "Filesystem access" combo (Read only/Workspace write/Full access) | **No** | none — node owns `SandboxPolicy`; there is **no app-facing wire setter**. |
| `safety/allowNetwork` | "Allow network access" toggle | **No** | none — node owns egress policy (`check_url`, etc.); no app-facing wire setter. |

### File inventory (GUI + TUI lockstep)
- GUI: `src/DaemonApp/Pages/SafetySettingsSection.qml` (the three rows).
- TUI: `src/tui/pages/hub_settings.cpp` lines ~199–205 (the same three rows via `configChoice`/`configToggle`).
- Nav (only relevant if we *remove* the section): `src/DaemonApp/Pages/SettingsPage.qml` (`safety` nav entry + `safetyComp`), and the TUI settings-page section list in `hub_settings.cpp`.
- Defaults/options seeded in `src/core/config/mock_daemon_config.cpp` (`safety/*` in `m_values`/`m_options`).
- Mode detection available if we want to label/hide only in daemon mode: `Application::isDaemonBacked()` (C++), TUI detects via `qobject_cast<DaemonConnectionService*>` (see `src/tui/root_widget.cpp`). Not currently exposed to QML as a bindable — would need a small `Q_PROPERTY`/context prop if we gate on mode.

### Remove-vs-label recommendation (per setting — **for your decision**)
Two coherent honest end-states exist; the user asked me to recommend and stop.

- **`safety/approvalPolicy` → REMOVE (recommended).** A real, node-enforced approval control already exists (per-session
  approval mode in `SessionSettingsPopover.qml`). Keeping a second, cosmetic, global copy is exactly the "app implies a policy
  it does not enforce" trap. Removal is safe — nothing reads the key. (Alternative: keep it but relabel + redirect to the
  session popover — worse, since it duplicates a working control.)
- **`safety/sandbox` + `safety/allowNetwork` → LABEL (recommended, low-risk) or REMOVE.** No wire home, node-owned, not
  enforced. Cleanest-honesty is removal; lowest-risk is a clear notice.

**My default recommendation (least structural churn, keeps mock/demo coverage):**
1. Remove the `safety/approvalPolicy` row from both surfaces (it has a real home elsewhere).
2. Add a prominent Kit-styled advisory at the top of the Safety section (GUI `SafetySettingsSection.qml` + TUI `hub_settings.cpp`)
   stating these are local/demo preferences that **do not change the connected node's policy**, and that the enforced
   per-session approval mode lives in the session settings popover.
3. Keep `safety/sandbox` + `safety/allowNetwork` visible but under that notice (labeled cosmetic), OR remove them too.

**Stronger-honesty alternative (more churn):** remove the entire Safety section — delete the three rows, the `safety` nav
entry + `safetyComp` in `SettingsPage.qml`, the TUI section, and the `safety/*` seeds in `mock_daemon_config.cpp`. This also
drops mock/demo UI coverage and touches settings nav on both surfaces.

**→ Please pick: (A) remove approvalPolicy + label sandbox/allowNetwork [my default], (B) remove all three + section, or (C) label all three.**
No node dependency either way.

---

## Item #2 — Stop fabricating tool `ok`/`stdout` locally

### What I found (exact fabrication sites)
On an approval answer, the app **locally synthesizes a successful tool completion** (`status: ok` + fake ANSI `stdout`),
instead of rendering only the node's real `ToolFinished` stream event:

- **GUI:** `src/DaemonApp/Transcript/Transcript.qml`, `onToolApprovalAnswered` (lines ~247–259):
  ```
  if (decision === "approved") {
      editor.updateTypedBlock(blockId, { status:"ok", durationMs:1400, detailKind:"ansi-stream",
                                         stdout:"\u001b[32m✓\u001b[0m approved — command finished\n" })
  }
  ```
- **TUI:** `src/tui/interactive_turn_host.cpp`, `onApprovalDecided` (lines ~33–41): builds a `done` map
  `{status:"ok", durationMs:1400, detailKind:"ansi-stream", stdout:"…approved — command finished\n"}` and
  `m_doc->updateBlockMetadata(id, done)`.

Both run **in daemon mode too** (GUI when `root.turn.active`; TUI unconditionally — `RootWidget` wiring at
`root_widget_wiring.cpp` ~346–349 calls **both** `host->onApprovalDecided(...)` *and* `turn->respondApproval(...)`).
In daemon mode the node also streams the **real** `ToolFinished` — mapped in `DaemonTurnEngine::applyLogPage` →
`toolFinished` event (`daemon_turn_engine.cpp`, `AgentEventKind::ToolFinished`). So the fabrication is a double lie: it
claims `ok`+`stdout` (a) before the node ran the tool and (b) even if the tool later **fails** after approval.

### Why the fabrication exists (and how to remove it honestly)
The **mock** `TurnController` approval branch (`src/DaemonApp/Turn/turn_controller.cpp`, `buildScript` `wantsApproval`,
lines ~329–353) emits a gated `toolStarted{callId:"sim-approve", needsApproval:true, allowPermanent:true, …}` and then, on
resume, only a `text` + `flush` — it **never emits a `toolFinished` for `sim-approve`**. It relies on the answer handlers to
fabricate the completion. So simply deleting the fabrication would leave the mock's gated tool row stuck "running".

The gate-clear itself is legitimate presentation and stays: `be::toolApprovalPatch(decision)`
(`agent_block.cpp`) sets `approval`, `needsApproval:false`, and (on deny) `status:"error"` — it writes **no** `stdout`
and no `status:ok` (confirmed by `core_tests.cpp::toolApprovalPatchContract`). Denial→error aligns with the node injecting a
tool-error on deny (`engine.rs::resolve_approvals` / `ask_host` reject).

### Plan
1. **Remove the fabricated `status:ok`/`stdout` block** from both `Transcript.qml::onToolApprovalAnswered` and
   `interactive_turn_host.cpp::onApprovalDecided`. Keep the gate-clear patch (`toolApprovalPatch` / editor patch) and keep
   forwarding the decision to the engine (`respondApproval`).
2. **Make the mock self-complete (so the simulator stays visually correct without lying via the handler):** override
   `TurnController::respondApproval(requestId, allow[, allowPermanent])` (today it inherits the base default → `resume()`)
   to inject the gated tool's own completion: on allow → `toolFinished{callId:"sim-approve", status:"ok", detailKind:"ansi-stream", stdout:…}`
   then the follow-up text + flush; on deny → `toolFinished{status:"error"}` (or leave the `toolApprovalPatch` error) + a short
   "denied" text + flush. This matches the code comment already claiming "the host drives the tool to ok and we close out."
3. **Daemon mode:** no change beyond removing the fabrication — the node's real `ToolFinished` already renders via
   `applyLogPage`. Verified.

### Adjacent (flagged, likely out of scope — confirm)
The same handlers also locally echo a **clarify** follow-up text ("Thanks — proceeding with: …") when no live turn is bound:
`Transcript.qml::onClarifyAnswered` (else branch) and `interactive_turn_host.cpp::onClarifySubmitted`. This fabricates *text*,
not tool `ok`/`stdout`, and only on the no-live-turn transcript-replay path. It is the same *class* of honesty issue but
outside the literal item-#2 scope. **Recommendation: leave as-is** (it is a reasonable local affordance when replaying a
loaded transcript with no engine attached) unless you want it folded in.

### GUI + TUI lockstep
Shared model/controller changes: `TurnController` (mock completion), `EditorController` (unchanged unless #3), and no logic in
`.qml`/`src/tui` view code beyond removing the fabrication. Both surfaces converge on the same `eventsEmitted → ingest`
completion path (TUI: `root_widget_wiring.cpp` `s->turn->eventsEmitted → onTurnEvents → ingest`).

---

## Item #3 — Plumb `allowPermanent` to the wire (**BLOCKED on node**)

### App-side reality today (100% cosmetic)
- UI: `src/DaemonApp/BlockEditor/qml/components/ToolApprovalBar.qml` — the "Allow permanently" button (lines ~121–125) is
  `visible: root.allowPermanent` (from `toolData.allowPermanent`); click → `decide("approved", true)` →
  `editorController.answerToolApproval(blockId, callId, "approved", /*permanent=*/true)`.
- `EditorController::answerToolApproval(blockId, callId, decision, permanent)` (`editor_controller.cpp` ~375–382): clears the
  gate and emits `toolApprovalAnswered(blockId, callId, decision, permanent)`.
- **The `permanent` bit is then dropped everywhere:**
  - GUI `Transcript.qml::onToolApprovalAnswered(blockId, callId, decision, permanent)` → `root.turn.respondApproval(callId, decision === "approved")` (no `permanent`).
  - TUI `root_widget_wiring.cpp` → `host->onApprovalDecided(callId, decision, permanent)` (TUI host ignores it: `bool /*permanent*/`) + `turn->respondApproval(callId, decision=="approved")` (no `permanent`).
  - Seam: `ITurnEngine::respondApproval(requestId, allow)` — **no `allowPermanent` parameter**.
  - Wire: `DaemonTurnEngine::respondApproval` → `NodeApiCodec::encodeRespondApprovalRequest(session, id, allow)` →
    `host-response-body-approved = { "Approved": bool }` → Rust `HostResponseBody::Approved(bool)`.
- **The button is also mock-only in live mode:** `DaemonTurnEngine::parkOnRequest` (approval branch) emits the approval
  `toolStarted` **without** `allowPermanent`; only the mock `TurnController` sets `allowPermanent:true`
  (`turn_controller.cpp:341`). So against a real node the button never even renders, and in the mock it is cosmetic.

### THE NODE-SIDE DEPENDENCY (precise request for the coordinator)

There are **two distinct approval wire surfaces**, and the app uses **both**. The `resolve_approvals` /
`decision.starts_with("allow")` path cited in the ticket is the **durable inbox** path — **not** the inline path the
transcript "Allow permanently" button currently drives. Both need the flag; here is exactly what exists and what to add.

**Surface 1 — inline live approval (transcript ToolApprovalBar → `respondApproval`):**
- Wire: `request-respond = { "Respond": { "session", "response": host-response } }`,
  `host-response-body-approved = { "Approved": bool }` (CDDL lines ~159, ~173, ~993).
- Rust: `HostResponseBody::Approved(bool)` → consumed **inline** in
  `daemon-node/crates/engine/daemon-core/src/turn.rs::ask_host` (`Approved(true) => Gate::Proceed`, `Approved(false) => Reject`).
  **This does NOT flow through `resolve_approvals`.**

**Surface 2 — durable approvals inbox (`DaemonApprovalsInbox` → `ApprovalRepository::decide` → `encodeApprovalDecideRequest`):**
- Wire: `request-approval-decide = { "ApprovalDecide": { "session": session-id, "request_id": tstr, "allow": bool } }` (CDDL ~1030).
- Rust chain: `node_api/control.rs::approval_decide(session, request_id, allow)` →
  `daemon-store::answer_approval(session, job_id, allow)` sets `JobCompletion.payload = b"allow" | b"deny"` →
  `daemon-core/src/engine.rs::resolve_approvals` reads `decision.starts_with("allow")` (line ~1490), re-runs the tool (allow)
  or injects a tool-error (deny).

**Requested node-side addition (small, back-compatible optional fields; node owns *what* "permanent" persists):**

1. **Inline body flag** — CDDL:
   `host-response-body-approved = { "Approved": bool, ? "allow_permanent": bool }`
   Rust: add `allow_permanent` to the `Approved` arm of `HostResponseBody` (e.g. `Approved { approved: bool, allow_permanent: bool }`
   or a sibling field). `turn.rs::ask_host`: on `approved && allow_permanent`, persist the permanent allow (node's policy
   choice — see (4)) then `Gate::Proceed`.
2. **Durable decide flag** — CDDL:
   `request-approval-decide = { "ApprovalDecide": { "session": session-id, "request_id": tstr, "allow": bool, ? "allow_permanent": bool } }`
   Rust: `approval_decide(session, request_id, allow, allow_permanent)` → `answer_approval(..., allow_permanent)` sets
   `JobCompletion.payload = b"allow_permanent"` when `allow && allow_permanent` (**still `starts_with("allow")`, so the
   existing resolve semantics are unchanged**). `resolve_approvals` additionally branches on `decision == "allow_permanent"`
   to persist the permanent allow.
3. **"Offer" signal (so the app shows the button honestly in live mode)** — add
   `host-request-kind-approval = { "Approval": { "prompt": tstr, ? "allow_permanent_offered": bool } }` (and/or
   `approval-info`), so `DaemonTurnEngine::parkOnRequest` / the inbox can set `allowPermanent` on the rendered row **only when
   the node offers it**. Without this the live button stays hidden (status quo) or is shown unconditionally and hoped-for.
4. **Decide what "permanent" persists (node decision, not the app's):** e.g. flip the session `ApprovalPolicy`
   (`engine.rs::set_approval_policy`), or — dovetailing with Phase 2 `exec-approval-fingerprint` — add the resolved
   `(abs-binary, argv, env-delta, cwd)` fingerprint to a durable per-session allow-list on the `Snapshot`. The app cannot and
   must not decide this; it only sends the intent.
5. **CDDL lockstep + conformance** (matches Phase 4 `conformance-cddl`, which already names `allowPermanent`): update
   `daemon-api.cddl`, pass `just conformance` / `cargo test -p daemon-api --features arbitrary`, then **coordinator runs
   `just update-codec`** so the app's vendored codec (`src/core/daemon/codec/generated/daemon_api_client_types.h` etc.)
   regenerates with the new fields — that regen is the concrete unblock for the app plumbing below.

**Minimal viable node scope if you want the smallest change:** items (1) + (3) alone unblock the *inline* transcript button
(the app's primary allowPermanent surface). Item (2) is only needed if we also add an "allow permanently" action to the durable
approvals inbox. I recommend (1)+(2)+(3) so both surfaces are coherent, but (1)+(3) is a valid smaller first step.

### App-side plumbing that will follow (after the node lands + codec regen)
1. `ITurnEngine::respondApproval(requestId, allow, allowPermanent=false)` — add the 3rd param (default false; base default still `resume()`).
2. `DaemonTurnEngine::respondApproval` → `NodeApiCodec::encodeRespondApprovalRequest(session, id, allow, allowPermanent)`.
3. `NodeApiCodec::encodeRespondApprovalRequest` (`node_api_codec/encode_requests.cpp`) — set the regenerated
   `host_response_body_approved_*allow_permanent*` field.
4. GUI `Transcript.qml::onToolApprovalAnswered(..., permanent)` → `root.turn.respondApproval(callId, decision==="approved", permanent === true)`.
5. TUI `root_widget_wiring.cpp` → `turn->respondApproval(callId, decision=="approved", permanent)`.
6. `DaemonTurnEngine::parkOnRequest` — set `allowPermanent` on the emitted approval `toolStarted` map from the decoded
   host-request offer flag (decoder side: host-request decode / `DecodedLogEntry` gains the offer flag), so the live button
   renders only when offered.
7. Mock `TurnController::respondApproval(requestId, allow, allowPermanent)` — accept the param for parity (reflect or note it).
8. (Only if surfacing in the durable inbox) `IApprovalsInbox`/`DaemonApprovalsInbox` gains an "allow permanently" action →
   `ApprovalRepository::decide(session, id, allow, allowPermanent)` → `encodeApprovalDecideRequest(..., allowPermanent)`.

### GUI + TUI lockstep for #3
Signature change ripples through shared C++ (`ITurnEngine`, `EditorController`, `DaemonTurnEngine`, `TurnController`, codec)
and both view layers (`ToolApprovalBar.qml`/`Transcript.qml`/`editor_controller` for GUI; `TranscriptView`/`transcript_render`/
`root_widget_wiring` for TUI). No logic added to `.qml`/`src/tui` view code — they only pass the bit through.

---

## Cross-track coordination (sibling `app-session-meta`)
- Sibling owns the view-model/session-meta write path + `seam_migration` continuation. **Shared/overlap files to flag:**
  - `src/core/daemon/seam_migration.h` — I only **read** it (item #1 rationale). I will **not** edit it.
  - `src/DaemonApp/Transcript/Transcript.qml` — I edit it for #2/#3; sibling may touch it for session-meta rendering. **Shared file — coordinate merge order.**
  - `ISessionSettings`/`DaemonSessionSettings` (approval mode) — session-settings territory. Item #1 will **not** wire
    `approvalPolicy` into session settings (that is the sibling's/other work's domain); I only remove/label the mock global row.
  - `src/DaemonApp/Pages/SettingsPage.qml` + `src/tui/pages/hub_settings.cpp` — only touched if item #1 chooses full removal.
- I own: settings safety section, tool-result rendering (transcript approval completion), approval UI (`ToolApprovalBar` +
  approval plumbing).

## Tests
- **#2 unit** — `tests/unit/tst_turn_controller.cpp::approvalPromptResumesViaRespondApproval`: extend to assert the mock now
  emits `toolFinished{callId:"sim-approve", status:"ok"}` after resume; add a **deny** case asserting `toolFinished{error}` /
  no `status:ok`.
- **#2 blockeditor** — `tests/blockeditor/core_tests.cpp`: add a case that `EditorController::answerToolApproval` writes only the
  gate-clear patch (no `status:ok`/`stdout`); `toolApprovalPatchContract` already asserts approve leaves no `status`.
- **#2 GUI (qml)** — add `tests/qml` transcript test: answering approval clears the bar but does **not** synthesize an
  ok/stdout block; the completion arrives only from a later `ingestEvents([{toolFinished, ok}])`.
- **#2 TUI** — `tests/tui/tst_transcript_view.cpp` / `tst_transcript_render.cpp`: `InteractiveTurnHost::onApprovalDecided`
  clears the gate but emits no completion block; the mock engine's scripted `toolFinished` renders. (`tst_transcript_render.cpp`
  already has an `allowPermanent:true` tool fixture at ~line 57 — unaffected by #1/#2, relevant to #3 rendering.)
- **#3 (post-node)** — codec round-trip tests for `allow_permanent` on `encodeRespondApprovalRequest` (and, if used,
  `encodeApprovalDecideRequest`); `parkOnRequest` sets `allowPermanent` only when the offer flag is present;
  `tests/tui/tui_parity_tests.cpp` for allowPermanent GUI/TUI parity.
- **#1** — if labeled: a QML/TUI test asserting the not-enforced notice renders. If removed: remove/adjust any test asserting
  the safety rows and (full removal) the `safety` nav entry.

## Exact gate (run from the worktree root, via the daemon-app devShell — PLAN ONLY, not run yet)
Per `daemon-app/AGENTS.md` (this worktree is a standalone daemon-app checkout with its own flake):
```
nix develop --command bash -euo pipefail -c '
  cmake -B build -G Ninja -DBUILD_TESTING=ON -DDAEMON_APP_TUI=ON && cmake --build build &&
  git ls-files "src/*.cpp" "src/*.h" "tests/*.cpp" "tests/*.h" | xargs clang-format --dry-run --Werror &&
  git ls-files "src/*.cpp" | xargs -P"$(nproc)" -n1 clang-tidy -p build --quiet &&
  cmake --build build --target all_qmllint &&
  QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
'
```
Plus, per the track prompt, from the **superproject** checkout (coordinator runs, esp. for the codec regen + full lint):
`nix develop --command just lint`, `nix develop --command just build-all`, and `just update-codec` after the node-side
`allowPermanent` fields land. (No wasm-surface files are touched, so the wasm gates are not required.)

---

## Suggested implementation order (after approval)
1. **Item #2** (no node dep): remove both fabrications + make the mock `TurnController` self-complete; update #2 tests.
2. **Item #1** (no node dep, once you pick remove-vs-label): apply to GUI + TUI (+ nav/seeds if full removal); tests.
3. **Item #3** (after node lands + `just update-codec`): plumb `allowPermanent` through the seam/codec; render the offer flag; tests.

**STOP — awaiting your review of this plan and the node-side `allowPermanent` decision before touching source.**
