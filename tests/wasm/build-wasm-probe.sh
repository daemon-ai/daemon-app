#!/usr/bin/env bash
# WASM smoke gate driver (spec 09 §11). Run from inside `nix develop .#wasm` (emscripten + the
# header-only immer source exported as IMMER_SOURCE_DIR). Compiles the minimal immer probe with
# em++ (exceptions off, single-threaded, IMMER_NO_THREAD_SAFETY) and runs it under node. A missing
# sentinel or nonzero exit fails the gate.
#
# The lager reactive-core portion was removed when ADR-008 was FINALIZED to the coarse-signals
# seam (A3): the mirror no longer vendors lager/zug, so the wasm gate tracks only immer, the
# substrate that ships. See src/core/mirror/ADR-008-addendum-A3.md.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
out="${1:-/tmp/mirror_wasm_probe.js}"

: "${IMMER_SOURCE_DIR:?set by the .#wasm devShell}"

echo "em++ = $(command -v em++)"
em++ --version | head -1

em++ -std=c++20 -Oz -fno-exceptions -fno-rtti \
    -DIMMER_NO_THREAD_SAFETY=1 \
    -isystem "$IMMER_SOURCE_DIR" \
    -s ENVIRONMENT=node -s WASM=1 \
    "$here/mirror_wasm_probe.cpp" -o "$out"

echo "--- running under node ---"
node "$out"
