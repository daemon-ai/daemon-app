# daemon-app

An AI agent chat application built with Qt 6.11 (QML/C++). Pure Qt Quick
(`QQuickWindow`) UI keeps the GUI portable, while shared C++ view models also
back the experimental terminal frontend.

The app is a pre-backend product scaffold: the main shell, transcript renderer,
composer, file/editor tabs, settings, memory views, first-run flow, command
palette, and TUI are present. Many data sources are still mock or local-dev
implementations behind interfaces so daemon adapters can replace them without
rewriting the UI.

## Structure

QML feature modules live under `src/DaemonApp/<Feature>/`, where the folder path
mirrors the module URI (`DaemonApp.<Feature>`). Shared, non-visual C++ lives in a
small plain-C++ `src/core/`.

```
src/
  core/          # plain C++ services, stores, mock adapters, and shared models
  DaemonApp/     # QML feature modules (URI == folder path)
  tui/           # experimental Tui Widgets frontend over shared C++ models
  tests/         # unit, QML, block editor, and TUI tests
```

## Conventions

- QML files: PascalCase (file name == type name).
- C++ files: snake_case (`db_manager.h/.cpp`); classes PascalCase; namespaced
  (`domain`, `persistence`, `platform`).
- CMake: library targets prefixed `da_`; QML module backing targets `da_<feature>`
  with URI `DaemonApp.<Feature>` (target name intentionally differs from URI).
- Modules wire together with `DEPENDENCIES TARGET` + `target_link_libraries`; no
  `OUTPUT_DIRECTORY`, no `RESOURCE_PREFIX` (default `/qt/qml`), no manual
  `qmldir`/`qrc`.
- Dependency direction: feature modules depend on `core/*`; nothing depends on
  `App`. Backend-facing surfaces should sit behind interfaces in `src/core/`.

## Building

Dependencies and the toolchain come from the Nix flake dev shell.

```sh
nix develop                          # cmake, ninja, clang-tools, Qt 6, dep source dirs
cmake --preset desktop-debug
cmake --build --preset desktop-debug
```

This builds the `daemon-app` GUI. The Nix flake also exposes the experimental
`daemon-tui` package with `-DDAEMON_APP_TUI=ON`.
