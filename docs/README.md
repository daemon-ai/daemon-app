# docs

Project documentation and historical audits.

## Current Docs

- `client-sync-architecture.md` - **the client data architecture (shipped): the
  mirror.** Store/journal/ingestor/outbox/projections/codegen end state. The
  normative contract is the superproject spec `docs/architecture/09-specification.md`;
  the per-package build history + the closing F6 audit / known-debt register live
  in `src/core/mirror/LEDGER-*.md` (see `LEDGER-a9.md`).
- `transcript-search.md` - shipped search engine and GUI/TUI find bars, with
  remaining highlight follow-ups.
- `placeholder-surface-inventory.md` - current policy and inventory for visible
  mock, disabled, or deferred UI surfaces.
- `file-browser-workspace-design.md` - file browser/editor design; local-disk
  development implementation exists, daemon FS adapter still pending.
- `memory-visualization-spec.md` - memory UI/API spec; GUI/TUI mock-backed
  surfaces exist, daemon memory adapter still pending.
- `first-run-config-audit.md` - historical first-run/backend-integration audit;
  some early status sections are stale and should be read as context.
- `feature-coverage-audit.md` - historical comparison against `daemon-hermes`;
  useful for backlog shape, not an exact current-state report.

When updating these docs, keep status lines explicit: shipped, mock-backed,
historical, or planned.