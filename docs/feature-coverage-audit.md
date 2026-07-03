# Feature coverage audit: daemon-app vs hermes-agent (pre-backend)

Status: historical pre-integration snapshot. Several Category A items have since
landed in `daemon-app`; treat this as backlog/context, not exact current state.
For the CURRENT GUI/TUI parity state see the next section (Wave 1, 2026-07); the
hermes comparison below it is unchanged history.

## GUI/TUI parity state (post-Wave 1)

The Wave 1 parity remediation closed every TabModel kind and every real command
route in the TUI. The guardrail test `tests/tui/tui_parity_tests.cpp` enforces
the state below: it fails when a TabModel kind or CommandRegistry id gains a GUI
route without a TUI route (or vice versa), and equally when a stale exemption
lingers after the gap closes.

### Parity policy

- Logic lives in shared C++ models under `src/core/` + `src/DaemonApp/` -
  "build the model once, render twice". A feature is complete only when both
  renderers (QML GUI, Tui Widgets TUI) bind the same model.
- Every new `TabModel::Kind`, every new `CommandRegistry` entry and every
  first-run phase ships with BOTH renderers, or with an explicit, reasoned
  exemption in `tests/tui/tui_parity_tests.cpp`. The parity test is the
  mechanical register of accepted divergences; this document is the narrative
  one.

### Parity matrix (TabModel kinds / major surfaces)

| Surface | GUI | TUI | Notes |
| --- | --- | --- | --- |
| Transcript / chat loop | yes | yes | shared ingest + view models |
| File editor tabs | yes | yes | preview/pin semantics shared |
| Settings | yes | yes (interactive) | seam-backed rows editable in both; see divergences |
| Models | yes | yes | picker, discover -> quant download in both |
| Accounts | yes | yes | add-account wizard ('a'), re-auth pending guard mirrored |
| Profiles | yes | yes | profile editor ('e'), clone, new agent ('a') |
| Profile (per-agent tab) | yes | yes | editable via 'e' in both |
| Fleet / Sessions / Dashboard | yes | yes | read-only projections |
| Approvals | yes | yes | approve/deny keys |
| Routing / Cron | yes | yes | text projections of the same stores |
| Memory | yes | yes | list/stats/timeline shared; graph is adjacency text in TUI |
| Channels | yes | yes | read-only in BOTH this slice (GUI Connect disabled) |
| Users & Access | yes | yes | read-only in BOTH pending the node access-admin API (Auth 5); capability-gated (`access_admin`) fail-closed in both |
| First-run gate | yes | yes | one FirstRunModel: key-validation gate + agent naming shared |
| Command palette | yes | yes | one CommandRegistry, capability-filtered in both |
| File finder / attachments | yes | yes | TUI Ctrl+G finder + Ctrl+O attach picker over the shared FileFinderModel |

### Accepted-divergence register

Rendering-medium divergences (permanent, inherent to a terminal):

- Force-directed graph views (memory graph, routing topology) render as
  adjacency/text projections in the TUI.
- Image / math / mermaid rich rendering: TUI shows text placeholders
  (terminal-graphics protocols are out of scope).
- Embedded terminal panel, drag-drop attachments, OS tray, native file dialogs:
  N/A in a terminal shell (the finder-backed pickers replace the dialogs).

Command exemptions (the only two left in `tui_parity_tests.cpp`):

- `usage` - deliberate no-op in both shells: usage/context is live in both
  status-bar footers.
- `compress` - compaction is a simulated placeholder in BOTH frontends (daemon
  backlog item, not a UI gap).

Per-surface divergences (accepted this wave):

- Accounts: the TUI wizard has no label prompt step - an empty label defaults
  to the provider name, exactly like leaving the GUI field blank. The
  OAuth-failure reason dialog is a small TUI-only addition. Account rename
  stays GUI-inline-only.
- Profiles: the engine field is read-only in the editor in BOTH shells (a
  create-time choice; picked via New Agent). PRO-7 export/import and PRO-8
  history/revert remain GUI-only (file pickers / revision panel). Per-profile
  credential set-key routes through the shared accounts flow.
