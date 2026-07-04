# macOS packaging — build / sign / notarize runbook

> **UNVALIDATED — CODE-ONLY WORKSTREAM.** Everything in this directory and the
> `if(APPLE)` branches it drives was authored and lint-/eval-checked on a
> Linux host. **No part of it has ever executed on macOS.** Specifically
> untested:
>
> - the CMake configure on darwin (Qt 6.11 from nixpkgs, vendored deps,
>   `qmltermwidget` qmake build);
> - the `MACOSX_BUNDLE` build + generated `Info.plist`;
> - the Qt deploy script run (`qt_generate_deploy_qml_app_script` →
>   `macdeployqt`), including whether the project-built QML modules
>   (`org.kde.syntaxhighlighting`) get deployed into the bundle;
> - `cpack -G DragNDrop` (and whether `hdiutil` is reachable inside the
>   darwin nix build sandbox at all — see [Build](#build));
> - every signing / notarization / stapling step below;
> - the `.icns` (generated on Linux by `packaging/linux/icons/regen.sh` via
>   `png2icns`: 16/32/128/256/512 px, **no retina `@2x` variants, no
>   1024 px** — regenerate with `iconutil` on a mac for release quality).
>
> Treat the first darwin run as a validation pass of this document, and fix
> the code where reality disagrees.

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
install dirs + the packaging flags and runs `cpack -G DragNDrop`.

**Known risk:** CPack's DragNDrop generator shells out to `hdiutil`
(`/usr/bin/hdiutil`), which the darwin nix *build sandbox* may not expose.
If the sandboxed build fails there, fall back to the manual flow (the
devShell runs unsandboxed):

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
        ├── Frameworks/                  # Qt frameworks (deploy script)
        ├── PlugIns/                     # Qt plugins (+ QML plugin dylibs)
        └── Resources/
            ├── daemon-app.icns
            ├── qt.conf                  # written by the deploy script
            ├── qml/                     # deployed QML imports
            ├── microtex-res/            # MicroTeX fonts + XML (see caveat)
            ├── LICENSE
            └── THIRD-PARTY-NOTICES.md
```

`LocalDaemonLauncher` discovers the sibling binaries co-located with the app
executable (`QCoreApplication::applicationDirPath()` == `Contents/MacOS`),
identical to the Linux `bin/` layout — no wrapper scripts.

### v1 payload caveats (same class as the Linux artifacts)

- **MicroTeX resources**: the binary on this branch still reads the baked
  compile-time `MICROTEX_RES_DIR` (a /nix/store path on the *build* machine).
  The bundle ships `Contents/Resources/microtex-res`, but the runtime probe
  that prefers it lands with the `pkg/size-wins` branch.
  **TODO(after pkg/size-wins):** add an
  `applicationDirPath()/../Resources/microtex-res` candidate to
  `microtexResDir()` in `src/DaemonApp/App/application.cpp` (mirroring the
  CMake-side TODO in `cmake/Packaging.cmake`). Until then, math rendering
  only works on machines that hold the builder's store path.
- **QMLTermWidget** (embedded terminal) is provided at runtime via the nix
  wrapper's import path on Linux; nothing deploys it into the bundle. The
  terminal panel will fail to load from a DMG install until the portable
  payload workstream covers it.
- The Qt deploy script should pick up the project-built
  `org.kde.syntaxhighlighting` QML module; **verify** the transcript code
  blocks highlight from a DMG install, and check `Resources/qml/org/kde/…`
  exists in the deployed bundle.

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

Also boot the headless contract the other platforms gate on:

```sh
DAEMON_APP_SERVICE_MODE=mock DAEMON_APP_SOCKET=/tmp/mock.sock \
DAEMON_APP_WAIT_READY=8000 QT_QPA_PLATFORM=offscreen \
  daemon-app.app/Contents/MacOS/daemon-app   # expect "DAEMON_APP_READY ok"
```

## Updates

The DMG artifact's update capability is **DownloadAndOpen** (fetch + verify
the feed, download the new DMG, hand it to the OS; the user completes the
install) — in-place `.app` swaps need quarantine/codesign care and stay out
of v1. The capability dial, signed `manifest.json` feed, and rollout design
land separately on the `pkg/release-feed` branch as `packaging/UPDATES.md`;
this DMG plugs into that design unchanged.

## Follow-ups tracked here

- [ ] `microtexResDir()` bundle probe after `pkg/size-wins` merges (TODO in
      `cmake/Packaging.cmake`).
- [ ] `entitlements.plist` authored + committed after the first signing run.
- [ ] Retina `.icns` (`iconutil` on a mac; replace the `png2icns` bonus
      output).
- [ ] DMG Finder layout: `dmg-background.png` + `.DS_Store` setup script
      (commented hooks in `cmake/CPackPerGen.cmake`).
- [ ] QMLTermWidget + portable (non-Nix-linked) payload — static-qt
      workstream.
