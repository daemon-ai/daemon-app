#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# macOS DMG SelfApply end-to-end test (validated on the M1). Run it from the
# daemon-app repo root INSIDE the dev shell so cmake/ninja/minisign/macdeployqt
# and $QMLTERMWIDGET_QML_DIR are on the environment:
#
#     nix develop --command bash packaging/macos/e2e-selfapply.sh
#
# It builds two ad-hoc-signed SelfApply DMGs with a THROWAWAY test minisign
# pubkey and versions 0.9.0 (A) + 0.9.1 (B), publishes a minisign-signed feed for
# B on loopback, installs A into a user-writable dir, runs it headless (the
# env-gated DAEMON_APP_UPDATE_E2E auto-drive in UpdateManager, shared with the
# AppImage/Windows lanes: check -> download -> self-apply), and asserts the
# daemon-updater two-move swap + relaunch to B. The version bumps are throwaway
# VERSION edits, restored after each build; nothing is committed. It NEVER
# touches the production minisign key.
set -uo pipefail

REPO="$(cd "$(dirname "$0")/../.." && pwd)"
W="${E2E_WORK:-$HOME/work/upd-e2e}"
PORT="${E2E_PORT:-8099}"
VA=0.9.0
VB=0.9.1

fail() {
    echo "E2E FAIL: $*" >&2
    exit 1
}

command -v minisign >/dev/null || fail "run inside 'nix develop' (minisign missing)"
[ -n "${QMLTERMWIDGET_QML_DIR:-}" ] ||
    fail "QMLTERMWIDGET_QML_DIR unset — run inside 'nix develop' (needed for the embedded-terminal deploy)"
ECM="$(dirname "$(find /nix/store -maxdepth 6 \
    -path '*extra-cmake-modules*/share/ECM/cmake/ECMConfig.cmake' 2>/dev/null | head -1)")"
[ -n "$ECM" ] && [ -d "$ECM" ] || fail "could not locate extra-cmake-modules (ECM_DIR)"

mkdir -p "$W"
rm -rf "$W/feed" "$W/Install" "$W"/e2e-*.log "$W/helper.log" "$W/parked-old.txt"
mkdir -p "$W/feed" "$W/Install"

# --- throwaway test key (generated once, reused) --------------------------
if [ ! -f "$W/test.key" ]; then
    printf '\n\n' | minisign -G -f -p "$W/test.pub" -s "$W/test.key" >/dev/null 2>&1 ||
        fail "minisign keygen failed"
fi
PUB="$(tail -1 "$W/test.pub")"

# --- build one SelfApply DMG for a version --------------------------------
# A FRESH build dir per version sidesteps a CMake quirk where an in-place
# reconfigure does not regenerate the bundle Info.plist after a VERSION change.
build_dmg() {
    local V="$1" BD="$REPO/build-e2e-$1"
    (
        cd "$REPO"
        echo "$V" >VERSION
        cmake -B "$BD" -G Ninja -DBUILD_TESTING=OFF \
            -DDAEMON_APP_UPDATE_CAPABILITY=SelfApply \
            -DDAEMON_APP_UPDATE_PUBKEY="$PUB" \
            -DDAEMON_APP_UPDATE_ARTIFACT_KIND=dmg \
            -DDAEMON_APP_UPDATE_FEED_URL= \
            -DCMAKE_INSTALL_BINDIR=bin -DCMAKE_INSTALL_LIBDIR=lib \
            -DCMAKE_INSTALL_DATADIR=share -DCMAKE_INSTALL_DOCDIR=share/doc/daemon-app \
            -DECM_DIR="$ECM" -DQMLTERMWIDGET_QML_DIR="$QMLTERMWIDGET_QML_DIR" >/dev/null ||
            exit 1
        cmake --build "$BD" >/dev/null || exit 1
        (cd "$BD" && PATH="$PATH:/usr/bin:/usr/sbin" cpack -G DragNDrop >/dev/null) || exit 1
        git checkout VERSION
    ) || fail "build $V failed"
    cp -f "$REPO/build-e2e-$1"/daemon-*-macos-*.dmg "$W/daemon-$1.dmg"
    # A shipped DMG never has its build tree around; here it does, and the binary
    # bakes DAEMON_APP_BUILD_QML_DIR at it — loading a second (store) Qt. Drop the
    # tree so the run matches a real deployed bundle.
    chmod -R u+w "$BD" 2>/dev/null
    rm -rf "$BD" 2>/dev/null
}

echo "== building A ($VA) and B ($VB) =="
build_dmg "$VA"
build_dmg "$VB"

# --- signed feed for B ----------------------------------------------------
cp "$W/daemon-$VB.dmg" "$W/feed/daemon-$VB.dmg"
# Absolute /usr/bin: inside the dev shell nix coreutils shadow `stat`/`shasum`
# (GNU `stat -f` means --file-system, not BSD's byte size).
SZ=$(/usr/bin/stat -f%z "$W/feed/daemon-$VB.dmg")
SHA=$(/usr/bin/shasum -a 256 "$W/feed/daemon-$VB.dmg" | cut -d' ' -f1)
cat >"$W/feed/manifest.json" <<JSON
{"schema":1,"product":"daemon","channel":"stable","version":"$VB","released":"2026-07-05T00:00:00Z","notesUrl":"","artifacts":[{"kind":"dmg","os":"macos","arch":"aarch64","file":"daemon-$VB.dmg","size":$SZ,"sha256":"$SHA","updateCapability":"SelfApply"}]}
JSON
printf '\n' | minisign -Sm "$W/feed/manifest.json" -s "$W/test.key" \
    -t "daemon stable $VB" -c "e2e" >/dev/null 2>&1 || fail "minisign sign failed"

