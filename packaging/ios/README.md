<!--
SPDX-FileCopyrightText: 2026 Jarrad Hope
SPDX-License-Identifier: MPL-2.0
-->

# daemon-app on iOS (simulator)

Exploratory, **simulator-only, unsigned** bring-up of daemon-app as a thin
remote client on iOS. This is the same posture as the WebAssembly and Android
builds: the app renders node state and talks to `daemon-node` over the
WebSocket mux; it does not (and on iOS cannot) spawn or probe a local daemon.

Device builds, code signing, provisioning profiles, and an App Store artifact
are **out of scope** here (see *Limitations* below).

## Why the Qt build is impure (unlike Android)

`nix/android.nix` is a fully pure cross derivation because `androidenv` puts the
NDK/SDK in the nix store. Apple ships the iPhoneSimulator SDK and its toolchain
**inside Xcode**, which is proprietary and not in nixpkgs — there is no
in-store equivalent. So `nix/ios.nix` pins everything that *is* pinnable (the
Qt + tinyxml2 **sources**, the host-tools prefix for `QT_HOST_PATH`, nix
`cmake`/`ninja`/`python3`) and hands them to a generated script
(`scripts/qt-ios-build.sh`, wrapped as the `qt-ios-builder` flake package) that
performs the actual build against the **system Xcode** via `xcrun` /
`DEVELOPER_DIR`. The produced Qt lands in a user cache dir, not the store.

The arm64-only simulator runtime on Apple-silicon Macs means Qt's prebuilt
x86_64 simulator libraries are useless anyway — building a static, trimmed Qt
from source targeting `arm64-simulator` is both required and desirable (same
philosophy as the wasm / linux-static / mingw flavors).

## Prerequisites

- An Apple-silicon Mac with **Xcode** (not just the Command Line Tools):
  `xcode-select -p` must point inside `Xcode.app`.
- The **iOS Simulator runtime** installed, in its **arm64 variant** — on an
  Apple-silicon host `xcodebuild -downloadPlatform iOS` installs the arm64
  runtime. Verify: `xcrun simctl list runtimes` shows an `iOS …` entry.
- Nix with flakes enabled.

## 1. Build static Qt for the iOS simulator

```sh
# Builds qtbase + qtshadertools + qtdeclarative(Basic-only) + qtsvg +
# qtwebsockets + a cross tinyxml2 into $HOME/.cache/daemon-app/qt-ios-sim-<ver>.
# ~2-4 h on an 8 GB M1; run under nohup/tmux and poll the log.
nix run .#qt-ios-builder

# Knobs (env): DAEMON_APP_QT_IOS_PREFIX (install dir),
#   DAEMON_APP_QT_IOS_JOBS (parallelism; cap at 4 on an 8 GB box),
#   DAEMON_APP_QT_IOS_FORCE=1 (rebuild a stage). The build is stamped per
#   module, so a re-run resumes where it stopped.
```

## 2. Build + boot daemon-app in the simulator

The whole configure → `xcodebuild` → `simctl` proof loop is encoded in
`scripts/ios-sim-smoke.sh`. Run it from the repo root inside the iOS devShell:

```sh
nix develop .#ios --command bash scripts/ios-sim-smoke.sh
```

That devShell exports `DAEMON_APP_QT_IOS` (the Qt prefix), the vendored-dep
source dirs, `ECM_DIR`, the host `katehighlightingindexer`, and the tinyxml2
`-isystem`/`-L` flags. The script:

1. **Configures** with the iOS `qt-cmake` and the Xcode generator:
   ```sh
   $DAEMON_APP_QT_IOS/bin/qt-cmake -S . -B build-ios -G Xcode \
     -DDAEMON_APP_TUI=OFF -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF \
     -DECM_DIR=$ECM_DIR -DKF_IGNORE_PLATFORM_CHECK=ON \
     -DKATEHIGHLIGHTINGINDEXER_EXECUTABLE=$DAEMON_APP_KATE_INDEXER \
     -D{MD4QT,EARCUT,KSYNTAXHIGHLIGHTING,MICROTEX}_SOURCE_DIR=... \
     -DCMAKE_CXX_FLAGS="$DAEMON_APP_IOS_CXX_FLAGS" \
     -DCMAKE_EXE_LINKER_FLAGS="$DAEMON_APP_IOS_LINKER_FLAGS"
   ```
2. **Builds** unsigned for the simulator:
   ```sh
   xcodebuild build -project build-ios/daemon-app.xcodeproj -scheme daemon-app \
     -configuration Release -sdk iphonesimulator \
     -destination 'generic/platform=iOS Simulator' CODE_SIGNING_ALLOWED=NO
   ```
