#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# Generate (and optionally sign) the authenticated release feed: manifest.json.
#
# This is the REAL update feed (packaging/UPDATES.json is only the legacy
# QSimpleUpdater mirror; see scripts/sync-updates-json.sh). Schema, signing
# flow, threat model, and the per-artifact update-capability dial are specified
# in packaging/UPDATES.md.
#
# Usage:
#   scripts/release-manifest.sh <artifact-dir> <version> <channel>
#       [--out FILE]          output path (default: <artifact-dir>/manifest.json)
#       [--notes-url URL]     release-notes page recorded in the manifest
#       [--released ISO8601]  override the release timestamp (defaults to now,
#                             or to SOURCE_DATE_EPOCH when set, both UTC)
#   scripts/release-manifest.sh --verify <manifest.json> <pubkey-file> [sig-file]
#
# Artifact discovery: every regular file in <artifact-dir> whose name matches a
# known packaging kind (.AppImage/.deb/.rpm/.exe/.dmg/.tar.{gz,xz}) becomes an
# artifacts[] entry with size + sha256. Sidecars are consumed, never listed:
#   <file>.glibc   first line becomes glibcFloor (linux artifacts only)
#   <file>.zsync   presence records the zsync URL (AppImage delta transport)
# `file`/`zsync` values are bare names, resolved relative to the manifest's own
# published location, so a feed relocates (bucket/CDN/mirror) without rewrites.
#
# Signing (detached, Ed25519 via minisign):
#   MINISIGN_SECRET_KEY_FILE=/path/to/minisign.key  -> writes <out>.minisig
#   unset                                           -> UNSIGNED manifest (dev only)
# The signed trusted comment pins "daemon <channel> <version>", so a valid
# signature from one channel/version cannot be replayed onto another.
#
# Key handling (keys are NEVER committed to this or any repo):
#   generate : minisign -G -p minisign.pub -s minisign.key
#              (use -W for a passwordless key only inside a CI secret store)
#   secret   : lives exclusively in the release CI's secret store; point
#              MINISIGN_SECRET_KEY_FILE at it in the signing job
#   public   : minisign.pub's second line is the base64 key the app pins at
#              compile time; committing the *public* key there is expected
#
# Tools: needs jq + sha256sum (coreutils) + minisign, all in the devShell
# (flake.nix); standalone: nix run nixpkgs#minisign, nix run nixpkgs#jq.
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  release-manifest.sh <artifact-dir> <version> <channel>
      [--out FILE] [--notes-url URL] [--released ISO8601]
  release-manifest.sh --verify <manifest.json> <pubkey-file> [sig-file]

Set MINISIGN_SECRET_KEY_FILE to sign the emitted manifest (detached .minisig).
Full schema/signing/key-handling documentation: header of this script and
packaging/UPDATES.md.
EOF
  exit "${1:-2}"
}

err() { printf 'release-manifest: error: %s\n' "$*" >&2; }
warn() { printf 'release-manifest: warning: %s\n' "$*" >&2; }

case "${1:-}" in --help|-h) usage 0 ;; esac

# ---------------------------------------------------------------------------
# --verify mode: authenticate an existing manifest against a pinned pubkey.
# ---------------------------------------------------------------------------
if [ "${1:-}" = "--verify" ]; then
  [ $# -ge 3 ] || usage
  manifest="$2"
  pubkey="$3"
  sig="${4:-$manifest.minisig}"
  for f in "$manifest" "$pubkey" "$sig"; do
    [ -f "$f" ] || { err "no such file: $f"; exit 1; }
  done
  # -V prints the verdict + the signed trusted comment (channel/version pin).
  minisign -V -p "$pubkey" -m "$manifest" -x "$sig"
  exit $?
fi

# ---------------------------------------------------------------------------
# generate mode
# ---------------------------------------------------------------------------
[ $# -ge 3 ] || usage

artifact_dir="$1"
version="$2"
channel="$3"
shift 3

out=""
notes_url=""
released=""
while [ $# -gt 0 ]; do
  case "$1" in
    --out)        out="${2:?--out needs a value}"; shift 2 ;;
    --notes-url)  notes_url="${2:?--notes-url needs a value}"; shift 2 ;;
    --released)   released="${2:?--released needs a value}"; shift 2 ;;
    --help|-h)    usage 0 ;;
    *)            err "unknown argument: $1"; usage ;;
  esac
done

[ -d "$artifact_dir" ] || { err "not a directory: $artifact_dir"; exit 1; }
# SemVer core with optional pre-release; build metadata is per-build, not per-release.
if ! printf '%s' "$version" | grep -Eq '^[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z.-]+)?$'; then
  err "not a SemVer version: $version"
  exit 1
fi
case "$channel" in
  *[!a-z0-9-]*|'') err "channel must be lowercase [a-z0-9-]: '$channel'"; exit 1 ;;
esac
[ -n "$out" ] || out="$artifact_dir/manifest.json"

if [ -z "$released" ]; then
  if [ -n "${SOURCE_DATE_EPOCH:-}" ]; then
    released="$(date -u -d "@$SOURCE_DATE_EPOCH" +%Y-%m-%dT%H:%M:%SZ)"
  else
    released="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  fi
