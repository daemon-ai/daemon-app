# SPDX-License-Identifier: MPL-2.0
# SPDX-FileCopyrightText: 2026 Jarrad Hope
#
# AppImage SelfApply A->B end-to-end runner (nix run .#updater-appimage-e2e).
# NOT a flake check: it builds real AppImages, runs one, and lets the
# daemon-updater helper swap the on-disk .AppImage - work that needs the host's
# nix + loopback networking, unavailable in the build sandbox (mirrors the
# apps.portable-smoke / apps.windows-smoke pattern).
#
# Flow: generate a throwaway minisign keypair -> build two SelfApply AppImages A
# (9.9.8) and B (9.9.9), both pinned to that test pubkey -> publish a signed
# manifest for B on 127.0.0.1 -> run A with $APPIMAGE pointed at the real on-disk
# A.AppImage and the env-gated E2E auto-drive -> A verifies+downloads+self-applies
# (the real daemon-updater helper rename-swaps the on-disk file) and quits ->
# assert the on-disk .AppImage is now byte-for-byte B, is executable, and is a
# genuinely bootable B (its payload boots offscreen and reports 9.9.9).
#
# The app process itself is launched from A's extracted payload binary (its ELF
# interpreter repointed at the store glibc loader via patchelf), NOT by executing
# the AppImage's type2 runtime: that runtime is a static-pie binary that segfaults
# in this sandbox (the same reason the repo's AppImage boot smoke unsquashes +
# boots the payload rather than running the runtime). The self-apply code path,
# $APPIMAGE handling, helper discovery next to the running binary, and the real
# rename-swap of the on-disk file are exercised exactly as in production; only the
# process's own launch + the helper's relaunch go through the payload binary
# rather than the unrunnable runtime.
#
# Placeholders (@...@) are substituted by nix/appimage-e2e.nix at build time.
set -euo pipefail

FLAKE="@self@"
SYSTEM="@system@"
BOOT_LIB_PATH="@bootLibPath@"
STORE_LOADER="@storeLoader@"
RUNTIME_FILE="@runtimeFile@"

say() { printf '\n=== %s ===\n' "$*"; }

work="$(mktemp -d)"
srv_pid=""
cleanup() {
  [ -n "$srv_pid" ] && kill "$srv_pid" 2>/dev/null || true
  chmod -R u+rwX "$work" 2>/dev/null || true
  rm -rf "$work"
}
trap cleanup EXIT

# The squashfs payload begins right after the pinned type2 runtime.
runtime_size="$(stat -c%s "$RUNTIME_FILE")"

# Unsquash an AppImage's payload to a dir and point its daemon-app binary at the
# store loader, so it runs DIRECTLY (correct /proc/self/exe -> applicationDirPath
# resolves to usr/bin, where daemon-updater sits) without depending on the host's
# /lib64 loader. The type2 runtime cannot execute in this sandbox, so we never
# run the AppImage itself - only its payload binary.
extract_payload() { # $1=appimage $2=destdir
  rm -rf "$2"
  unsquashfs -q -f -d "$2" -o "$runtime_size" "$1" >/dev/null
  patchelf --set-interpreter "$STORE_LOADER" "$2/usr/bin/daemon-app"
}

# A throwaway keypair, generated fresh by default (no committed secret). Set
# DAEMON_APP_E2E_KEYDIR to a dir holding a prior e2e.pub/e2e.key to reuse one -
# purely a local speed-up (the test AppImages are keyed by pubkey, so a stable
# key makes A/B a nix cache hit across re-runs). Never point it at a real key.
if [ -n "${DAEMON_APP_E2E_KEYDIR:-}" ] && [ -f "${DAEMON_APP_E2E_KEYDIR}/e2e.key" ]; then
  say "reusing the keypair in $DAEMON_APP_E2E_KEYDIR"
  cp "$DAEMON_APP_E2E_KEYDIR/e2e.pub" "$DAEMON_APP_E2E_KEYDIR/e2e.key" "$work/"
