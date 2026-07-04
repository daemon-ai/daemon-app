#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# glibc-floor gate: scan every packaged ELF for versioned glibc/libstdc++
# symbol requirements (readelf --dyn-syms) and fail if any exceeds the
# recorded floor. Run over an extracted package tree:
#
#   scripts/glibc-floor.sh <dir> [<dir>...]
#
# Floors are overridable via env (raise deliberately, never silently):
#   DAEMON_APP_GLIBC_FLOOR    (default 2.38   - measured on the v1 payload)
#   DAEMON_APP_GLIBCXX_FLOOR  (default 3.4.31 - measured on the v1 payload)
#
# v1 caveat: the payload is Nix-linked (glibc 2.42 toolchain), so the floor
# records what the artifacts require today; the static-qt workstream's
# portable payload is expected to LOWER it, and this gate keeps it from
# creeping back up.
set -euo pipefail

if [ "$#" -lt 1 ]; then
    echo "usage: $0 <dir> [<dir>...]" >&2
    exit 2
fi

glibc_floor="${DAEMON_APP_GLIBC_FLOOR:-2.38}"
glibcxx_floor="${DAEMON_APP_GLIBCXX_FLOOR:-3.4.31}"

# version_le A B: true when A <= B in dotted-numeric ordering.
version_le() {
    [ "$(printf '%s\n%s\n' "$1" "$2" | sort -V | tail -1)" = "$2" ]
}

fail=0
scanned=0
while IFS= read -r -d '' file; do
    # ELF magic probe: cheap and dependency-free (no `file` needed).
    head -c4 "$file" 2>/dev/null | od -An -tx1 | grep -q '7f 45 4c 46' || continue
    scanned=$((scanned + 1))
    for fam in GLIBC GLIBCXX; do
        case "$fam" in
            GLIBC) floor="$glibc_floor" ;;
            GLIBCXX) floor="$glibcxx_floor" ;;
        esac
        max="$(readelf --dyn-syms -W "$file" 2>/dev/null |
            grep -o "@${fam}_[0-9.]*" | sed "s/@${fam}_//" | sort -uV | tail -1 || true)"
        [ -n "$max" ] || continue
        if ! version_le "$max" "$floor"; then
            echo "FAIL: $file requires ${fam}_$max > floor ${fam}_$floor" >&2
            fail=1
        fi
    done
done < <(find "$@" -type f -print0)

if [ "$scanned" -eq 0 ]; then
    echo "FAIL: no ELF files found under: $*" >&2
    exit 1
fi

if [ "$fail" -ne 0 ]; then
    echo "glibc floor exceeded (floors: GLIBC_$glibc_floor GLIBCXX_$glibcxx_floor)" >&2
    exit 1
fi
echo "glibc-floor ok: $scanned ELF file(s) within GLIBC_$glibc_floor / GLIBCXX_$glibcxx_floor"