fi

# kind + os from the artifact filename. Kinds and the capability dial they map
# to are the settled per-artifact table (packaging/UPDATES.md): the dial value
# recorded here is the artifact's TARGET ceiling; the binary's compiled dial and
# this field are combined as min() by the client, so the feed can lower (kill a
# bad updater) but never raise what a shipped binary may do.
classify() {
  # $1 = filename; prints "kind os capability" or nothing to skip the file.
  local name="${1,,}"
  case "$name" in
    *.appimage)        echo "appimage linux SelfApply" ;;
    *.deb)             echo "deb linux Notify" ;;
    *.rpm)             echo "rpm linux Notify" ;;
    *.exe)             echo "nsis windows SelfApply" ;;
    *.dmg)             echo "dmg macos DownloadAndOpen" ;;
    *.tar.gz|*.tgz|*.tar.xz)
      local os=linux
      case "$name" in
        *darwin*|*macos*) os=macos ;;
        *windows*|*win64*|*win32*) os=windows ;;
      esac
      echo "portable-tar $os Notify"
      ;;
    *) return 0 ;; # sidecars (.minisig/.zsync/.glibc/.sha256/...) and anything unknown
  esac
}

# arch from the artifact filename, normalized to the manifest vocabulary.
arch_of() {
  local name="${1,,}"
  case "$name" in
    *x86_64*|*x86-64*|*amd64*|*x64*) echo x86_64 ;;
    *aarch64*|*arm64*)               echo aarch64 ;;
    *) warn "cannot parse arch from '$1'; recording \"unknown\""
       echo unknown ;;
  esac
}

command -v jq >/dev/null || { err "jq not found (enter the devShell: nix develop)"; exit 1; }

artifacts='[]'
count=0
while IFS= read -r path; do
  name="$(basename "$path")"
  spec="$(classify "$name")"
  [ -n "$spec" ] || continue
  read -r kind os capability <<<"$spec"
  arch="$(arch_of "$name")"
  size="$(stat -c%s "$path")"
  sha256="$(sha256sum "$path" | cut -d' ' -f1)"

  entry="$(jq -n \
    --arg kind "$kind" --arg os "$os" --arg arch "$arch" --arg file "$name" \
    --argjson size "$size" --arg sha256 "$sha256" --arg capability "$capability" \
    '{kind: $kind, os: $os, arch: $arch, file: $file, size: $size,
      sha256: $sha256, updateCapability: $capability}')"

  # Optional sidecars (see header). glibcFloor only makes sense on linux.
  if [ "$os" = linux ] && [ -f "$path.glibc" ]; then
    glibc_floor="$(head -n1 "$path.glibc" | tr -d '[:space:]')"
    [ -n "$glibc_floor" ] && entry="$(jq --arg g "$glibc_floor" '. + {glibcFloor: $g}' <<<"$entry")"
  fi
  if [ -f "$path.zsync" ]; then
    entry="$(jq --arg z "$name.zsync" '. + {zsync: $z}' <<<"$entry")"
  fi

  artifacts="$(jq --argjson e "$entry" '. + [$e]' <<<"$artifacts")"
  count=$((count + 1))
done < <(find "$artifact_dir" -maxdepth 1 -type f | LC_ALL=C sort)

if [ "$count" -eq 0 ]; then
  err "no release artifacts found in $artifact_dir"
  exit 1
fi

jq -n \
  --arg channel "$channel" --arg version "$version" \
  --arg released "$released" --arg notesUrl "$notes_url" \
  --argjson artifacts "$artifacts" \
  '{schema: 1, product: "daemon", channel: $channel, version: $version,
    released: $released, notesUrl: $notesUrl, artifacts: $artifacts}' > "$out"
echo "release-manifest: wrote $out ($count artifact(s), channel=$channel, version=$version)"

# ---------------------------------------------------------------------------
# Signing hook. An UNSIGNED manifest must never be published: clients pin the
# minisign pubkey and reject feeds without a valid signature (UPDATES.md).
# ---------------------------------------------------------------------------
if [ -n "${MINISIGN_SECRET_KEY_FILE:-}" ]; then
  [ -f "$MINISIGN_SECRET_KEY_FILE" ] || {
    err "MINISIGN_SECRET_KEY_FILE points at a missing file: $MINISIGN_SECRET_KEY_FILE"
    exit 1
  }
  command -v minisign >/dev/null || { err "minisign not found (enter the devShell)"; exit 1; }
  # Trusted comment is covered by the signature: pins product/channel/version so
  # a signed manifest cannot be replayed as another channel's or version's feed.
  minisign -S -s "$MINISIGN_SECRET_KEY_FILE" -m "$out" -x "$out.minisig" \
    -t "daemon $channel $version" \
    -c "daemon release manifest ($channel)"
  echo "release-manifest: signed -> $out.minisig"
else
  warn "manifest is UNSIGNED — set MINISIGN_SECRET_KEY_FILE to sign (dev builds only; never publish unsigned)"
fi
