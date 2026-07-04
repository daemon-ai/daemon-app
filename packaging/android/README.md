<!--
SPDX-FileCopyrightText: 2026 Jarrad Hope
SPDX-License-Identifier: MPL-2.0
-->

# Android packaging (arm64-v8a)

The Android build of daemon-app is a **thin remote client**, exactly like the
WASM build: it renders node state and sends intents over the WebSocket
transport. **No daemon is bundled** - Android has no sensible way to host the
Rust node as a subprocess of an app (background execution limits, no exec of
extracted binaries under the default W^X policy on modern API levels), and
store-distributed builds must not self-update anyway. Per
`packaging/UPDATES.md`, app-store artifacts run with update capability
**`None`**: the platform owns delivery and updates.

## Building

### Primary: hermetic Nix build (no Gradle, no network)

```sh
nix build .#apk                            # -> result/daemon-app-<version>-arm64-v8a.apk
nix build .#checks.x86_64-linux.apk-sanity # structural gate over the APK
```

Everything is pinned by the flake: Qt 6.11 for Android is cross-compiled from
the same nixpkgs qt6 sources as the desktop stack (`nix/qt-from-source.nix`,
flavor `android-arm64`; consumer file `nix/android.nix`), the SDK/NDK come
from `androidenv.composeAndroidPackages` on the same nixpkgs pin (build-tools
35.0.0, platform 35, NDK 27.2.12479018 - unfree, accepted via a second
nixpkgs import scoped to the android outputs), and the APK is assembled
WITHOUT Gradle:

1. the app cross-builds against the android Qt through `qt-cmake`
   (`libdaemon-app_arm64-v8a.so` + `android-deployment-settings.json`);
2. `androiddeployqt --aux-mode` stages the full package tree (Qt libs +
   scanned QML plugins into `libs/`, the compiled resource bundle
   `assets/android_rcc_bundle.rcc`, this directory overlaid over Qt's
   template, manifest/`libs.xml` placeholders filled) and stops before
   Gradle - Gradle's first run downloads AGP/androidx from Maven, which the
   sandbox forbids;
3. the classic toolchain finishes the job: `javac` (Qt's
   QtActivity/QtApplication bindings against `android.jar` + the staged Qt
   jars) -> `d8` -> `aapt2 compile/link` -> zip in `classes.dex`,
   `lib/arm64-v8a/*.so`, `assets/` (the `.rcc` stored uncompressed - Qt maps
   it straight out of the APK) -> `zipalign` -> `apksigner`.

Intermediate outputs for debugging each layer: `.#qt-android` (joined Qt
prefix), `.#qtbase-android` (+ per-module outputs), `.#android-staged` (the
androiddeployqt-staged package tree).

### Fallback: Gradle dev shell (network required)

```sh
nix develop .#android
./scripts/build-apk-gradle.sh   # configure + build + androiddeployqt --gradle
```

Uses the same pinned Qt/SDK/NDK but lets androiddeployqt run the stock Gradle
build (first run resolves AGP + androidx from Maven into `~/.gradle`). Use
this when you need what Gradle uniquely provides (e.g. the androidx
FileProvider wiring, AAB bundles, Play-style resource shrinking).

## Signing: debug vs release

The Nix APK is signed with an **ephemeral debug key generated during the
build** (`keytool -genkeypair`, CN=Android Debug): every rebuild produces a
differently-signed artifact. That is fine for sideloading and emulator use;
it is NOT a distribution artifact - Android updates require stable signatures.

Release signing is **documented as hooks, deliberately not implemented**: a
store rollout would need a real keystore in CI secrets. `apksigner` accepts
the standard environment-shaped inputs, so re-sign the built APK like

```sh
apksigner sign \
  --ks "$DAEMON_ANDROID_KEYSTORE" \
  --ks-key-alias "$DAEMON_ANDROID_KEY_ALIAS" \
  --ks-pass env:DAEMON_ANDROID_KS_PASS --key-pass env:DAEMON_ANDROID_KEY_PASS \
  --out daemon-app-release.apk result/daemon-app-*-arm64-v8a.apk
```

(androiddeployqt's Gradle path reads `QT_ANDROID_KEYSTORE_PATH`,
`QT_ANDROID_KEYSTORE_ALIAS`, `QT_ANDROID_KEYSTORE_STORE_PASS`,
`QT_ANDROID_KEYSTORE_KEY_PASS` with `--sign --release` if you go through the
dev shell instead.) Keys are never committed; generate with
`keytool -genkeypair` and park them in the release CI secret store, mirroring
the minisign key handling in `packaging/UPDATES.md`.

## Version stamping

Same single source of truth as every other artifact - the top-level `VERSION`
file:

* **versionName** = the flake's `versionStr`
  (`<VERSION>+g<shortRev>[.dirty]`), passed as `DAEMON_APP_VERSION_STR` and
  applied via the `QT_ANDROID_VERSION_NAME` target property.
* **versionCode** = `major * 10000 + minor * 100 + patch` (e.g. `0.1.3` ->
  `103`), computed in `src/DaemonApp/App/CMakeLists.txt` from
  `PROJECT_VERSION`. Monotonic as long as minor/patch stay < 100, which
  `just set-version`'s SemVer gate enforces; a future scheme change only has
  to keep the integer growing.

## Layout of this directory

* `AndroidManifest.xml` - QtActivity-based manifest, application id
  `io.daemon.app`, minSdk 28 / targetSdk 35. Overlaid onto Qt's package
  template by androiddeployqt (`QT_ANDROID_PACKAGE_SOURCE_DIR`); the
  `%%INSERT_*%%` markers are filled from the deployment settings.
* `res/mipmap-*/ic_launcher.png` - launcher icons, generated from
  `packaging/linux/icons/daemon-app.svg` by
  `packaging/linux/icons/regen.sh` (committed, like the Linux rasters, so
  packaging never rasterizes at build time).

## Known caveats (debug artifact)

* **No TLS (`wss://`)**: Qt-for-Android needs a bundled OpenSSL pair
  (upstream ships it via the separate `android_openssl` repo); the pinned
  stack builds without it, so only `ws://` transports work. Bundle
  libssl/libcrypto for arm64 before any real rollout.
* **No androidx FileProvider**: the stock Qt manifest's `<provider>` block is
  dropped (androidx.core exists only in the Gradle path). Share-to-other-apps
  and open-in intents are not wired; the app itself does not use them.
* The x86_64 ABI (emulator-native) is not built; arm64-v8a runs on emulators
  via ARM translation or on any physical device.
