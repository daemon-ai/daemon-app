# macOS packaging — build / sign / notarize runbook

> **VALIDATED (ad-hoc / unsigned lane) on aarch64-darwin.** The pure-nix
> `nix build .#macos-dmg` path builds a working, ad-hoc-signed DMG on an
> Apple-Silicon mac (macOS 26, Xcode 26.5, Nix 2.34). The bundle launches
> headless through the `DAEMON_APP_WAIT_READY` contract (mock mode), and the
> superproject `package-dmg` output co-locates the Rust node binaries next to
> the app on `LocalDaemonLauncher`'s discovery path. What is proven vs. still
> open:
>
> **Proven on darwin**
> - CMake configure + `MACOSX_BUNDLE` build + generated `Info.plist` (Qt 6.11
>   from nixpkgs, the vendored deps, and the `qmltermwidget` qmake build).
> - The Qt deploy script (`qt_generate_deploy_qml_app_script` → `macdeployqt`)
>   and `cpack -G DragNDrop`. `hdiutil` **does** run inside the nix build
>   sandbox on this host (sandboxing is disabled for the daemon build user;
>   the flake appends `/usr/bin:/usr/sbin` to PATH for the packaging step).
> - `org.kde.syntaxhighlighting` deploys correctly (`libKF6SyntaxHighlighting`
>   in `Frameworks/`, `libkquicksyntaxhighlightingplugin` in `PlugIns/`).
> - The offscreen QPA plugin and the `QMLTermWidget` module are deployed into
>   the bundle with their store refs rewritten (see the two caveats below).
> - `codesign --verify --deep --strict` passes on the (ad-hoc) sealed bundle,
>   including the co-located Rust binaries (each carries the arm64 linker's
>   own ad-hoc signature).
> - Co-location: `daemon` / `daemon-cli` / `daemon-infer` ship in
>   `Contents/MacOS/`, are arm64, executable, and run standalone (`--version`),
>   with no dangling `/nix/store` load commands (see the libiconv note under
>   "Payload notes"). `LocalDaemonLauncher::discoverDaemonBinary` probes
>   `applicationDirPath()/daemon`, i.e. exactly where they land.
> - Retina `.icns` regenerated with `iconutil` (16/32/128/256/512 + `@2x` +
>   1024, rasterized from the scalable SVG source).
>
> **Still open (untested)**
> - Every **Developer ID** signing / notarization / stapling step below (the
>   ad-hoc `codesign --sign -` path is what is validated; a real identity,
>   `notarytool`, and `stapler` have not been exercised).
> - `entitlements.plist` (not authored yet — needed at first real signing).
> - DMG Finder background/layout polish.

## Prerequisites

