#!/usr/bin/env bash
# ADR-008 gate-1 driver (package A3, spec 09 §4.2) — PRESERVED as the methodology + a coarse-seam
# "representative VM projection TU builds" smoke.
#
# A3 re-ran gate 1 on this representative VM TU set (5 shapes incl. a QML-heavy and a TUI
# consumer). Recorded result (best-of-3, g++ 15.2, -std=c++20 -O1, desktop devShell):
#
#   TU             coarse (clean/best)   lager (clean/best)   d clean   d best
#   conv-list           8.66s / 8.52s        13.39s / 13.24s    +55%      +55%
#   chat-window         9.25s / 8.63s        13.85s / 13.62s    +50%      +58%
#   session-list        8.95s / 8.62s        13.12s / 13.12s    +47%      +52%
#   QML-heavy           9.43s / 9.43s        13.66s / 13.66s    +45%      +45%
#   TUI-consumer        8.61s / 8.61s        13.40s / 13.40s    +56%      +56%
#   TOTAL (5 TU)       44.90s /43.81s        67.42s / 67.04s    +50%      +53%
#
# Gate-1 thresholds: <=15% clean, <=10% incremental. Lager stayed DECISIVELY RED, confirming A1's
# default, so LagerObserve + the lager/zug flake inputs were DELETED. The lager variant can no
# longer be built (the candidate is gone); this driver now times only the surviving coarse seam
# to keep the representative VM TU set compiling green.
#
# Run inside the daemon-app devShell (IMMER_SOURCE_DIR exported):
#   nix develop --command bash tests/mirror/observe_gate1/run_gate1.sh
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$here/../../.." && pwd)"
tu="$here/vm_tu.cpp"
out="$(mktemp -d)"
trap 'rm -rf "$out"' EXIT

CXX="${CXX:-g++}"

base_flags=(
  -std=c++20 -fPIC -c -O1 -g0
  -DIMMER_NO_THREAD_SAFETY=1
  -isystem "$IMMER_SOURCE_DIR"
  -I "$repo/src/core"
  -I "$repo/src/core/mirror/generated"
  -I "$repo/src/tui"
  $(pkg-config --cflags Qt6Core Qt6Gui Qt6Qml TuiWidgetsQt6)
)

kinds=(1 2 3 4 5)
kind_name=( [1]="conv-list " [2]="chat-window" [3]="session-list" [4]="QML-heavy  " [5]="TUI-consumer" )

now() { date +%s.%N; }
elapsed() { echo "$1 $2" | awk '{printf "%.2f", $2-$1}'; }

printf '%-14s | %-10s (coarse, best-of-3)\n' "TU" "compile"
printf -- '---------------+------------------------------\n'
total=0
for k in "${kinds[@]}"; do
  times=()
  for i in 1 2 3; do
    t0="$(now)"
    "$CXX" "${base_flags[@]}" -DTU_KIND="$k" -o "$out/tu_${k}.o" "$tu"
    t1="$(now)"
    times+=("$(elapsed "$t0" "$t1")")
  done
  best="$(printf '%s\n' "${times[@]}" | sort -g | head -1)"
  printf '%-14s | %8ss\n' "${kind_name[$k]}" "$best"
  total="$(echo "$total $best" | awk '{printf "%.2f",$1+$2}')"
done
printf -- '---------------+------------------------------\n'
printf '%-14s | %8ss\n' "TOTAL (5 TU)" "$total"
echo
echo "Coarse seam confirmed (ADR-008). Lager deleted: see ADR-008-addendum-A3.md. CXX=$CXX"
