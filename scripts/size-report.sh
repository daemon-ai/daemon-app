#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# Measure the desktop deliverables: per-binary sizes, installed output size,
# and the full runtime closure of the GUI (.#default) and TUI (.#tui)
# packages. The closure is what a fresh `nix profile install` (or docker
# layer) actually downloads, so that column is the deploy cost.
#
#   scripts/size-report.sh
#
# Optional budget gate: set DAEMON_APP_GUI_CLOSURE_BUDGET_MB and/or
# DAEMON_APP_TUI_CLOSURE_BUDGET_MB (integer MiB) to make the run fail once a
# closure exceeds its budget. Unset budgets just report.
set -euo pipefail

fail=0

report() {
  local label="$1" attr="$2" budget_mb="${3:-}"
  local out bin size closure_bytes closure_mb

  out="$(nix build "$attr" --print-out-paths --no-link)"
  printf '%s (%s)\n' "$label" "$attr"
  printf '  %s\n' "$out"

  # bin/ holds the small Qt env wrappers plus the real ELF entry points they
  # exec (the hidden .<name>-wrapped files); list both, sizes tell them apart.
  while IFS= read -r bin; do
    size=$(stat -c%s "$bin")
    printf '  %-34s %14s bytes\n' "bin/$(basename "$bin")" "$size"
  done < <(find "$out/bin" -maxdepth 1 -type f | sort)

  printf '  %-34s %14s\n' "output (du -sh)" "$(du -sh "$out" | cut -f1)"

  closure_bytes=$(nix path-info -S "$out" | awk '{print $NF}')
  closure_mb=$((closure_bytes / 1024 / 1024))
  printf '  %-34s %14s bytes (%s MiB)\n' "closure (nix path-info -S)" \
    "$closure_bytes" "$closure_mb"

  if [ -n "$budget_mb" ] && [ "$closure_mb" -gt "$budget_mb" ]; then
    printf '  BUDGET EXCEEDED: closure %s MiB > %s MiB\n' "$closure_mb" "$budget_mb"
    fail=1
  fi
  printf '\n'
}

report "GUI" ".#default" "${DAEMON_APP_GUI_CLOSURE_BUDGET_MB:-}"
report "TUI" ".#tui" "${DAEMON_APP_TUI_CLOSURE_BUDGET_MB:-}"

exit "$fail"