3. **Boots** a simulator device, installs `daemon-app.app`, and launches it
   headlessly with the real boot contract. `simctl` passes environment to the
   app via the `SIMCTL_CHILD_` prefix (this Xcode's `simctl launch` has no
   `--env`):
   ```sh
   SIMCTL_CHILD_DAEMON_APP_SERVICE_MODE=mock \
   SIMCTL_CHILD_DAEMON_APP_SOCKET=/ \
   SIMCTL_CHILD_DAEMON_APP_WAIT_READY=30000 \
   xcrun simctl launch --console-pty --terminate-running-process booted ai.daemon.app
   # expect on stdout: DAEMON_APP_READY ok
   ```
   Mock service mode avoids needing a reachable node: `driveFirstRunConnect()`
   opens a `local` connection to `DAEMON_APP_SOCKET`, and the mock connection
   reports *ready* iff that path exists — so `DAEMON_APP_SOCKET=/` (always
   present in the sim) settles it without a real daemon. The WAIT_READY run
   prints the sentinel then exits; the script relaunches without WAIT_READY so
   the UI stays up for the screenshot.
4. **Screenshots** the booted app (`xcrun simctl io booted screenshot`) into
   `IOS_SHOT_DIR` (default `/tmp/ios-proof/`).

No code signature is required for the simulator (`CODE_SIGNING_ALLOWED=NO`).

## App-side gating (thin-client posture)

iOS reuses the Android/wasm seams — it is APPLE + `DAEMON_APP_MOBILE`:

- **Transport**: the WebSocket-mux stubs (`daemon_transport_stream_wasm.cpp` +
  `local_daemon_launcher_wasm.cpp`) are compiled; the connection service and
  the picker default to `remote-ws` and refuse other modes
  (`src/core/daemon/*`, `connection_dtos.h`).
- **Terminal**: `TerminalPanelStub.qml` stands in for the QMLTermWidget
  embedded terminal (a shared qmake plugin + native PTY, neither of which
  exists on a static iOS build) — same substitution as Android/wasm/static.
- **Static QML plugins**: everything is static, so `main.cpp`
  `Q_IMPORT_QML_PLUGIN`s the KSyntaxHighlighting plugin
  (`org_kde_syntaxhighlightingPlugin`) and `App/CMakeLists.txt` links it.
- **Resources**: MicroTeX's `res/` tree is staged into the (flat) `.app`
  bundle at `microtex-res/` via per-file `MACOSX_PACKAGE_LOCATION`;
  `microtexResDir()` finds it app-dir-relative.
- **Token store**: no qtkeychain on iOS, so the server-token store falls back
  to `QSettings` (same as the static/mobile builds).
- **Bundle id**: `ai.daemon.app` (matches the macOS/Android/AppStream id); the
  Info.plist is Qt's default iOS template.

## KSyntaxHighlighting × the Xcode "new build system"

Qt for iOS requires the CMake **Xcode generator** (`-G Xcode`). The vendored
KSyntaxHighlighting generates shared syntax files (`html-php.xml`, `jinja-*`)
and `index.katesyntax` through custom commands that are attached to several
targets at once. Ninja and Make (used by every other daemon-app flavor) dedup
that; the Xcode *new build system* (the only one Xcode 14+ ships) rejects it:

> The custom command generating … is attached to multiple targets … none of
> these is a common dependency of the other(s).

`scripts/ios-sim-smoke.sh` works around it **without touching the shared flake
input**: it copies the vendored KSyntaxHighlighting source to a writable dir,
adds the missing `add_dependencies(...)` (so the generator targets are a common
dependency of `katesyntax` / `SyntaxHighlightingData`), and points the configure
at the patched copy. If this is ever upstreamed to the pinned KSyntaxHighlighting
(or the flake input gains the patch), drop the workaround.

## Limitations / caveats

- **Simulator + unsigned only.** No device build, signing, provisioning, or
  App Store packaging. `ENABLE_MACOS_RELEASE`-style release wiring is not
  provided for iOS.
- **No local daemon.** A sandboxed `.app` cannot spawn/probe a local node, and
  the iOS Qt has no bundled OpenSSL — so only `ws://` remote connections work
  (a bonus `wss://` / real-node run is out of scope here). The headless smoke
  uses `DAEMON_APP_SERVICE_MODE=mock`.
- **No embedded terminal.** The QMLTermWidget panel is stubbed (see above).
- **Headless keychain.** Any real keychain path would block a headless boot on
  an unanswerable Secret-Service/Keychain prompt; mock mode + the QSettings
  fallback keep the smoke deterministic.
- **Impure build.** The Qt build is not reproducible via `nix build` — it
  depends on the host Xcode version. It is pinned only as far as the Qt/
  tinyxml2 sources and nix host tools go.

## Follow-ups

- A superproject `justfile` recipe wrapping `scripts/ios-sim-smoke.sh` (mirroring
  the other packaging recipes) is **descoped** for now — the superproject repo
  has an unrelated staged change awaiting a maintainer decision, so it was left
  untouched. Add the recipe there once that lands.
