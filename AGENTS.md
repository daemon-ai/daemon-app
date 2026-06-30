# AGENTS.md — daemon-app (Qt 6 C++ / QML + TUI)

Nix-managed. There are NO host tools — build and analyze inside the devShell:
`nix develop --command <cmd>` (or the superproject `just` recipes). CMake emits
`compile_commands.json` (`CMAKE_EXPORT_COMPILE_COMMANDS=ON`) for the analyzers; the
`<DEP>_SOURCE_DIR` vars are exported by the devShell, so CMake resolves them automatically.

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
