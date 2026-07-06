# Placeholder Surface Inventory

Status: current cleanup inventory. Use this file to keep visible mock, disabled,
or deferred UI honest until daemon-backed implementations land.

## Policy

- **Hide** controls that imply immediate action but have no behavior.
- **Keep visible disabled** controls only when they explain an unavailable
  platform/build capability.
- **Keep visible mock-backed** surfaces when they exercise an intentional seam,
  but label the backing as mock/demo in code comments or nearby docs.
- **Promote to backlog** public APIs that are useful but cannot work until a
  daemon adapter exposes more data.

See also the GUI/TUI parity matrix + accepted-divergence register in
[feature-coverage-audit.md](feature-coverage-audit.md) (post-Wave 1 state); the
guardrail test `tests/tui/tui_parity_tests.cpp` enforces the route/exemption
bookkeeping mechanically.

## Current Surfaces

- `src/DaemonApp/StatusBar/StatusBar.qml`: Command Center, Agents, and Cron now
  route to existing manager pages. YOLO and Terminal are hidden behind local
  availability flags until real behavior exists.
- `src/DaemonApp/StatusBar/GatewayMenu.qml`: Open System and View All Logs route
  to the dashboard for now. Replace the dashboard target with a dedicated logs
  page when one exists.
- `src/DaemonApp/Pages/ConnectionPicker.qml`: Embedded mode stays visible but
  disabled because full-node FFI is intentionally unavailable.
- `src/DaemonApp/Pages/AboutSection.qml`: Check for updates stays visible but
  disabled because updater support is build/platform dependent and not configured.
- `src/DaemonApp/Pages/Placeholder.qml`: Generic fallback for unlisted or
  deliberately deferred settings/page sections. Avoid routing users to it from
  primary navigation unless the section is intentionally planned.
- `src/DaemonApp/Files/qml/FileExplorer.qml` and `FileTree.qml`: Follow-active
  reveal was removed from the public QML surface until parent-chain resolution is
  available from the model/daemon adapter.
- `src/DaemonApp/Turn/turn_controller.*` and
  `src/DaemonApp/Session/session_orchestrator.*`: Scripted turns and status-stack
  todos are simulator-backed UI coverage, not daemon output.
- `/compress`: compaction is a simulated placeholder in BOTH frontends (daemon
  backlog item, not a UI gap); exempted with that reason in
  `tests/tui/tui_parity_tests.cpp`.
- Channels page (GUI `ChannelsPage.qml`, TUI `pages/hub_channels.cpp`):
  Connect stays disabled (EIO-2; the wizard/auth stream owns the AuthFlowSheet).
  Per-account **Disconnect is visible-disabled with the reason** — the wire has
  NO transport disconnect/remove op (B3; node-first future wire candidate,
  deliberately NOT in the v29 batch). The partial lever that IS wired:
  "Remove the stored credential…" (CredentialRemove, confirmed + clearly
  labeled) on accounts with a bound profile. Room lists + route-pin chips are
  live (B4/B6). Reachable via Nav only (no registry/completion id yet;
  add one in a single change for both shells).
- Users & Access page (GUI, TUI `pages/hub_users_access.cpp`): read-only
  WhoAmI projection in both shells pending the node access-admin API (Auth 5);
  the `access_admin` capability gate fails closed in both.

## Backlog

- Add a real gateway/system log surface and route GatewayMenu's View All Logs to
  it.
- Wire approval/YOLO mode into the turn submission policy before showing the
  status-bar control.
- Add an embedded connection adapter or remove embedded mode from first-run and
  settings.
- Add update-check platform support or remove the About action for builds that do
  not ship an updater.
- Implement file-tree reveal once the explorer model can expand parent chains.