else
  say "generating a throwaway minisign keypair (never the release key)"
  minisign -W -G -p "$work/e2e.pub" -s "$work/e2e.key" >/dev/null 2>&1
  if [ -n "${DAEMON_APP_E2E_KEYDIR:-}" ]; then
    mkdir -p "$DAEMON_APP_E2E_KEYDIR"
    cp "$work/e2e.pub" "$work/e2e.key" "$DAEMON_APP_E2E_KEYDIR/"
  fi
fi
pub="$(tail -n1 "$work/e2e.pub")"
echo "test pubkey: $pub"

build_image() {
  # $1 = version. Build a SelfApply AppImage pinned to the test pubkey.
  nix build --impure --no-link --print-out-paths \
    --expr "(builtins.getFlake \"$FLAKE\").appimageSelfApplyImage.\"$SYSTEM\" { pubkey = \"$pub\"; version = \"$1\"; }"
}

say "building AppImage A (version 9.9.8)"
outA="$(build_image 9.9.8)"
say "building AppImage B (version 9.9.9)"
outB="$(build_image 9.9.9)"
imgA="$(find "$outA" -maxdepth 1 -name '*.AppImage' | head -n1)"
imgB="$(find "$outB" -maxdepth 1 -name '*.AppImage' | head -n1)"
[ -n "$imgA" ] && [ -n "$imgB" ] || { echo "FAIL: could not locate built AppImages" >&2; exit 1; }

say "publishing a signed feed for B on 127.0.0.1"
feed="$work/feed"
mkdir -p "$feed"
cp "$imgB" "$feed/daemon-app-9.9.9-x86_64.AppImage"
chmod u+w "$feed/daemon-app-9.9.9-x86_64.AppImage"
MINISIGN_SECRET_KEY_FILE="$work/e2e.key" \
  bash "$FLAKE/scripts/release-manifest.sh" "$feed" 9.9.9 stable --out "$feed/manifest.json" >/dev/null
minisign -V -p "$work/e2e.pub" -m "$feed/manifest.json" -x "$feed/manifest.json.minisig" >/dev/null
echo "manifest signed + verified against the test pubkey"
sha_b="$(sha256sum "$feed/daemon-app-9.9.9-x86_64.AppImage" | cut -d' ' -f1)"

say "installing A into a writable directory (the file that gets swapped)"
appdir="$work/app"
mkdir -p "$appdir"
target="$appdir/daemon-app.AppImage"
cp "$imgA" "$target"
chmod u+wx "$target"
sha_a="$(sha256sum "$target" | cut -d' ' -f1)"
echo "installed A: $target"
echo "  sha256(A): $sha_a"
echo "  sha256(B): $sha_b"
[ "$sha_a" != "$sha_b" ] || { echo "FAIL: A and B are identical (version override failed)" >&2; exit 1; }

say "extracting A's payload (the type2 runtime is a static-pie binary that segfaults in this sandbox)"
extract_payload "$target" "$work/payloadA"
[ -x "$work/payloadA/usr/bin/daemon-updater" ] || { echo "FAIL: daemon-updater helper not packaged in the AppImage" >&2; exit 1; }
echo "payload extracted; daemon-updater present in usr/bin"

say "serving the feed over loopback HTTP"
# An unbuffered inline server that binds an ephemeral port and writes it to a
# file the moment it is bound (python -m http.server buffers its banner when
# stdout is a file, so the port can't be scraped reliably).
port_file="$work/http.port"
python3 -u -c '
import http.server, socketserver, sys, os
os.chdir(sys.argv[1])
srv = socketserver.TCPServer(("127.0.0.1", 0), http.server.SimpleHTTPRequestHandler)
with open(sys.argv[2], "w") as f:
    f.write(str(srv.server_address[1]))
srv.serve_forever()
' "$feed" "$port_file" >"$work/http.log" 2>&1 &
srv_pid=$!
port=""
for _ in $(seq 100); do
  if [ -s "$port_file" ]; then port="$(cat "$port_file")"; break; fi
  sleep 0.1
done
[ -n "$port" ] || { echo "FAIL: local HTTP server did not start" >&2; cat "$work/http.log" >&2; exit 1; }
feed_url="http://127.0.0.1:$port/manifest.json"
echo "feed: $feed_url"

