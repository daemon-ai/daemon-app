#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# Fallback Android APK build through androiddeployqt's stock Gradle path.
# Run inside `nix develop .#android` (exports the pinned Qt-for-Android
# stack, SDK/NDK, JDK, and the vendored-dep source dirs). NEEDS NETWORK on
# the first run: Gradle resolves the Android Gradle Plugin + androidx from
# Maven into ~/.gradle. The hermetic (gradle-less) build is `nix build .#apk`;
# see packaging/android/README.md for when to prefer which.
#
#   nix develop .#android --command ./scripts/build-apk-gradle.sh [builddir]
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
builddir="${1:-$here/build/android-gradle}"

: "${DAEMON_APP_QT_ANDROID:?run inside nix develop .#android}"
: "${ANDROID_SDK_ROOT:?run inside nix develop .#android}"
: "${JAVA_HOME:?run inside nix develop .#android}"

"$DAEMON_APP_QT_ANDROID/bin/qt-cmake" -S "$here" -B "$builddir" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    "-DCMAKE_CXX_FLAGS=$DAEMON_APP_ANDROID_CXX_FLAGS" \
    "-DCMAKE_SHARED_LINKER_FLAGS=$DAEMON_APP_ANDROID_LINKER_FLAGS" \
    "-DCMAKE_MODULE_LINKER_FLAGS=$DAEMON_APP_ANDROID_LINKER_FLAGS" \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF \
    -DDAEMON_APP_TUI=OFF \
    -DECM_DIR="$ECM_DIR" \
    -DKATEHIGHLIGHTINGINDEXER_EXECUTABLE="$DAEMON_APP_KATE_INDEXER" \
    -DMD4QT_SOURCE_DIR="$MD4QT_SOURCE_DIR" \
    -DEARCUT_SOURCE_DIR="$EARCUT_SOURCE_DIR" \
    -DKSYNTAXHIGHLIGHTING_SOURCE_DIR="$KSYNTAXHIGHLIGHTING_SOURCE_DIR" \
    -DMICROTEX_SOURCE_DIR="$MICROTEX_SOURCE_DIR"

cmake --build "$builddir"

appdir="$builddir/src/DaemonApp/App"

# The stock full-fat path: stages AND runs the generated Gradle project
# (downloads AGP/androidx on first run), signs with the SDK debug key.
"$ANDROIDDEPLOYQT" \
    --input "$appdir/android-daemon-app-deployment-settings.json" \
    --output "$appdir/android-build" \
    --android-platform android-35 \
    --jdk "$JAVA_HOME" \
    --gradle \
    --verbose

echo
echo "APK: $appdir/android-build/build/outputs/apk/debug/android-build-debug.apk"
