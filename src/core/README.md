# core

Plain C++ libraries with **no QML** - the non-visual foundation of the app.
Feature modules under `src/DaemonApp/` depend on these; `core` never depends on
any QML module.

- `domain/` - entities and value types (`da_domain`).
- `persistence/` - session store interfaces plus in-memory and SQLite stores.
- `platform/` - OS integration behind desktop/mobile implementations.
- `settings/` - local app settings such as first-run and saved connection state.
- `connection/` - daemon connection state/config interface and mock connection.
- `fs/` - file-system browser/editor service interface and local-disk dev implementation.
- `memory/` - memory inspection service interface and seeded mock.
- `models/`, `accounts/`, `profiles/`, `fleet/`, `automation/`, `session/` -
  pre-backend service interfaces and mock stores for manager pages.
- `nav/`, `appcache/`, `uimodels/`, `theme/` - shared infrastructure used by
  GUI and TUI surfaces.

Dependency direction: feature modules depend on `core`; backend adapters should
implement core interfaces rather than reaching into QML modules.
