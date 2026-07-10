#!/usr/bin/env bash
# A1 WASM smoke gate driver (spec 09 §11). Run from inside `nix develop .#wasm` (emscripten +
# the header-only immer/zug/lager sources exported as *_SOURCE_DIR). Compiles the minimal
# immer+lager probe with em++ (exceptions off, single-threaded, IMMER_NO_THREAD_SAFETY) and runs
# it under node. A missing sentinel or nonzero exit fails the gate.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
out="${1:-/tmp/mirror_wasm_probe.js}"

: "${IMMER_SOURCE_DIR:?set by the .#wasm devShell}"
: "${ZUG_SOURCE_DIR:?set by the .#wasm devShell}"
: "${LAGER_SOURCE_DIR:?set by the .#wasm devShell}"
: "${BOOST_INCLUDE_DIR:?set by the .#wasm devShell}"

echo "em++ = $(command -v em++)"
em++ --version | head -1

em++ -std=c++20 -Oz -fno-exceptions -fno-rtti \
    -DIMMER_NO_THREAD_SAFETY=1 \
    -isystem "$IMMER_SOURCE_DIR" \
    -isystem "$ZUG_SOURCE_DIR" \
    -isystem "$LAGER_SOURCE_DIR" \
    -isystem "$BOOST_INCLUDE_DIR" \
    -s ENVIRONMENT=node -s WASM=1 \
    "$here/mirror_wasm_probe.cpp" -o "$out"

echo "--- running under node ---"
node "$out"
