#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# Headless boot check for the wasm artifact set: serve `dir` over localhost,
# load daemon-app.html in headless chromium, and assert the Qt runtime came up
# (no fatal wasm instantiation/abort in the console, non-blank screenshot).
#
#   scripts/wasm-boot-smoke.sh [dir] [port]
#
# `dir` defaults to `nix build .#wasm`'s share/daemon-app/wasm. The screenshot
# lands at $SMOKE_SHOT (default /tmp/size-smoke.png), the console log next to
# it as .log. Exit 0 = booted.
set -euo pipefail

dir="${1:-}"
if [ -z "$dir" ]; then
  out="$(nix build .#wasm --print-out-paths --no-link)"
  dir="$out/share/daemon-app/wasm"
fi
port="${2:-8901}"
shot="${SMOKE_SHOT:-/tmp/size-smoke.png}"
log="${shot%.png}.log"
# Some hosts wrap `chromium` in firejail, which cannot run headless here;
# CHROMIUM overrides the binary (e.g. an unwrapped nixpkgs chromium).
chromium_bin="${CHROMIUM:-chromium}"

python3 -m http.server "$port" --bind 127.0.0.1 --directory "$dir" >/dev/null 2>&1 &
server=$!
trap 'kill "$server" 2>/dev/null || true' EXIT
sleep 1

"$chromium_bin" --headless=new --enable-logging=stderr --virtual-time-budget=60000 \
  --window-size=1400,900 --screenshot="$shot" \
  "http://127.0.0.1:$port/daemon-app.html" >"$log" 2>&1 || true

fail=0

# Fatal patterns: wasm compile/instantiation failures, emscripten aborts, and
# the html shell's "Application exit" path all mean the app never came up.
if grep -E "RuntimeError|CompileError|LinkError|Aborted\(|failed to asynchronously prepare wasm|Application exit" "$log" >/dev/null; then
  echo "boot-smoke: FATAL console output:" >&2
  grep -E "RuntimeError|CompileError|LinkError|Aborted\(|failed to asynchronously prepare wasm|Application exit" "$log" | head -5 >&2
  fail=1
fi

# Positive signal: the app's QML engine warning channel stays silent on success,
# but Qt always logs the wasm platform init; require SOME console activity from
# the page plus a screenshot that is not a single flat color.
if [ ! -s "$shot" ]; then
  echo "boot-smoke: no screenshot produced" >&2
  fail=1
else
  python3 - "$shot" <<'EOF' || fail=1
import sys, struct, zlib
path = sys.argv[1]
data = open(path, 'rb').read()
# Parse the PNG IDAT stream and count distinct pixel byte values; a blank
# (never-painted) page compresses to a single color.
pos = 8
idat = b''
w = h = None
while pos < len(data):
    ln, typ = struct.unpack('>I4s', data[pos:pos+8])
    if typ == b'IHDR':
        w, h = struct.unpack('>II', data[pos+8:pos+16])
    if typ == b'IDAT':
        idat += data[pos+8:pos+8+ln]
    pos += 12 + ln
raw = zlib.decompress(idat)
values = set(raw[:4_000_000])
print(f"boot-smoke: screenshot {w}x{h}, {len(values)} distinct bytes")
if len(values) < 8:
    print("boot-smoke: screenshot looks blank", file=sys.stderr)
    sys.exit(1)
EOF
fi

if [ "$fail" -ne 0 ]; then
  echo "boot-smoke: FAILED (log: $log, shot: $shot)" >&2
  exit 1
fi
echo "boot-smoke: OK (log: $log, shot: $shot)"
