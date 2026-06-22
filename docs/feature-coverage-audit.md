# Feature coverage audit: daemon-app vs hermes-agent (pre-backend)

Status: pre-integration snapshot (before wiring the `../daemon` NodeApi backend).

This audit compares the daemon-app GUI (Qt/QML) and TUI (Tui Widgets) against the
reference product, hermes-agent's desktop (Electron + React) and TUI (Ink), to decide
what front-end functionality is worth **stubbing client-side now** - behind the existing
seams (`TurnController`, `IConversationStore`, `ConversationOrchestrator`,
`StatusBarModel`, `CompletionModel`, `IPlatformServices`) - so that backend integration
becomes a data-source swap rather than new UI work.

## Method

Four surfaces were inventoried:

- daemon-app GUI: `src/DaemonApp/**` (App, Sidebar, ConversationsList, Conversation,
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
- Reachable settings / help entry point in the TUI.

## Seam notes

- Because both front ends consume the same C++ models, most stubs are "build the model
  once, render twice" - the cost of TUI parity is a painter widget, not a second state
  implementation.
- Keep funnelling new agent behaviors through `TurnController` event emission using the
  daemon's event names (`usage`, `context`, `subagent.*`, `tool.*`, `Rewound`, ...), so
  the eventual swap to a `NodeApi` adapter stays a single integration point - as the
  rewind seam already does.
