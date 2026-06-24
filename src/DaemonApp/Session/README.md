# DaemonApp.Session

The center pane host. It owns the tab strip and keeps transcript, manager,
memory/profile, and file-editor tabs alive while switching between them. Each
transcript tab owns its own controller/orchestrator; app-level manager pages bind
global service seams from `src/core`.