- Settings: GUI-rendering-only prefs are not mirrored in the TUI (font
  family/size, center text, raw markdown, pane toggles, user rail).
  Test-notification and re-run-first-run actions are deferred. The
  ConnectionPicker connect flow stays first-run/GUI. Archived chats and
  About/updates are unchanged per the placeholder-surface-inventory policy.
- Channels: read-only in both shells this slice (the GUI Connect button is
  disabled too). Reachable only via Nav in both shells - there is no registry
  id yet. Follow-up: add a shared registry/completion id in ONE change so both
  shells gain the palette/slash route together.
- Users & Access: read-only projection of the authenticated principal (WhoAmI)
  pending the node access-admin API (Auth 5); mirrors the GUI page.

This audit compares the daemon-app GUI (Qt/QML) and TUI (Tui Widgets) against the
reference product, hermes-agent's desktop (Electron + React) and TUI (Ink), to decide
what front-end functionality is worth **stubbing client-side now** - behind the existing
seams (`TurnController`, `ISessionStore`, `SessionOrchestrator`,
`StatusBarModel`, `CompletionModel`, `IPlatformServices`) - so that backend integration
becomes a data-source swap rather than new UI work.

## Method

Four surfaces were inventoried:

- daemon-app GUI: `src/DaemonApp/**` (App, Sidebar, SessionsList, Session,
  Transcript, BlockEditor, Composer, ComposerSession, Tabs, StatusBar, StatusModel,
  Settings, Theme, Turn) + `src/core/**` (persistence, platform, presentation).
- daemon-app TUI: `src/tui/**` (reuses the GUI's C++ view-models without Qt Quick).
- hermes desktop: `../daemon-hermes/hermes-agent/apps/desktop/**`.
- hermes TUI: `../daemon-hermes/hermes-agent/ui-tui/**` + `tui_gateway/**`.

## Verdict

daemon-app is at rough parity with hermes on the **core chat loop**, and is arguably
cleaner because both front ends share one C++ view-model layer where hermes reimplements
desktop and TUI separately. Hermes is far ahead on **breadth of agent-management
surfaces** (subagents, profiles, cron, skills, MCP, providers, voice, workspace
files/terminal) - most of which are genuinely backend-coupled and should wait.

Key finding: the simulator seam is already the right shape, but several already-built UI
surfaces are not wired to it (e.g. the GUI status bar renders context/usage/turn items
but nothing feeds them from the active turn). The highest-leverage stub work is connecting
existing UI to `TurnController`-emitted events that mirror the daemon's event vocabulary.

## Core chat loop - at parity (no stub needed)

Both daemon-app front ends already match hermes here:

- Three-pane shell, pane-level preview/pin tabs, adaptive/compact layout.
- Block transcript: messages, reasoning, tool calls (ANSI, web-search rows, unified diff,
  generated images, content), code, tables, mermaid, math, lightbox.
- Approvals + clarify (radio/checkbox/free-text) inline; rewind/edit/restore/regenerate;
  slash rewind (`/retry`, `/edit`, `/undo`).
- Composer: multiline, queue-while-busy, send/steer/stop FSM, sent-history, reverse
  i-search, slash + `@` completions, attachment refs, model pill.
- Sidebar fleet tree / tags / scopes / archived, conversation search, 4 synced themes,
  persisted settings, tray + close-to-tray.

Caveat (expected): answers/turns are mocked (`InteractiveTurnHost`, `TurnController`).
That is the backend seam.

## Gap classification

### A. Stub-ready now (frontend behind existing seams)

These need no daemon; they wire existing UI to the simulator or in-memory store, in both
front ends unless noted.

- Live status: context window / usage / cost / turn timer. The GUI `StatusBarModel`
  (`src/DaemonApp/StatusModel/status_bar_model.h`) shows static `12500/128000`, and
  busy/timer is only wired in the TUI (`src/tui/root_widget.cpp`); neither shows
  usage/cost. Have `TurnController` emit `usage`/`context`/`rate-limit` deltas and bind
  them in `StatusBarModel` for both front ends. Parity win + pre-shapes the event contract.
- Model picker + modes. Replace the canned `ComposerSessionController::models`
  (`src/DaemonApp/ComposerSession/composer_session_controller.h`) pill with a real picker
  overlay (GUI) / picker overlay (TUI, like hermes `modelPicker.tsx`) plus reasoning-effort
  / fast-mode / verbose toggles writing to client state.
- Command palette (both front ends). Hermes' command-palette (nav, settings deep-links,
  theme/session jump) has no daemon-app equivalent. Pure client-side over existing
  actions/stores: GUI opens on Mod+K, TUI on Ctrl+P, both backed by one shared
  `CommandRegistry`.
