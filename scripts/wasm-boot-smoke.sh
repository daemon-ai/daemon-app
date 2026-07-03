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
set -euo pipefail

dir="${1:-}"
if [ -z "$dir" ]; then
  out="$(nix build .#wasm --print-out-paths --no-link)"
  dir="$out/share/daemon-app/wasm"
fi
port="${2:-8901}"
shot="${SMOKE_SHOT:-/tmp/size-smoke.png}"

exec python3 "$(dirname "$0")/wasm-boot-smoke.py" "$dir" \
  --port "$port" --shot "$shot" --timeout "${SMOKE_TIMEOUT:-90}"
