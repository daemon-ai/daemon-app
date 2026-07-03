#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# Headless boot check for the wasm artifact set: serve `dir` over localhost,
# drive headless chromium over the DevTools protocol, and wait for the app's
# own boot banner on the console (see wasm-boot-smoke.py for mechanics).
#
#   scripts/wasm-boot-smoke.sh [dir] [port]
#
# `dir` defaults to `nix build .#wasm`'s share/daemon-app/wasm. The screenshot
# lands at $SMOKE_SHOT (default /tmp/size-smoke.png), the console log next to
# it as .log. CHROMIUM overrides the browser binary (some hosts wrap chromium
# in firejail, which cannot run headless here). Exit 0 = booted.
#
# The default run also asserts the shell's WebGL probe chose the GL path
# (chromium always has one, via swiftshader at worst), so a regression into
# the software fallback fails the smoke. Override with
# SMOKE_EXPECT_RENDERER=software when driving a deliberately WebGL-less
# browser, or SMOKE_EXPECT_RENDERER="" to skip the assertion.
set -euo pipefail

dir="${1:-}"
if [ -z "$dir" ]; then
  out="$(nix build .#wasm --print-out-paths --no-link)"
  dir="$out/share/daemon-app/wasm"
fi
port="${2:-8901}"
shot="${SMOKE_SHOT:-/tmp/size-smoke.png}"

expect_args=()
expect="${SMOKE_EXPECT_RENDERER-webgl}"
if [ -n "$expect" ]; then
  expect_args=(--expect-renderer "$expect")
fi

exec python3 "$(dirname "$0")/wasm-boot-smoke.py" "$dir" \
  --port "$port" --shot "$shot" --timeout "${SMOKE_TIMEOUT:-90}" \
  "${expect_args[@]}"
