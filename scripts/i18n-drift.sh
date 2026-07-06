#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# i18n source-vs-catalog drift gate. Catches the desync the parity checker
# (i18n/check_translations.py) cannot see: a qsTr()/tr()/QObject::tr() string was
# added, reworded, or removed but `update_translations` was never re-run, so the
# committed catalogs no longer match the source. (The parity checker only proves
# the 12 catalogs agree with *each other* -- they can all be stale together.)
#
# Mechanism: regenerate the catalogs with lupdate (the update_translations target,
# which is deterministic because LUPDATE_OPTIONS pins `-no-obsolete -locations
# none`), then fail if any i18n/daemon-app_*.ts changed on disk. A non-empty diff
# means the catalogs were stale: re-run `update_translations` and translate the
# new `type="unfinished"` entries (see i18n/README.md).
#
# Runs inside the Nix devShell (lupdate/cmake come from there):
#     nix develop --command bash scripts/i18n-drift.sh
#
# Honors DAEMON_APP_BUILD_DIR (default: build). Configures that build dir if it
# has no CMake cache yet.
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

build_dir="${DAEMON_APP_BUILD_DIR:-build}"
ts_glob=(i18n/daemon-app_*.ts)

if [ ! -e "${ts_glob[0]}" ]; then
    echo "i18n-drift: no i18n/daemon-app_*.ts catalogs found" >&2
    exit 2
fi

# Configure the build tree if needed (update_translations only needs the CMake
# config + lupdate; a full build is not required).
if [ ! -f "$build_dir/CMakeCache.txt" ]; then
    echo "i18n-drift: configuring $build_dir ..."
    cmake -B "$build_dir" -G Ninja -DBUILD_TESTING=ON -DDAEMON_APP_TUI=ON >/dev/null
fi

# Snapshot the catalogs before regenerating. Content hashes work whether or not
# the files are tracked yet (git diff alone misses not-yet-committed catalogs).
before="$(sha256sum "${ts_glob[@]}" | sort)"

echo "i18n-drift: running update_translations (lupdate) ..."
cmake --build "$build_dir" --target update_translations >/dev/null

after="$(sha256sum "${ts_glob[@]}" | sort)"

if [ "$before" != "$after" ]; then
    echo "i18n-drift: catalogs are stale -- lupdate changed them." >&2
    echo "Re-run 'cmake --build $build_dir --target update_translations' and translate" >&2
    echo "the new type=\"unfinished\" entries, then re-run this gate. Changed files:" >&2
    # Show the human-readable diff for anything git is already tracking.
    git --no-pager diff --stat -- "${ts_glob[@]}" >&2 || true
    diff <(printf '%s\n' "$before") <(printf '%s\n' "$after") >&2 || true
    exit 1
fi

echo "i18n-drift: OK -- catalogs are in sync with the source strings."
