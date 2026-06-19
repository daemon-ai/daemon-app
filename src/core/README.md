# core

Plain C++ libraries with **no QML** - the non-visual foundation of the app.
Feature modules under `src/DaemonApp/` depend on these; `core` never depends on
any QML module.

- `domain/` - entities and value types (`da_domain`).
- `persistence/` - storage layer (`da_persistence`).
- `platform/` - OS integration behind interfaces (`da_platform`).

Dependency direction: `domain <- persistence`; `platform -> domain`.