# --- install A into a user-writable dir -----------------------------------
MP=$(hdiutil attach -nobrowse -noverify -readonly "$W/daemon-$VA.dmg" | grep -o '/Volumes/.*' | head -1)
ditto "$MP/daemon-app.app" "$W/Install/daemon-app.app"
hdiutil detach "$MP" >/dev/null
APP="$W/Install/daemon-app.app"
xattr -dr com.apple.quarantine "$APP" 2>/dev/null || true

# --- serve the feed + capture the transient staging -----------------------
(cd "$W/feed" && exec python3 -m http.server --bind 127.0.0.1 "$PORT") >/dev/null 2>&1 &
SRV=$!
(
    for _ in $(seq 1 600); do
        for d in "$W/Install/.daemon-app-update-"*; do
            [ -d "$d" ] || continue
            [ -f "$d/daemon-updater.log" ] && cp -f "$d/daemon-updater.log" "$W/helper.log" 2>/dev/null
            [ -d "$d/old-bundle.app" ] &&
                plutil -extract CFBundleShortVersionString raw \
                    "$d/old-bundle.app/Contents/Info.plist" >"$W/parked-old.txt" 2>/dev/null
        done
        sleep 0.05
    done
) &
CAP=$!
trap 'kill $SRV $CAP 2>/dev/null' EXIT
sleep 1

# --- run A headless (shared UpdateManager auto-drive) ---------------------
# DAEMON_APP_UPDATE_E2E arms the in-manager auto-drive; DAEMON_APP_UPDATE_E2E_LOG
# collects "E2E boot/apply/uptodate version=" sentinels (A's log, then B's after
# the relaunch inherits the env). Launch with a CLEAN env (env -i): the dev shell
# exports Qt/DYLD/QML paths that would load a second (store) Qt beside the bundle
# Qt and SIGABRT the app — a real user launch has none of that. PATH=/usr/bin so
# the app's QProcess calls to hdiutil/ditto/xattr/plutil resolve.
echo "== run A (clean env) =="
/usr/bin/env -i HOME="$HOME" PATH=/usr/bin:/bin \
    DAEMON_APP_SERVICE_MODE=mock QT_QPA_PLATFORM=offscreen \
    DAEMON_APP_UPDATE_E2E=1 DAEMON_APP_UPDATE_E2E_LOG="$W/e2e-A.log" \
    DAEMON_APP_UPDATE_FEED_URL_OVERRIDE="http://127.0.0.1:$PORT/manifest.json" \
    "$APP/Contents/MacOS/daemon-app" >"$W/a.stdout" 2>&1
echo "A exited rc=$?"
cat "$W/e2e-A.log" 2>/dev/null

echo "== waiting for swap + relaunch to B =="
for _ in $(seq 1 60); do
    V=$(plutil -extract CFBundleShortVersionString raw "$APP/Contents/Info.plist" 2>/dev/null || echo '?')
    grep -q "E2E uptodate version=$VB" "$W/e2e-A.log" 2>/dev/null && [ "$V" = "$VB" ] && break
    sleep 0.5
done
sleep 2
kill "$CAP" 2>/dev/null

# --- evidence + assertions -----------------------------------------------
echo "== EVIDENCE =="
echo "-- E2E sentinel log (A boot/apply, then relaunched B boot/uptodate) --"
cat "$W/e2e-A.log" 2>/dev/null
echo "-- helper log --"
sed 's/^/    /' "$W/helper.log" 2>/dev/null
echo "-- parked-old version --"
cat "$W/parked-old.txt" 2>/dev/null

echo "== ASSERTIONS =="
TGTV=$(plutil -extract CFBundleShortVersionString raw "$APP/Contents/Info.plist" 2>/dev/null || echo '?')
[ "$TGTV" = "$VB" ] || fail "target bundle version is $TGTV, want $VB"
echo "PASS: swap applied — target bundle Info.plist == $VB"

grep -q "E2E boot version=$VA" "$W/e2e-A.log" 2>/dev/null || fail "no A-boot sentinel"
grep -q "E2E apply version=$VB" "$W/e2e-A.log" 2>/dev/null || fail "no apply sentinel"
grep -q "E2E boot version=$VB" "$W/e2e-A.log" 2>/dev/null || fail "relaunched process did not boot as $VB"
grep -q "E2E uptodate version=$VB" "$W/e2e-A.log" 2>/dev/null || fail "relaunched $VB did not settle UpToDate"
echo "PASS: relaunched process is B — booted $VB and settled UpToDate against the feed"

grep -q 'move-staged ok' "$W/helper.log" 2>/dev/null || fail "helper log missing the two-move swap"
echo "PASS: helper performed the two-move swap"

[ "$(cat "$W/parked-old.txt" 2>/dev/null)" = "$VA" ] || fail "parked old bundle was not the $VA build"
echo "PASS: old $VA bundle parked aside by the helper"

if ls -d "$W/Install/.daemon-app-update-"* >/dev/null 2>&1; then fail "staging not cleaned on next start"; fi
echo "PASS: relaunched B swept the staging + parked old on next start"

if xattr -p com.apple.quarantine "$APP" >/dev/null 2>&1; then fail "quarantine xattr present"; fi
echo "PASS: no com.apple.quarantine on the swapped bundle"

echo "== E2E PASSED =="
