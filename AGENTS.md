# AGENTS.md — daemon-app (Qt 6 C++ / QML GUI + TUI + WASM)

Nix-managed. There are NO host tools — build and analyze inside the devShell:
`nix develop --command <cmd>` (or the superproject `just` recipes). CMake emits
`compile_commands.json` (`CMAKE_EXPORT_COMPILE_COMMANDS=ON`) for the analyzers; the
`<DEP>_SOURCE_DIR` vars are exported by the devShell, so CMake resolves them automatically.

## Thin client — the node decides, the app renders

Every surface this repo builds (desktop GUI, TUI, WASM preset) is a thin client of
`daemon-node`, which is the single authority for domain state, business logic, validation,
persistence, and orchestration. The app renders node state and sends intents — it never
re-derives, caches-and-mutates, or forks domain behavior locally.

- Data access goes through `src/core` interfaces; the daemon adapters (`src/core/daemon`)
  implement them over the wire contract. The in-repo mocks are dev stand-ins for those same
  interfaces — never a place to grow real behavior.
- View models and QML are presentation-only: formatting, selection, layout state. If a change
  makes the app answer a domain question itself (business rules, validation beyond input
  hygiene, derived domain state), stop — the node API is missing a capability. Extend the Rust
  types + CDDL in `daemon-node`, `just update-codec` from the superproject, then consume it.

## One feature, every surface — GUI + TUI parity is the default

The GUI and TUI are two thin renderers over ONE shared C++ view-model layer. A UI change that
lands GUI-only (or TUI-only) is INCOMPLETE — do not stop at the QML. Parity is the default;
skipping a surface is the exception that needs a stated reason.

- Feature state and behavior go in the shared C++ view models / controllers — the classes
  backing the feature's QML module (`SessionsListModel`, `SidebarModel`,
  `ComposerSessionController`, `TurnController`, ...) or `src/core` — NEVER in `.qml` files or
  `src/tui` view code. QML binds to these models; `src/tui` links the very same classes.
- Wire the TUI counterpart in the same change: a Tui Widgets view in `src/tui` bound to the
  same model (via `DisplayRoleAdapter` when it needs QML custom roles mapped onto
  `Qt::DisplayRole`), hooked up in `root_widget_wiring.cpp`. Only per-surface rendering code
  may differ.
- Litmus test before finishing: a diff that adds user-facing behavior but only touches `.qml` —
  no shared model/controller change, nothing under `src/tui` — has either baked logic into QML
  or dropped the TUI; fix one or both. (Purely cosmetic GUI polish is the one legitimate
  QML-only case.) Cover the shared model in `tests/unit`; surface-specific rendering goes in
  `tests/qml` / `tests/tui`.
- Where exact parity is impossible (inherently graphical content: images, diagrams, graphs),
  the TUI still surfaces the feature in a reduced form (text summary, placeholder, count) —
  never silently omits it. State the degradation and why in the commit/PR.

## UI kit & theming — Daemon Kit only, no custom one-off QML

All visible UI is composed from the Daemon Kit (`DaemonApp.Controls`, imported
`as Kit`) styled by `DaemonApp.Theme` (`Theme` + `FontIcons` singletons). Do not hand-roll
per-feature copies of controls the Kit already has (buttons, fields, checkboxes, switches,
dropdowns, menus, dialogs, tooltips, scrollbars, progress bars, ...).

- **Kit control exists → use it.** Never restyle a raw `QtQuick.Controls` equivalent in a
  feature module.
- **Kit control missing → add it to `DaemonApp.Controls`** (themed via `Theme` tokens, with the
  `QQC as QQC` wrapping pattern the existing controls use), then consume it. One-off styled
  controls inside feature modules are a defect.
- Raw `QtQuick.Controls` types in feature QML are acceptable only for unstyled layout/behavior
  primitives the Kit deliberately doesn't wrap (`Label`, `ScrollView`, `Popup`, `SplitView`,
  `Overlay`, ...) — anything with a themed skin comes from the Kit.
- Every color, font, and icon binds to `Theme.*` / `FontIcons.*` tokens — no hardcoded hex
  colors or font families in feature QML. New tokens go into the shared C++ `ThemePalette`
  (`src/core/theme`, exposed as `DaemonApp.ThemeCore`) plus `Theme.qml`, so the GUI and the TUI
  (`tui_palette`) pick up the same value; all four themes (Light / Dark / Sepia / Midnight) must
  be covered.

## Required gate before finishing

- Build:        `cmake -B build -G Ninja -DBUILD_TESTING=ON -DDAEMON_APP_TUI=ON && cmake --build build`
- clang-format: `git ls-files 'src/*.cpp' 'src/*.h' 'tests/*.cpp' 'tests/*.h' | xargs clang-format --dry-run --Werror`
- clang-tidy:   `git ls-files 'src/*.cpp' | xargs -P"$(nproc)" -n1 clang-tidy -p build --quiet`
- qmllint:      `cmake --build build --target all_qmllint`
- Tests:        `QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure`

(`clang-tools` ships `clang-tidy` but not `run-clang-tidy`; drive it per-TU with `xargs` as above.)
Never bypass the pre-commit hook (no `git commit --no-verify`).

## Conventions

- Formatting is enforced by `.clang-format`; static checks by `.clang-tidy` — don't loosen either
  casually. Keep new code clean against `cmake/CompilerWarnings.cmake`.
- Suppress a clang-tidy finding only with a narrow `// NOLINT(<check>)` plus a reason — never
  file-wide or with a bare `// NOLINT`.
- Do NOT hand-edit `src/core/daemon/codec/generated` or `.../vendor`; regenerate from the
  superproject (`just update-codec`).

## Versioning

`VERSION` (repo root, clean SemVer) is the source of truth: CMake reads it live into
`project(VERSION ...)`, and `cmake/Version.cmake` generates `daemon_app_version.h` with a
git-enriched string (`X.Y.Z+<n>.g<hash>[.dirty]`; the Nix flake injects `-DDAEMON_APP_VERSION_STR`,
dev builds use `git describe`). Both frontends' `main()` call
`QCoreApplication::setApplicationVersion()`; the status bar and About page read it back. Do NOT
hand-edit the generated header or hardcode a version anywhere; `packaging/UPDATES.json` is a derived
mirror.

Bump in the monorepo with `just set-version daemon-app X.Y.Z` (writes `VERSION` and syncs
`UPDATES.json`); standalone, edit `VERSION` (CMake picks it up) and `UPDATES.json`. `just
check-version` validates the SemVer.
