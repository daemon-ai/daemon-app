#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# Patch the legacy QSimpleUpdater mirror (packaging/UPDATES.json) from a signed
# release manifest (manifest.json, produced by scripts/release-manifest.sh).
#
# manifest.json is the authoritative, authenticated feed; UPDATES.json remains
# only as the simple/legacy mirror for QSimpleUpdater-shaped consumers. This
# script keeps the mirror's latest-version / download-url / changelog-url in
# step with a manifest so the two can never silently diverge.
#
# Usage:
#   scripts/sync-updates-json.sh <manifest.json> <UPDATES.json>
#       [--download-url URL]   set download-url on every platform entry
#       [--changelog-url URL]  set changelog-url (defaults to the manifest's
#                              notesUrl when that is non-empty)
#
# Every platform entry under .updates.* gets latest-version = manifest.version;
# the URL fields are patched only when a value is available (flag or manifest),
# so a placeholder mirror is never clobbered with empty strings.
#
# Wiring note: the superproject's `just set-version` already writes
# latest-version when a human bumps VERSION; hooking THIS script into the
# release pipeline (so the mirror follows each published manifest) is a later
# workstream in the superproject. It is deliberately standalone-runnable:
# only bash + jq (devShell, or nix run nixpkgs#jq).
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  sync-updates-json.sh <manifest.json> <UPDATES.json>
      [--download-url URL] [--changelog-url URL]
EOF
  exit "${1:-2}"
}

err() { printf 'sync-updates-json: error: %s\n' "$*" >&2; }

case "${1:-}" in --help|-h) usage 0 ;; esac
[ $# -ge 2 ] || usage

manifest="$1"
updates="$2"
shift 2

download_url=""
changelog_url=""
while [ $# -gt 0 ]; do
  case "$1" in
    --download-url)  download_url="${2:?--download-url needs a value}"; shift 2 ;;
    --changelog-url) changelog_url="${2:?--changelog-url needs a value}"; shift 2 ;;
    --help|-h)       usage 0 ;;
    *)               err "unknown argument: $1"; usage ;;
  esac
done

[ -f "$manifest" ] || { err "no such manifest: $manifest"; exit 1; }
[ -f "$updates" ] || { err "no such updates file: $updates"; exit 1; }
command -v jq >/dev/null || { err "jq not found (enter the devShell: nix develop)"; exit 1; }

version="$(jq -er '.version' "$manifest")" || { err "manifest has no .version"; exit 1; }
[ -n "$changelog_url" ] || changelog_url="$(jq -r '.notesUrl // empty' "$manifest")"

# Patch every app/platform leaf under .updates (the committed shape is
# .updates."daemon-app"."open-source"; keep the walk generic so added platform
# keys inherit the sync). jq's 2-space indent matches the committed formatting.
tmp="$(mktemp "$updates.XXXXXX")"
trap 'rm -f "$tmp"' EXIT
jq \
  --arg version "$version" \
  --arg download "$download_url" \
  --arg changelog "$changelog_url" \
  '.updates |= map_values(map_values(
     ."latest-version" = $version
     | (if $download  != "" then ."download-url"  = $download  else . end)
     | (if $changelog != "" then ."changelog-url" = $changelog else . end)
   ))' "$updates" > "$tmp"
mv "$tmp" "$updates"
trap - EXIT

echo "sync-updates-json: $updates <- $manifest (latest-version=$version)"