# Shared runtime env for the app process.
export HOME="$work/home"
export XDG_RUNTIME_DIR="$work/xdg"
export XDG_DATA_HOME="$work/data"
export XDG_CONFIG_HOME="$work/cfg"
export XDG_CACHE_HOME="$work/cache"
mkdir -p "$HOME" "$XDG_RUNTIME_DIR" "$XDG_DATA_HOME" "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME"
chmod 700 "$XDG_RUNTIME_DIR"
touch "$work/sock-stub"
export QT_QPA_PLATFORM=offscreen
export QT_QUICK_BACKEND=software
export DAEMON_APP_SERVICE_MODE=mock
export DAEMON_APP_SOCKET="$work/sock-stub"
# The static payload expects the host to provide the system library floor
# (glibc + GL/X/fontconfig); hand it the set the portable boot smoke uses.
export LD_LIBRARY_PATH="$BOOT_LIB_PATH${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export DAEMON_APP_UPDATE_FEED_URL_OVERRIDE="$feed_url"
export DAEMON_APP_UPDATE_E2E=1
# $APPIMAGE is what the app treats as itself: the REAL on-disk file that the
# helper will rename-swap. (Set explicitly since we launch the payload directly,
# not through the runtime that would export it.)
export APPIMAGE="$target"

say "running A - expect self-apply to rename-swap $target to B, then quit"
export DAEMON_APP_UPDATE_E2E_LOG="$work/e2e-A.log"
: > "$DAEMON_APP_UPDATE_E2E_LOG"
timeout 240 "$work/payloadA/usr/bin/daemon-app" </dev/null >"$work/runA.log" 2>&1 || true

say "waiting for the helper swap"
ok=0
for _ in $(seq 180); do
  now="$(sha256sum "$target" | cut -d' ' -f1)"
  if [ "$now" = "$sha_b" ]; then ok=1; break; fi
  sleep 1
done

say "A E2E sentinel log"
cat "$work/e2e-A.log" 2>/dev/null || echo "(none)"
say "A run log (tail)"
tail -n 20 "$work/runA.log" 2>/dev/null || true
for lg in "$appdir"/.daemon-update-*/apply.log; do
  [ -f "$lg" ] || continue
  say "helper apply.log ($lg)"
  cat "$lg"
done

say "assertion 1/3: the on-disk .AppImage was rename-swapped to B"
now="$(sha256sum "$target" | cut -d' ' -f1)"
echo "target sha256 now: $now"
echo "expected (B):      $sha_b"
[ "$now" = "$sha_b" ] || { echo "FAIL: the on-disk AppImage was not swapped to B" >&2; exit 1; }
echo "PASS"

say "assertion 2/3: the swapped-in AppImage is executable"
[ -x "$target" ] || { echo "FAIL: swapped AppImage is not executable" >&2; exit 1; }
echo "PASS"

say "assertion 3/3: the swapped-in file is a genuinely bootable B (payload boots + reports 9.9.9)"
# The helper's relaunch (execs $APPIMAGE) fires the unrunnable type2 runtime in
# this sandbox; prove the swapped-in bytes are a bootable B by booting its
# payload the same way A was booted.
extract_payload "$target" "$work/payloadB"
export DAEMON_APP_UPDATE_E2E_LOG="$work/e2e-B.log"
: > "$DAEMON_APP_UPDATE_E2E_LOG"
timeout 120 "$work/payloadB/usr/bin/daemon-app" </dev/null >"$work/runB.log" 2>&1 || true
say "B E2E sentinel log"
cat "$work/e2e-B.log" 2>/dev/null || echo "(none)"
grep -q 'boot version=9.9.9' "$work/e2e-B.log" || { echo "FAIL: swapped-in B did not boot / report 9.9.9" >&2; exit 1; }
echo "PASS"

[ "$ok" = 1 ] || { echo "FAIL: swap did not converge in time" >&2; exit 1; }

say "AppImage SelfApply A->B swap E2E: PASS"
