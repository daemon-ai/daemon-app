#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# Build a static Qt 6 for the iOS simulator (arm64) from source, using the
# SYSTEM Xcode toolchain (impure: the iPhoneSimulator SDK is not in the nix
# store) and store-pinned Qt sources + nix cmake/ninja/python3. Driven by the
# `qt-ios-builder` writeShellApplication (nix/ios.nix), which exports the
# DAEMON_APP_QT_IOS_SRC_* store paths + DAEMON_APP_QT_HOST before exec'ing it.
#
# Everything lands in ONE prefix ($DAEMON_APP_QT_IOS_PREFIX); consumers point
# $prefix/bin/qt-cmake at the app. Mirrors nix/android.nix's module set and
# feature trims (static, Basic-only QQC, optimize-size), but the build itself
# cannot be a pure derivation, hence this script.
set -euo pipefail

# --- inputs (exported by the qt-ios-builder wrapper) -----------------------
: "${DAEMON_APP_QT_HOST:?qt-ios-builder must export DAEMON_APP_QT_HOST}"
: "${DAEMON_APP_QT_IOS_SRC_QTBASE:?missing qtbase src}"
: "${DAEMON_APP_QT_IOS_SRC_QTSHADERTOOLS:?missing qtshadertools src}"
: "${DAEMON_APP_QT_IOS_SRC_QTDECLARATIVE:?missing qtdeclarative src}"
: "${DAEMON_APP_QT_IOS_SRC_QTSVG:?missing qtsvg src}"
: "${DAEMON_APP_QT_IOS_SRC_QTWEBSOCKETS:?missing qtwebsockets src}"
: "${DAEMON_APP_QT_IOS_SRC_TINYXML2:?missing tinyxml2 src}"
qtver="${DAEMON_APP_QT_IOS_VERSION:-unknown}"

# --- runtime knobs ---------------------------------------------------------
prefix="${DAEMON_APP_QT_IOS_PREFIX:-$HOME/.cache/daemon-app/qt-ios-sim-${qtver}}"
buildroot="${DAEMON_APP_QT_IOS_BUILD:-$HOME/.cache/daemon-app/qt-ios-build}"
force="${DAEMON_APP_QT_IOS_FORCE:-0}"
jobs="${DAEMON_APP_QT_IOS_JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || echo 4)}"

# qtdeclarative's qml configure hard-requires a host Python. Under the iOS
# toolchain CMAKE_FIND_ROOT_PATH_MODE_PROGRAM re-roots find_program into the
# sysroot, so cmake's FindPython misses the one on PATH; pin it explicitly.
pybin="$(command -v python3 || true)"

# --- system Xcode (impure) -------------------------------------------------
# The Qt macx-ios-clang mkspec + QT_APPLE_SDK=iphonesimulator resolve the
# compiler/sysroot via xcrun; make sure a real Xcode (not the CLT shim) is
# selected and drop any nix cc-wrapper env that would fight it.
DEVELOPER_DIR="${DEVELOPER_DIR:-$(xcode-select -p)}"
export DEVELOPER_DIR
unset CC CXX CFLAGS CXXFLAGS LDFLAGS CPPFLAGS 2>/dev/null || true

echo "== qt-ios-build =="
echo "   version : ${qtver}"
echo "   prefix  : ${prefix}"
echo "   build   : ${buildroot}"
echo "   jobs    : ${jobs}"
echo "   xcode   : ${DEVELOPER_DIR}"
echo "   sim sdk : $(xcrun --sdk iphonesimulator --show-sdk-version 2>/dev/null || echo '??')"

mkdir -p "$prefix" "$buildroot"

# Unpack a store src (tarball or dir) into a writable build dir.
prepare_src() {
  local src="$1" dst="$2"
  rm -rf "$dst"
  mkdir -p "$dst"
  if [ -d "$src" ]; then
    cp -R "$src"/. "$dst"/
  else
    tar -xf "$src" -C "$dst" --strip-components=1
  fi
  chmod -R u+w "$dst"
}

stamp() { echo "$prefix/.stamp-$1"; }
done_stage() { [ "$force" != "1" ] && [ -f "$(stamp "$1")" ]; }
mark_stage() { : > "$(stamp "$1")"; }