- Expanded slash catalog. Add hermes-style local commands satisfiable client-side now:
  `/title`, `/save` (export), `/usage`, `/compress` (simulated summary), `/model`,
  `/help`. Route through `ConversationOrchestrator::invokeCommand` + `CompletionModel`
  (`src/DaemonApp/ComposerSession/completion_model.cpp`). (`/branch` is excluded - it
  depends on the deferred branch/fork feature.)
- Session actions: rename, delete, export-transcript (JSON), pin/reorder - expressible
  against `IConversationStore` (`src/core/persistence/iconversation_store.h`) now.
  daemon-app has an archived scope but lacks the rename/export/pin UX in hermes
  `session-actions-menu`.
- Extra HITL surfaces: sudo-password and secret/API-key masked prompts, and a
  destructive-confirm dialog (used by `/clear`, etc.). Build as UI shells answered by
  `InteractiveTurnHost` (`src/tui/interactive_turn_host.h`) / the GUI mock host now; swap
  the responder to the daemon `HostRequestKind` later. Keep both front ends in parity.
- Live subagent/delegation progress. daemon-app has a static seeded fleet tree; hermes
  shows live subagent rows in the composer status stack + an agents dashboard. Add a
  subagents model fed by simulated `subagent.*` events; render a status-stack row (both)
  and optionally a fleet-overlay. Aligns with the daemon's delegation concept.
- OS notifications for approvals. Extend `IPlatformServices`
  (`src/core/platform/iplatform_services.h`) with a `notify()` capability and raise a
  native notification when a turn hits an approval/clarify gate. Desktop-only, but fits
  the seam.

### B. Partially present - finish/wire (not new features)

- GUI status bar left cluster (Command Center, Agents, Cron, Gateway) are local toggles
  that open nothing - hide until backed, or stub minimal panels.
- Attachments are reference-only (`@file:`/`@image:` appended); a local file picker that
  inserts refs is a cheap frontend improvement (real read/upload is backend-coupled).
- Steering is logged as `(steer) ...` text only; UI exists.

### C. Backend-coupled - defer until `../daemon`

Low value to stub now (mostly data/transport, not exercisable UI), and some depend on
whether the NodeApi exposes them at all (append-only turns, one-shot HITL, filesystem-only
checkpoints): embedded terminal/PTY, file-preview rail + workspace/git tree, voice
input + TTS, providers/OAuth + API-keys/env, MCP servers, cron scheduling,
messaging-platform agents, profiles, skills/toolsets catalog, self-update,
gateway-connecting/onboarding/install overlays, command-center usage analytics, artifacts
browser.

## Decision (scope for the near-term stub pass)

Port the following Category-A items, at GUI/TUI parity where it makes sense:

1. Live status (context / usage / cost / turn timer).
2. Model picker + reasoning-effort / fast / verbose modes.
3. Command palette in both front ends (GUI Mod+K, TUI Ctrl+P, shared CommandRegistry).
4. Expanded slash catalog (`/title`, `/save`, `/usage`, `/compress`, `/model`, `/help`).
5. Session actions (rename, delete, export, pin/reorder).
6. Extra HITL surfaces (sudo, secret, destructive-confirm).
7. Live subagent/delegation progress.
8. OS notifications for approvals (desktop).

Explicitly excluded from this pass:

- Branch / fork at a message (and therefore the `/branch` slash command).
- Reachable settings / help entry point in the TUI. (Since closed by Wave 1:
  the TUI Settings page is reachable via /settings and interactive - see the
  parity matrix above.)

## Seam notes

- Because both front ends consume the same C++ models, most stubs are "build the model
  once, render twice" - the cost of TUI parity is a painter widget, not a second state
  implementation.
- Keep funnelling new agent behaviors through `TurnController` event emission using the
  daemon's event names (`usage`, `context`, `subagent.*`, `tool.*`, `Rewound`, ...), so
  the eventual swap to a `NodeApi` adapter stays a single integration point - as the
  rewind seam already does.
