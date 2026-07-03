#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# Measure the shipped wasm artifact set: raw + gzip -9 + brotli -q 11 bytes.
#
#   scripts/wasm-size-report.sh [dir]
#
# `dir` defaults to `nix build .#wasm`'s share/daemon-app/wasm. Compressed
# sizes are computed to stdout (nothing written next to the inputs); the
# network cost of a deploy is approximately the brotli column.
set -euo pipefail

dir="${1:-}"
if [ -z "$dir" ]; then
  out="$(nix build .#wasm --print-out-paths --no-link)"
  dir="$out/share/daemon-app/wasm"
fi

printf '%-18s %12s %12s %12s\n' "file" "raw" "gzip-9" "brotli-11"
total_raw=0 total_gz=0 total_br=0
for f in daemon-app.wasm daemon-app.js daemon-app.data qtloader.js daemon-app.html; do
  p="$dir/$f"
  [ -f "$p" ] || continue
  raw=$(stat -c%s "$p")
  gz=$(gzip -9 -c "$p" | wc -c)
  br=$(brotli -q 11 -c "$p" | wc -c)
  total_raw=$((total_raw + raw)) total_gz=$((total_gz + gz)) total_br=$((total_br + br))
  printf '%-18s %12s %12s %12s\n' "$f" "$raw" "$gz" "$br"
done
printf '%-18s %12s %12s %12s\n' "TOTAL" "$total_raw" "$total_gz" "$total_br"