# --- qtbase ----------------------------------------------------------------
build_qtbase() {
  if done_stage qtbase; then echo "-- qtbase: already built, skipping"; return; fi
  echo "== qtbase =="
  local s="$buildroot/qtbase"
  prepare_src "$DAEMON_APP_QT_IOS_SRC_QTBASE" "$s"

  # arm64 pitfall (community-documented for 6.11.x): qyieldcpu.h uses __yield()
  # but does not include <arm_acle.h>. Defensive, guarded, idempotent (the
  # header has its own include guards).
  local yfile="$s/src/corelib/thread/qyieldcpu.h"
  if [ -f "$yfile" ] && ! grep -q arm_acle "$yfile"; then
    echo "-- patching qyieldcpu.h with <arm_acle.h>"
    python3 - "$yfile" <<'PY'
import sys
p = sys.argv[1]
text = open(p).read()
inc = "#if defined(__aarch64__) || defined(_M_ARM64)\n#include <arm_acle.h>\n#endif\n"
if "#pragma once" in text:
    text = text.replace("#pragma once", "#pragma once\n" + inc, 1)
else:
    text = inc + text
open(p, "w").write(text)
PY
  fi

  cmake -S "$s" -B "$s/build" -G Ninja \
    -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DCMAKE_BUILD_TYPE=Release \
    -DQT_QMAKE_TARGET_MKSPEC=macx-ios-clang \
    -DQT_APPLE_SDK=iphonesimulator \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DQT_HOST_PATH="$DAEMON_APP_QT_HOST" \
    -DBUILD_SHARED_LIBS=OFF \
    -DQT_BUILD_EXAMPLES=OFF \
    -DQT_BUILD_TESTS=OFF \
    -DQT_GENERATE_SBOM=OFF \
    -DFEATURE_optimize_size=ON \
    -DFEATURE_dbus=OFF \
    -DFEATURE_zstd=OFF \
    -DFEATURE_sql_sqlite=ON \
    -DFEATURE_system_sqlite=OFF
  cmake --build "$s/build" -j "$jobs"
  cmake --install "$s/build"
  mark_stage qtbase
}

# --- a Qt module through the freshly built iOS qt-cmake --------------------
build_module() {
  local name="$1" src="$2"; shift 2
  if done_stage "$name"; then echo "-- $name: already built, skipping"; return; fi
  echo "== $name =="
  local s="$buildroot/$name"
  prepare_src "$src" "$s"
  "$prefix/bin/qt-cmake" -S "$s" -B "$s/build" -G Ninja \
    -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DCMAKE_BUILD_TYPE=Release \
    -DQT_BUILD_EXAMPLES=OFF \
    -DQT_BUILD_TESTS=OFF \
    -DQT_GENERATE_SBOM=OFF \
    ${pybin:+-DPython_EXECUTABLE="$pybin" -DPython3_EXECUTABLE="$pybin"} \
    "$@"
  cmake --build "$s/build" -j "$jobs"
  cmake --install "$s/build"
  mark_stage "$name"
}

# --- tinyxml2 (links into the app; MicroTeX resolves it via pkg-config) ----
# Reuse the Qt iOS toolchain so tinyxml2 gets the exact same
# simulator/arch/sysroot as Qt, then install into the same prefix so the app
# configure discovers it via $prefix/lib/pkgconfig + -isystem/-L $prefix.
build_tinyxml2() {
  if done_stage tinyxml2; then echo "-- tinyxml2: already built, skipping"; return; fi
  echo "== tinyxml2 =="
  local s="$buildroot/tinyxml2"
  prepare_src "$DAEMON_APP_QT_IOS_SRC_TINYXML2" "$s"
  "$prefix/bin/qt-cmake" -S "$s" -B "$s/build" -G Ninja \
    -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -Dtinyxml2_BUILD_TESTING=OFF \
    -DBUILD_TESTING=OFF
  cmake --build "$s/build" -j "$jobs"
  cmake --install "$s/build"
  mark_stage tinyxml2
}

build_qtbase
build_module qtshadertools "$DAEMON_APP_QT_IOS_SRC_QTSHADERTOOLS"
build_module qtdeclarative "$DAEMON_APP_QT_IOS_SRC_QTDECLARATIVE" \
  -DFEATURE_quickcontrols2_fusion=OFF \
  -DFEATURE_quickcontrols2_imagine=OFF \
  -DFEATURE_quickcontrols2_material=OFF \
  -DFEATURE_quickcontrols2_universal=OFF
build_module qtsvg "$DAEMON_APP_QT_IOS_SRC_QTSVG"
build_module qtwebsockets "$DAEMON_APP_QT_IOS_SRC_QTWEBSOCKETS"
build_tinyxml2

echo
echo "== DONE =="
echo "Qt for iOS simulator (arm64, static) is at:"
echo "  $prefix"
echo "Configure the app with: $prefix/bin/qt-cmake -S . -B build-ios -G Xcode ..."