- A mac (Apple Silicon or Intel) running macOS 13+ (the app's
  `LSMinimumSystemVersion`, per Qt 6.11's supported-platforms floor).
- Xcode or the Command Line Tools for `hdiutil`, `codesign`, `spctl`,
  `stapler`. Notarization needs `notarytool`, which ships with full Xcode
  13+ — verify with `xcrun --find notarytool` before relying on a bare CLT
  install.
- Nix (multi-user install) with flakes enabled. There are NO host build
  tools in this repo's flow: CMake/Ninja/Qt all come from the flake.
- For signing: a **Developer ID Application** certificate in the login
  keychain; for notarization: an App Store Connect API key or app-specific
  password wired into a `notarytool` keychain profile:

```sh
xcrun notarytool store-credentials daemon-notary \
  --apple-id <apple-id> --team-id <team-id> --password <app-specific-pw>
```

## Build

Preferred (mirrors `nix build .#artifacts` on Linux):

```sh
nix build .#macos-dmg
# -> result/daemon-<version>-macos-<arch>.dmg (+ .sha256)
```

`packages.macos-dmg` exists only on the darwin systems
(`aarch64-darwin` / `x86_64-darwin`). It configures the app with relative
install dirs + the packaging flags and runs `cpack -G DragNDrop`. The darwin
artifact derivation also sets `CCACHE_DISABLE=1` (the base stdenv is
`ccacheStdenv`, whose wrapper wants a `/var/cache/ccache` that a fresh mac
lacks) and appends `/usr/bin:/usr/sbin` to PATH in the install phase so the
deploy script and DragNDrop generator find `hdiutil` / `codesign` / `SetFile`.

**For the shippable product, build the superproject DMG** — it injects the
Rust node binaries next to the app so managed local-daemon spawn works:

```sh
# in the superproject (daemon), on a mac:
just package-dmg
# == nix build '.?submodules=1#package-dmg' --accept-flake-config
# -> result-package-dmg/daemon-<version>-macos-<arch>.dmg
```

**`hdiutil` in the sandbox:** on this build host `hdiutil` runs fine inside
the nix build (the daemon build user has sandboxing disabled, and the flake
puts the system bindirs on PATH for the packaging step). On a host where the
sandbox blocks `/usr/bin`, fall back to the manual unsandboxed flow:

```sh
nix develop
cmake -B build-macos -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF \
  # optional co-located sibling binaries (see below):
  # -DDAEMON_APP_BUNDLED_DAEMON=/path/to/daemon \
  # -DDAEMON_APP_BUNDLED_DAEMON_INFER=/path/to/daemon-infer \
  # -DDAEMON_APP_BUNDLED_DAEMON_CLI=/path/to/daemon-cli
cmake --build build-macos
(cd build-macos && cpack -G DragNDrop)
```

(or `nix build .#macos-dmg --option sandbox false` at the cost of purity).

## What the DMG contains (theoretical — verify on first run)

```
daemon-<version>-macos-<arch>.dmg        # UDZO, volume name "Daemon"
└── daemon-app.app                       # + /Applications symlink (CPack default)
    └── Contents/
        ├── Info.plist                   # from packaging/macos/Info.plist.in
        │                                #   io.daemon.app, LSMinimumSystemVersion 13.0
        ├── MacOS/
        │   ├── daemon-app               # the Qt Quick GUI
        │   ├── daemon                   # DAEMON_APP_BUNDLED_DAEMON (if set)
        │   ├── daemon-infer             # DAEMON_APP_BUNDLED_DAEMON_INFER (if set)
        │   └── daemon-cli               # DAEMON_APP_BUNDLED_DAEMON_CLI (if set)
        ├── Frameworks/                  # Qt frameworks + vendored dylibs
        │                                #   (incl. libKF6SyntaxHighlighting)
        ├── PlugIns/                     # Qt plugins (+ QML plugin dylibs)
        │   └── platforms/
        │       ├── libqcocoa.dylib      # deploy script (the build QPA)
        │       └── libqoffscreen.dylib  # backstop-deployed (headless boot)
        └── Resources/
            ├── daemon-app.icns          # retina iconset (iconutil)
            ├── qt.conf                  # written by the deploy script
            ├── qml/                     # deployed QML imports
            │   ├── org/kde/…            # org.kde.syntaxhighlighting (deploy)
            │   └── QMLTermWidget/        # backstop-deployed (embedded terminal)
            ├── microtex-res/            # MicroTeX fonts + XML (see caveat)
            ├── LICENSE
            └── THIRD-PARTY-NOTICES.md
```

`LocalDaemonLauncher` discovers the sibling binaries co-located with the app
executable (`QCoreApplication::applicationDirPath()` == `Contents/MacOS`),
identical to the Linux `bin/` layout — no wrapper scripts. The superproject
`package-dmg` fills these via the `DAEMON_APP_BUNDLED_*` cache vars (daemon +
daemon-cli + the llama-enabled daemon-infer, matching the Linux bundle).

**Co-location + signing order (ad-hoc lane):** the `install(PROGRAMS …)` rules
that copy the Rust binaries into `Contents/MacOS` are registered *before* the
deploy script and the backstop, so at install time the binaries are in place
before `macdeployqt` / the backstop re-seal the outer bundle. Each Rust binary
keeps the arm64 linker's own ad-hoc signature (nested Mach-Os are not sealed by
the outer signature — they are validated independently), so
`codesign --verify --deep --strict` passes on the finished bundle. For the
notarized lane this order still holds, but each nested binary must additionally
be **hardened-runtime + Developer ID** signed *before* the outer seal — see
[Codesigning](#codesigning).

### Relocatability backstop — why the deploy step is not enough

nixpkgs builds Qt/KF6 with `CMAKE_INSTALL_NAME_DIR=$out/lib`, so the app
executable and the project-built plugins carry **absolute `/nix/store` install
names** for their directly-linked vendored dylibs. `macdeployqt` copies those
dylibs into `Frameworks/` and rewrites the *plugin's* reference, but it leaves
the **consumer's** own load command pointing at the dead store path — which
dangles the moment the `.app` leaves the store for the DMG, so the app aborts
at launch with a dyld "Library not loaded". `cmake/Packaging.cmake` runs a
post-deploy backstop (`install(CODE …)`, after `install(SCRIPT deploy)`) that
`otool -L`s the finished Mach-Os and `install_name_tool -change`s any residual
`/nix/store/…` reference to a bundle-relative path (`@rpath/…` for the exe,
`@loader_path/…/Frameworks/…` for the plugins), drops store `LC_RPATH`s so no
`@rpath` can fall back to the store Qt, then re-`codesign --sign -` the edited
files and re-seal the bundle. The same pattern deploys the offscreen plugin and
the QMLTermWidget module (below); it generalizes to any directly-linked
vendored dylib.

### Payload notes (validated on darwin)

- **MicroTeX resources**: the bundle ships `Contents/Resources/microtex-res`,
  and `microtexResDir()` in `src/DaemonApp/App/application.cpp` probes
  `applicationDirPath()/../Resources/microtex-res` at startup (after the env
  override and the shared-prefix layouts), so math rendering resolves the
  bundled copy on any machine. **Confirmed present** in the DMG.
- **Offscreen QPA plugin**: `macdeployqt` deploys only the build's QPA
  (`libqcocoa`), which needs a window server. The backstop additionally copies
  `libqoffscreen.dylib` into `Contents/PlugIns/platforms/` (Qt refs rewritten
  to `@loader_path/../../Frameworks/…`) so `QT_QPA_PLATFORM=offscreen` — the
  headless boot contract, and any display-less launch — works from the bundle.
  Pointing `QT_QPA_PLATFORM_PLUGIN_PATH` at the store Qt instead loads a second
  Qt binary set (two `QtCore`) and crashes, hence the in-bundle deploy.
- **QMLTermWidget** (embedded terminal): supplied via the wrapper's QML import
  path on Linux; `macdeployqt` does not discover it. **This is not cosmetic** —
  `Main.qml` imports it unconditionally (`Session` → `TerminalPanel` →
  `import QMLTermWidget`), so with the module absent `QQmlApplicationEngine`
  fails to load the *root* component and the `.app` does not launch **at all**
  (GUI or headless). The backstop therefore deploys the module under
  `Resources/qml/QMLTermWidget/` (plugin dylib refs rewritten to
  `@loader_path/../../../Frameworks/…`). Terminal functionality itself is
  otherwise the same as on Linux.
- **`org.kde.syntaxhighlighting`**: **answered — it deploys.** The deploy
  script pulls `libKF6SyntaxHighlighting.6.dylib` into `Frameworks/` and
  `libkquicksyntaxhighlightingplugin.dylib` into `PlugIns/`, and
  `Resources/qml/org/kde/…` exists in the bundle.

## Codesigning

Hardened runtime is mandatory for notarization. Two rules dominate:

1. **Sign inside-out.** Nested code (the bundled Rust binaries, every
   framework/plugin dylib) must be signed before the outer bundle seal.
   `codesign --deep` is deprecated/unreliable for this — it misses non-code
   directories and signs with the wrong entitlements; use it only for
   *verification*.
2. **Hardened runtime blocks what the app spawns and loads.** The GUI
   launches `Contents/MacOS/daemon` / `daemon-infer` as child processes:
   unsigned (or team-mismatched) children are killed on launch under
   hardened runtime, so **each bundled binary needs its own
   hardened-runtime signature** with the same Developer ID.

```sh
IDENTITY="Developer ID Application: <name> (<team id>)"
APP=daemon-app.app   # the bundle inside the mounted/staged tree

# 1. Bundled sibling binaries first (skip those not bundled):
for bin in daemon daemon-infer daemon-cli; do
  [ -f "$APP/Contents/MacOS/$bin" ] || continue
  codesign --force --options runtime --timestamp \
    --sign "$IDENTITY" "$APP/Contents/MacOS/$bin"
done

# 2. Qt frameworks + plugins (skip if macdeployqt already signed them via
#    DEPLOY_TOOL_OPTIONS -hardened-runtime -codesign=..., see
#    cmake/Packaging.cmake):
find "$APP/Contents/Frameworks" "$APP/Contents/PlugIns" \
     -name '*.dylib' -o -name '*.framework' | while read -r f; do
  codesign --force --options runtime --timestamp --sign "$IDENTITY" "$f"
done

# 3. The outer bundle (seals Resources + Info.plist):
codesign --force --options runtime --timestamp \
  --entitlements packaging/macos/entitlements.plist \
  --sign "$IDENTITY" "$APP"
```

### Entitlements note (file not committed yet — author on first signing run)

The v1 payload is Nix-linked: the app and the Rust binaries may `dlopen` /
link dylibs from `/nix/store`, which are **not** signed by your team. Under
hardened runtime + library validation that load is refused. Expect to need,
at minimum:

```xml
<key>com.apple.security.cs.disable-library-validation</key><true/>
```

and possibly `com.apple.security.cs.allow-dyld-environment-variables` (the
nix Qt wrapper's env vars) on the *app*; `daemon-infer` may additionally
need `com.apple.security.cs.allow-jit` if its inference backend JIT-compiles.
Grant per-binary, only what each one demonstrably needs — every entitlement
weakens the hardened runtime story. The static/portable payload workstream
should remove the library-validation exemption.

The `DEPLOY_TOOL_OPTIONS` hook in `cmake/Packaging.cmake` shows where
`-hardened-runtime -codesign=<identity>` slot into the *deploy-time* flow
(macdeployqt signs what it deploys); the bundled Rust binaries are outside
its scope either way and always need step 1 above. Signing at deploy time
also means cpack re-runs it when staging the DMG.

## Notarization + stapling

Notarize the finished DMG (covers the nested .app):

```sh
xcrun notarytool submit daemon-*-macos-*.dmg \
  --keychain-profile daemon-notary --wait
xcrun stapler staple daemon-*-macos-*.dmg
```

On rejection, fetch the log for per-binary findings (unsigned nested code,
missing hardened runtime, invalid timestamps):

```sh
xcrun notarytool log <submission-id> --keychain-profile daemon-notary
```

## Verification

```sh
codesign --verify --deep --strict --verbose=2 daemon-app.app
spctl --assess --type exec --verbose daemon-app.app          # Gatekeeper, app
spctl --assess --type open --context context:primary-signature \
  --verbose daemon-*-macos-*.dmg                             # Gatekeeper, dmg
stapler validate daemon-*-macos-*.dmg
# launch check without Finder quarantine surprises:
xattr -p com.apple.quarantine daemon-app.app 2>/dev/null || echo "no quarantine"
```

Also boot the headless `DAEMON_APP_WAIT_READY` contract the other platforms
gate on, from the mounted/installed bundle, in both service modes. The offscreen
plugin (above) makes this work without a display; the marker on stdout is
`DAEMON_APP_READY ok`.

```sh
APP=daemon-app.app/Contents/MacOS/daemon-app

# Mock mode: no daemon needed — the WAIT_READY probe drives a first-run "local"
# connect the mock accepts once the socket file exists.
DAEMON_APP_SERVICE_MODE=mock DAEMON_APP_SOCKET=/tmp/mock.sock \
DAEMON_APP_WAIT_READY=20000 QT_QPA_PLATFORM=offscreen \
  sh -c 'touch "$DAEMON_APP_SOCKET"; "'"$APP"'"'   # expect "DAEMON_APP_READY ok"

# Daemon mode against the BUNDLED node: LocalDaemonLauncher discovers
# Contents/MacOS/daemon next to the app and spawns it.
DAEMON_APP_SERVICE_MODE=daemon DAEMON_APP_WAIT_READY=30000 \
QT_QPA_PLATFORM=offscreen "$APP"                    # interactive login only, see note
```

> **Headless daemon-mode caveat (macOS).** The daemon-mode connect reads the
> saved connection token from the OS keychain
> (`QtSettingsStore::connectionToken` → qtkeychain →
> `configureTransport`), which spins a nested event loop until the Security
> framework answers. In a **headless SSH session** with a locked/inaccessible
> login keychain, the app's own keychain-access prompt can never be answered,
> so this call blocks and the daemon-mode probe hangs *before* the daemon is
> spawned. This is an environment limitation, **not** a co-location or
> packaging defect — mock mode (which reads no token) boots headless fine, the
> co-located binaries run standalone, and the discovery path is
> `applicationDirPath()/daemon`. On an interactive login (unlocked keychain, or
> once the first keychain-access prompt is approved) daemon mode spawns and
> connects to the co-located daemon normally. To exercise daemon mode over SSH,
> unlock the login keychain first (`security unlock-keychain`) and grant the
> app keychain access, or run from a logged-in GUI session.

## Updates

The DMG artifact's update capability is **DownloadAndOpen** (fetch + verify
the feed, download the new DMG, hand it to the OS; the user completes the
install) — in-place `.app` swaps need quarantine/codesign care and stay out
of v1. The capability dial, signed `manifest.json` feed, and rollout design
land separately on the `pkg/release-feed` branch as `packaging/UPDATES.md`;
this DMG plugs into that design unchanged.

## Follow-ups tracked here

- [x] `microtexResDir()` bundle probe (`../Resources/microtex-res` candidate
      in `application.cpp`, landed at integration).
- [x] Retina `.icns` (`iconutil`, 16/32/128/256/512 + `@2x` + 1024 from the
      SVG source) — replaces the `png2icns` bonus output.
- [x] Offscreen QPA plugin deployed into the bundle (headless boot works).
- [x] `QMLTermWidget` module deployed into the bundle (the app now launches
      from a DMG install; it was a hard boot blocker, not a dead-panel caveat).
- [ ] `entitlements.plist` authored + committed after the first signing run.
- [ ] DMG Finder layout: `dmg-background.png` + `.DS_Store` setup script
      (commented hooks in `cmake/CPackPerGen.cmake`).
- [ ] Developer ID signing + notarization + stapling (only the ad-hoc lane is
      validated so far).
- [ ] Non-Nix-linked (static-Qt) macOS payload — removes the dynamic-Qt
      `/nix/store` frameworks and the library-validation entitlement; the app +
      node executables already have zero dangling store refs.
