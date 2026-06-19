# daemon-app

An AI agent chat application built with Qt 6.11 (QML/C++). Pure Qt Quick
(`QQuickWindow`) UI so the same codebase targets desktop and mobile.

This repository is currently a **scaffold / quality foundation** - the directory
structure, build system, tooling, and dependency flake are in place, plus a
minimal app that opens an empty window. Feature modules are populated in later
steps.

## Structure

QML feature modules live under `src/DaemonApp/<Feature>/`, where the folder path
mirrors the module URI (`DaemonApp.<Feature>`). Shared, non-visual C++ lives in a
small plain-C++ `src/core/`.

```
src/
  core/                # plain C++ libs (no QML)
    domain/            # entities (Conversation; Message/Folder/Tag later)  -> da_domain
    persistence/       # DbManager + repositories + migrations              -> da_persistence
    platform/          # OS services behind interfaces (desktop/ + mobile/) -> da_platform
  DaemonApp/           # QML feature modules (URI == folder path)
    App/               # DaemonApp.App         shell window + main()
    Theme/             # DaemonApp.Theme        palette / theming
    Controls/          # DaemonApp.Controls     reusable UI kit
    Sidebar/           # DaemonApp.Sidebar      All Conversations / Archived / Folders / Tags
    ConversationsList/ # DaemonApp.ConversationsList  list of chat threads
    Conversation/      # DaemonApp.Conversation center container (Transcript + Composer)
    Transcript/        # DaemonApp.Transcript   scrollable message bubbles
    Composer/          # DaemonApp.Composer     bottom-middle message input
    Settings/          # DaemonApp.Settings     settings menu
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
- Dependency direction: `core/domain <- core/persistence`; feature modules depend
  on `core/*`; nothing depends on `App`.

## Building

Dependencies and the toolchain come from the Nix flake dev shell.

```sh
nix develop                          # cmake, ninja, clang-tools, Qt 6, dep source dirs
cmake --preset desktop-debug
cmake --build --preset desktop-debug
```

This builds `daemon-app`, which currently opens an empty window.
