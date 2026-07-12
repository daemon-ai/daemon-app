# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# Static Qt 6 for the iOS SIMULATOR (arm64) + the bits daemon-app needs to
# cross-configure against it as a thin remote client.
#
# WHY THIS IS NOT A PURE DERIVATION (unlike nix/android.nix): the iPhone
# Simulator SDK and its toolchain live inside Xcode, which is proprietary and
# not in nixpkgs. androidenv puts the Android SDK/NDK in the store, so the
# android Qt stack is a normal cross derivation; there is no equivalent for
# Apple's SDK. So this file pins everything that is pinnable (the Qt + tinyxml2
# SOURCES, nix cmake/ninja/python3, the host-tools prefix) and hands them to a
# generated script (scripts/qt-ios-build.sh, wrapped as `qt-ios-builder`) that
# performs the impure build with the SYSTEM Xcode via xcrun/DEVELOPER_DIR. The
# app is then configured in `devShells.ios` with the produced qt-cmake.
#
# aarch64-darwin only (the flake gates the outputs). Mirrors android.nix's
# module set (qtbase + shadertools + declarative + svg + websockets), feature
# trims (static, Basic-only QQC, optimize-size), and vendored-dep provisioning
# (cross tinyxml2, host katehighlightingindexer, ECM_DIR) as closely as an
# impure build allows.
{
  pkgs,
  appSrc,
  baseVersion,
  versionStr,
  depSources,
}:
let
  inherit (pkgs) lib;

  # Qt requires the host tools' version to match the target version, so the
  # iOS stack tracks nixpkgs' qt6 release (the darwin desktop qt6 = 6.11.1).
  qtVersion = pkgs.qt6.qtbase.version;

  # --- Host-tools prefix (QT_HOST_PATH) -----------------------------------
  # Identical join to the wasm/android stacks: one native (arm64 macOS) prefix
  # carrying moc/rcc/syncqt/qmlcachegen/qmltyperegistrar/lrelease/qsb + their
  # Qt6*Tools CMake packages, for the cross build's QT_HOST_PATH.
  qtHost = pkgs.symlinkJoin {
    name = "qt-host-tools-${qtVersion}";
    paths = with pkgs.qt6; [
      qtbase # moc, rcc, syncqt, qmake, Qt6HostInfo
      qtdeclarative # qmlcachegen, qmltyperegistrar, qmlimportscanner
      qttools # lrelease/lupdate (Qt6LinguistTools for qt_add_translations)
      qtshadertools # qsb (Qt6ShaderToolsTools, needed by Quick's shader baking)
    ];
  };

  # KSyntaxHighlighting's find_package(ECM) cannot see host prefixes under the
  # iOS toolchain's re-rooted find paths; an explicit ECM_DIR bypasses the
  # search (ECM is host-arch-independent CMake modules, so the host copy is
  # correct). Same value as the wasm/android stacks.
  ecmDir = "${pkgs.kdePackages.extra-cmake-modules}/share/ECM/cmake";

  # Cross-compiling KSyntaxHighlighting needs a NATIVE katehighlightingindexer
  # (it bakes the syntax-definition index into the QRC resources). Identical
  # derivation to the wasm/android stacks' (same inputs -> shared store path).
  ksyntaxIndexerHost = pkgs.stdenv.mkDerivation {
    pname = "katehighlightingindexer";
    version = "host";
    src = depSources.ksyntaxhighlighting;

    nativeBuildInputs = with pkgs; [
      cmake
      ninja
      perl
      kdePackages.extra-cmake-modules
      qt6.qttools # ecm_install_po_files_as_qm needs Qt6LinguistTools at configure
      qt6.wrapQtAppsHook
    ];
    buildInputs = [ pkgs.qt6.qtbase ];

    cmakeFlags = [
      "-DKSYNTAXHIGHLIGHTING_USE_GUI=OFF"
      "-DBUILD_TESTING=OFF"
    ];
    ninjaFlags = [ "katehighlightingindexer" ]; # only the tool, not the framework

    installPhase = ''
      runHook preInstall
      install -Dm755 bin/katehighlightingindexer "$out/bin/katehighlightingindexer"
      runHook postInstall
    '';
    dontWrapQtApps = true; # links Qt6::Core only; no plugins to wrap for
  };

  # Default install prefix for the produced Qt (a user cache dir, since the
  # build is impure and cannot land in the store). The builder honours
  # DAEMON_APP_QT_IOS_PREFIX; the devShell + docs use the same default.
  defaultPrefix = "$HOME/.cache/daemon-app/qt-ios-sim-${qtVersion}";

  # --- The impure Qt-for-iOS-simulator builder ------------------------------
  # A writeShellApplication that exports the store-pinned sources + host-tools
  # prefix, then runs scripts/qt-ios-build.sh (system Xcode via xcrun). The
  # heavy build logic lives in the tracked script so it stays shellcheck-able
  # and free of nix-string escaping; the pinning is here.
  qtIosBuilder = pkgs.writeShellApplication {
    name = "qt-ios-builder";
    runtimeInputs = with pkgs; [
      cmake
      ninja
      python3
      gnutar
      xz
    ];
    text = ''
      export DAEMON_APP_QT_HOST="${qtHost}"
      export DAEMON_APP_QT_IOS_SRC_QTBASE="${pkgs.qt6.qtbase.src}"
      export DAEMON_APP_QT_IOS_SRC_QTSHADERTOOLS="${pkgs.qt6.qtshadertools.src}"
      export DAEMON_APP_QT_IOS_SRC_QTDECLARATIVE="${pkgs.qt6.qtdeclarative.src}"
      export DAEMON_APP_QT_IOS_SRC_QTSVG="${pkgs.qt6.qtsvg.src}"
      export DAEMON_APP_QT_IOS_SRC_QTWEBSOCKETS="${pkgs.qt6.qtwebsockets.src}"
      export DAEMON_APP_QT_IOS_SRC_TINYXML2="${pkgs.tinyxml-2.src}"
      export DAEMON_APP_QT_IOS_VERSION="${qtVersion}"
      exec bash ${../scripts/qt-ios-build.sh} "$@"
    '';
  };

  # --- Dev shell for the app cross-configure (I2b) --------------------------
  # nix cmake/ninja + host Qt tools + the vendored-dep wiring the android/wasm
  # configures pass. DAEMON_APP_QT_IOS points at the builder's output prefix;
  # the app is configured with $DAEMON_APP_QT_IOS/bin/qt-cmake -G Xcode. The
  # cross tinyxml2 lands INSIDE that prefix (the builder installs it there), so
  # PKG_CONFIG_PATH + the -isystem/-L flags reference the same prefix.
  devShell = pkgs.mkShell {
    packages = with pkgs; [
      cmake
      ninja
      pkg-config
      kdePackages.extra-cmake-modules
      perl
      qt6.qttools # host lrelease/lupdate on PATH for the translation targets
    ];

    shellHook = ''
      # The nixpkgs darwin stdenv sets SDKROOT/DEVELOPER_DIR/MACOSX_DEPLOYMENT_
      # TARGET to the in-store (macOS) Apple SDK. An iOS build drives the SYSTEM
      # Xcode toolchain (xcrun, iphonesimulator SDK); the in-store macOS SDK
      # makes CMake reject the target ("iphonesimulator is not an iOS SDK").
      # Unset them so xcrun/xcode-select fall back to the system Xcode selection.
      unset SDKROOT DEVELOPER_DIR MACOSX_DEPLOYMENT_TARGET
      # The host qttools drags in nixpkgs' qtbase setup hook, which exports
      # QT_ADDITIONAL_PACKAGES_PREFIX_PATH = the whole host CMAKE_PREFIX_PATH.
      # Qt's iOS qt.toolchain.cmake folds that into CMAKE_FIND_ROOT_PATH, so the
      # app's find_package(Qt6) would resolve the HOST (desktop) Qt instead of
      # the iOS one. The iOS prefix already carries every module - scrub it
      # (same fix as the wasm devShell).
      unset QT_ADDITIONAL_PACKAGES_PREFIX_PATH
      export QT_HOST_PATH="${qtHost}"
      export DAEMON_APP_QT_IOS="''${DAEMON_APP_QT_IOS:-${defaultPrefix}}"
      export ECM_DIR="${ecmDir}"
      export DAEMON_APP_KATE_INDEXER="${ksyntaxIndexerHost}/bin/katehighlightingindexer"
      # tinyxml2 (cross-built into the Qt prefix by qt-ios-builder). MicroTeX's
      # CMake links the raw pkg-config library name, so its include/lib dirs
      # never reach the compile/link lines; ride the global flags like the
      # wasm/android configures.
      export PKG_CONFIG_PATH="$DAEMON_APP_QT_IOS/lib/pkgconfig:''${PKG_CONFIG_PATH:-}"
      export DAEMON_APP_IOS_CXX_FLAGS="-isystem $DAEMON_APP_QT_IOS/include"
      export DAEMON_APP_IOS_LINKER_FLAGS="-L$DAEMON_APP_QT_IOS/lib"
      export MD4QT_SOURCE_DIR="${depSources.md4qt}"
      export EARCUT_SOURCE_DIR="${depSources.earcut}"
      export KSYNTAXHIGHLIGHTING_SOURCE_DIR="${depSources.ksyntaxhighlighting}"
      export MICROTEX_SOURCE_DIR="${depSources.microtex}"
      export QRCODEGEN_SOURCE_DIR="${depSources.qrcodegen}"
      export IMMER_SOURCE_DIR="${depSources.immer}"
      echo "daemon-app iOS devShell"
      echo "  QT_HOST_PATH      = $QT_HOST_PATH"
      echo "  DAEMON_APP_QT_IOS = $DAEMON_APP_QT_IOS"
      if [ ! -x "$DAEMON_APP_QT_IOS/bin/qt-cmake" ]; then
        echo "  (Qt not built yet - run: nix run .#qt-ios-builder)"
      fi
    '';
  };
in
{
  inherit
    qtHost
    qtIosBuilder
    devShell
    ksyntaxIndexerHost
    ;
}
