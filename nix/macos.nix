# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# Static-Qt macOS desktop build: a self-contained daemon-app .app bundle
# with Qt compiled in (no /nix/store Qt references, no macdeployqt needed),
# packaged as a drag-and-drop .dmg via hdiutil.
#
# Mirrors portable.nix (the Linux static stack) using the same shared
# qt-from-source.nix builder, flavor "macos-static". macOS SDK comes from
# nixpkgs apple-sdk (pure — no Xcode; Xcode is only needed for iOS).
#
# Static/dynamic decisions (the .app binary's runtime dependencies):
#  * INTO the binary (static): all Qt modules + platform plugins (cocoa,
#    offscreen), Qt's bundled zlib/libpng/libjpeg/harfbuzz/pcre2/
#    double-conversion, bundled sqlite.
#  * DYNAMIC against the system (always present on macOS): system frameworks
#    (Cocoa, AppKit, Metal, Security, IOKit, CoreText, CoreGraphics, etc.),
#    libSystem, libc++. macOS has no distro variability — every supported
#    macOS ships identical system libraries.
#  * OFF entirely: icu, glib, zstd, brotli, dbus (macOS has no D-Bus;
#    qtkeychain uses Security.framework directly), vulkan, cups,
#    sessionmanager, fontconfig (macOS uses CoreText), xcb, wayland,
#    openssl (macOS uses SecureTransport for TLS natively).
{
  pkgs,
  appSrc,
  baseVersion,
  versionStr,
  depSources,
  tuiSources,
  # Crash reporting (Sentry native SDK): the unzipped sentry-native source tree
  # add_subdirectory'd by cmake/Dependencies.cmake (crashpad backend on macOS) +
  # the compiled-in product default DSN. Same contract as the Linux/Windows
  # lanes; an unset DSN compiles crash reporting OUT (documented SKIP).
  sentryNativeSrc ? null,
  sentryDsn ? "",
}:
let
  inherit (pkgs) lib;

  # On this mac /var/cache/ccache is not provisioned, so use plain stdenv
  # (ccacheStdenv would abort with "Permission denied"). The Qt-from-source
  # compile is a one-shot build with no incremental reuse here.
  staticStdenv = pkgs.stdenv;

  # The macOS SDK provides all system frameworks (Cocoa, Metal, Security,
  # IOKit, CoreText, etc.) plus headers and stubs for the system libraries.
  # On modern nixpkgs-darwin, apple-sdk is the unified package.
  sdk = pkgs.apple-sdk;

  qtFromSource = import ./qt-from-source.nix { inherit pkgs; } {
    flavor = "macos-static";
    stdenv = staticStdenv;
    # Qt's cmake checks call xcrun/xcodebuild which live in /usr/bin (not in
    # the nix store). Expose system tool paths for the configure + build.
    # Also pin Python for FindPython (qtdeclarative needs it).
    buildEnv = ''
      export PATH="$PATH:/usr/bin:/usr/sbin"
      export Python_EXECUTABLE="${pkgs.python3.interpreter}"
      export Python3_EXECUTABLE="${pkgs.python3.interpreter}"
    '';
    qtbaseFlags = [
      "-DBUILD_SHARED_LIBS=OFF"
      "-DQT_BUILD_EXAMPLES=OFF"
      "-DQT_BUILD_TESTS=OFF"
      "-DQT_GENERATE_SBOM=OFF"
      "-DFEATURE_optimize_size=ON"
      # Cocoa platform plugin compiled in statically; offscreen for CI/smoke.
      "\"-DQT_QPA_PLATFORMS=cocoa;offscreen;minimal\""
      "-DFEATURE_cocoa=ON"
      # macOS rendering: Metal (primary) + OpenGL (fallback via OpenGL.framework).
      "-DFEATURE_opengl=ON"
      "-DFEATURE_metal=ON"
      # Qt's bundled third-party — macOS SDK provides zlib natively.
      "-DFEATURE_system_zlib=ON"
      "-DFEATURE_system_png=OFF"
      "-DFEATURE_system_jpeg=OFF"
      "-DFEATURE_system_harfbuzz=OFF"
      "-DFEATURE_system_pcre2=OFF"
      "-DFEATURE_system_doubleconversion=OFF"
      # Bundled SQLite driver (same pin as linux-static/wasm).
      "-DFEATURE_sql_sqlite=ON"
      "-DFEATURE_system_sqlite=OFF"
      # macOS uses CoreText for fonts (no fontconfig/freetype system).
      "-DFEATURE_fontconfig=OFF"
      "-DFEATURE_system_freetype=OFF"
      # macOS uses SecureTransport (Security.framework) for TLS natively;
      # OpenSSL is not part of the macOS SDK and not needed.
      "-DFEATURE_openssl=OFF"
      # macOS has NO D-Bus: disable entirely (keychain uses Security.framework).
      "-DFEATURE_dbus=OFF"
      # Trims (same rationale as linux-static).
      "-DFEATURE_icu=OFF"
      "-DFEATURE_glib=OFF"
      "-DFEATURE_zstd=OFF"
      "-DFEATURE_brotli=OFF"
      "-DFEATURE_sessionmanager=OFF"
      "-DFEATURE_vulkan=OFF"
      # PrintSupport is needed by vendored KSyntaxHighlighting; CUPS headers
      # live in the nixpkgs SDK but the library is only in the system Xcode SDK
      # (nixpkgs apple-sdk is trimmed). Point cmake at Xcode's SDK copy.
      "-DFEATURE_cups=ON"
      "-DCUPS_INCLUDE_DIR=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include"
      "-DCUPS_LIBRARIES=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/lib/libcups.tbd"
      # No X11/Wayland on macOS.
      "-DFEATURE_xcb=OFF"
      "-DFEATURE_wayland_client=OFF"
    ];
    qtbaseNativeBuildInputs = with pkgs; [
      cmake
      ninja
      pkg-config
    ];
    qtbaseBuildInputs = [
      sdk
      pkgs.zlib # system zlib for Qt and module tool linking
    ];
    moduleNativeBuildInputs = with pkgs; [
      cmake
      ninja
      python3
      pkg-config
    ];
    moduleBuildInputs = [
      sdk
      pkgs.zlib
    ];
  };
  inherit (qtFromSource) qtVersion mkQtModule;
  qtbaseStatic = qtFromSource.qtbase;

  qtshadertoolsStatic = mkQtModule {
    name = "qtshadertools";
    src = pkgs.qt6.qtshadertools.src;
  };

  qtdeclarativeStatic = mkQtModule {
    name = "qtdeclarative";
    src = pkgs.qt6.qtdeclarative.src;
    qtDeps = [ qtshadertoolsStatic ];
    extraFlags = [
      "-DFEATURE_quickcontrols2_fusion=OFF"
      "-DFEATURE_quickcontrols2_imagine=OFF"
      "-DFEATURE_quickcontrols2_material=OFF"
      "-DFEATURE_quickcontrols2_universal=OFF"
      "-DPython_EXECUTABLE=${pkgs.python3.interpreter}"
    ];
  };

  qtsvgStatic = mkQtModule {
    name = "qtsvg";
    src = pkgs.qt6.qtsvg.src;
  };

  qtwebsocketsStatic = mkQtModule {
    name = "qtwebsockets";
    src = pkgs.qt6.qtwebsockets.src;
    qtDeps = [
      qtdeclarativeStatic
      qtshadertoolsStatic
    ];
  };

  qt5compatStatic = mkQtModule {
    name = "qt5compat";
    src = pkgs.qt6.qt5compat.src;
    qtDeps = [
      qtdeclarativeStatic
      qtshadertoolsStatic
    ];
  };

  qtStaticJoined = pkgs.symlinkJoin {
    name = "qt-macos-static-${qtVersion}";
    paths = [
      qtbaseStatic
      qtshadertoolsStatic
      qtdeclarativeStatic
      qtsvgStatic
      qtwebsocketsStatic
      qt5compatStatic
    ];
  };

  # Relocated qmake for meson-based builds (same pattern as linux-static).
  qtStaticQmakeHost = pkgs.runCommand "qt-macos-static-qmake-host-${qtVersion}" { } ''
    mkdir -p "$out/bin"
    cp -L ${qtStaticJoined}/bin/qmake6 "$out/bin/qmake6"
    printf '[Paths]\nPrefix=%s\n' "${qtStaticJoined}" > "$out/bin/qt.conf"
    for t in moc rcc uic qmlcachegen qmltyperegistrar qmlimportscanner qsb; do
      if [ -e "${qtStaticJoined}/bin/$t" ]; then
        ln -s "${qtStaticJoined}/bin/$t" "$out/bin/$t"
      fi
    done
  '';

  tinyxml2Static = staticStdenv.mkDerivation {
    pname = "tinyxml2-macos-static";
    version = pkgs.tinyxml-2.version;
    src = pkgs.tinyxml-2.src;

    nativeBuildInputs = with pkgs; [
      cmake
      ninja
    ];

    cmakeFlags = [
      "-DBUILD_SHARED_LIBS=OFF"
      "-Dtinyxml2_BUILD_TESTING=OFF"
      "-DBUILD_TESTING=OFF"
      "-DCMAKE_INSTALL_INCLUDEDIR=include"
      "-DCMAKE_INSTALL_LIBDIR=lib"
    ];
  };

  # OS keychain: Security.framework backend (macOS native). No LIBSECRET
  # (that's Linux D-Bus Secret Service). qtkeychain auto-selects the macOS
  # SecItemAdd/SecItemCopyMatching backend when building on darwin.
  qtkeychainStatic = staticStdenv.mkDerivation {
    pname = "qtkeychain-macos-static";
    version = pkgs.qt6Packages.qtkeychain.version;
    src = pkgs.qt6Packages.qtkeychain.src;

    nativeBuildInputs = with pkgs; [
      cmake
      ninja
      pkg-config
    ];
    buildInputs = [
      qtStaticJoined
      sdk
      pkgs.zlib
    ];

    cmakeFlags = [
      "-DBUILD_SHARED_LIBS=OFF"
      "-DBUILD_WITH_QT6=ON"
      "-DLIBSECRET_SUPPORT=OFF"
      "-DBUILD_TEST_APPLICATION=OFF"
      "-DBUILD_TRANSLATIONS=OFF"
    ];
  };

  # --- Static TUI stack (termpaint + Tui Widgets) ---
  termpaintStatic = pkgs.termpaint.overrideAttrs (old: {
    pname = "termpaint-static";
    mesonFlags = (old.mesonFlags or [ ]) ++ [ "-Ddefault_library=static" ];
    doCheck = false;
  });

  posixSignalManagerStatic = staticStdenv.mkDerivation {
    pname = "posixsignalmanager-static";
    version = "0.3.1";
    src = tuiSources.posixsignalmanager;
    nativeBuildInputs = with pkgs; [
      meson
      ninja
      pkg-config
      qtStaticQmakeHost
    ];
    buildInputs = [
      qtStaticJoined
      pkgs.zlib
    ];
    mesonFlags = [
      "-Dqt=qt6"
      "-Dtests=false"
      "-Ddefault_library=static"
    ];
  };

  tuiWidgetsStatic = staticStdenv.mkDerivation {
    pname = "tuiwidgets-static";
    version = "0.2.3";
    src = tuiSources.tuiwidgets;
    nativeBuildInputs = with pkgs; [
      meson
      ninja
      pkg-config
      python3
      qtStaticQmakeHost
    ];
    buildInputs = [
      termpaintStatic
      qtStaticJoined
      posixSignalManagerStatic
      pkgs.zlib
    ];
    mesonFlags = [
      "-Dqt=qt6"
      "-Dtests=false"
      "-Dsystem-posixsignalmanager=enabled"
      "-Ddefault_library=static"
    ];
    postPatch = ''
      patchShebangs .
      substituteInPlace src/meson.build \
        --replace-fail 'requires: [qt_dep],' 'requires: [],'
      substituteInPlace meson.build \
        --replace-fail "subdir('examples')" ""
    '';
  };

  # --- Embedded terminal QML plugin, STATIC (GPL-2.0) ---
  # Same as Linux: qmltermwidget built as a static QML plugin. Unix PTY
  # works identically on macOS (forkpty/openpty from util.h).
  qmltermwidgetStatic = staticStdenv.mkDerivation {
    pname = "qmltermwidget-macos-static";
    version = "unstable";
    src = tuiSources.qmltermwidget;

    nativeBuildInputs = [
      qtStaticQmakeHost
    ];
    buildInputs = [
      qtStaticJoined
      pkgs.zlib
    ];

    dontWrapQtApps = true;

    postPatch = ''
      if ! grep -q '^classname ' src/qmldir; then
        echo "classname QmltermwidgetPlugin" >> src/qmldir
      fi
    '';

    configurePhase = ''
      runHook preConfigure
      ${qtStaticQmakeHost}/bin/qmake6 CONFIG+=staticlib CONFIG-=shared
      runHook postConfigure
    '';

    installPhase = ''
      runHook preInstall
      dest="$out/qml/QMLTermWidget"
      mkdir -p "$dest/color-schemes"
      find . -name 'libqmltermwidget*.a' -exec cp {} "$dest/" \;
      cp src/qmldir "$dest/qmldir"
      cp src/QMLTermScrollbar.qml "$dest/"
      cp -r lib/color-schemes/. "$dest/color-schemes/" 2>/dev/null || true
      cp -r lib/kb-layouts "$dest/" 2>/dev/null || true
      cp ${appSrc}/src/DaemonApp/Terminal/color-schemes/*.colorscheme "$dest/color-schemes/" 2>/dev/null || true
      runHook postInstall
    '';
  };

  # --- daemon-app static macOS build ---
  # Feature-complete: real QMLTermWidget terminal + real OS keychain
  # (Security.framework) + TUI, all statically linked.
  appStatic = staticStdenv.mkDerivation {
    pname = "daemon-app-macos-static";
    version = baseVersion;
    src = appSrc;

    nativeBuildInputs = with pkgs; [
      cmake
      ninja
      pkg-config
      kdePackages.extra-cmake-modules
      perl
    ];

    buildInputs = [
      qtStaticJoined
      tinyxml2Static
      qtkeychainStatic
      tuiWidgetsStatic
      termpaintStatic
      posixSignalManagerStatic
      sdk
      pkgs.zlib
    ];

    cmakeFlags = [
      "-DCMAKE_BUILD_TYPE=Release"
      "-DBUILD_SHARED_LIBS=OFF"
      "-DBUILD_TESTING=OFF"
      "-DDAEMON_APP_TUI=ON"
      "-DDAEMON_APP_STATIC=ON"
      "-DPython_EXECUTABLE=${pkgs.python3.interpreter}"
      # Static Qt6PrintSupport re-resolves Cups::Cups in consumers.
      "-DCUPS_INCLUDE_DIR=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include"
      "-DCUPS_LIBRARIES=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/lib/libcups.tbd"
      "-DQMLTERMWIDGET_STATIC_LIB=${qmltermwidgetStatic}/qml/QMLTermWidget/libqmltermwidget.a"
      "-DQMLTERMWIDGET_STATIC_QML_DIR=${qmltermwidgetStatic}/qml"
      "-DQMLTERMWIDGET_QML_DIR=${qmltermwidgetStatic}/qml"
      "-DQt6LinguistTools_DIR=${pkgs.qt6.qttools}/lib/cmake/Qt6LinguistTools"
      "-DMD4QT_SOURCE_DIR=${depSources.md4qt}"
      "-DEARCUT_SOURCE_DIR=${depSources.earcut}"
      "-DKSYNTAXHIGHLIGHTING_SOURCE_DIR=${depSources.ksyntaxhighlighting}"
      "-DMICROTEX_SOURCE_DIR=${depSources.microtex}"
      "-DQRCODEGEN_SOURCE_DIR=${depSources.qrcodegen}"
      "-DIMMER_SOURCE_DIR=${depSources.immer}"
      "-DDAEMON_APP_VERSION_STR=${versionStr}"
      "-DDAEMON_APP_UPDATE_CAPABILITY=SelfApply"
      "-DDAEMON_APP_UPDATE_FEED_URL=https://github.com/daemon-ai/daemon/releases/latest/download/manifest.json"
      "-DDAEMON_APP_UPDATE_PUBKEY=RWRXpowS90Fy+TYhRsrBbQNSDvjbtJpqi9T89OGqSNTLkOa5vn62hK0o"
      "-DDAEMON_APP_UPDATE_ARTIFACT_KIND=dmg"
    ]
    # Crash reporting: sentry-native (crashpad backend, static) + the compiled-in
    # DSN, mirroring the Linux/Windows lanes. Wired once here; DAEMON_APP_TUI=ON
    # means this single build covers both the GUI and the TUI (the shared
    # cmake/Dependencies.cmake links libsentry into the app and builds the
    # co-located crashpad_handler that cmake/Packaging.cmake stages into the
    # bundle). Left unset (no sentryNativeSrc) it compiles crash reporting OUT.
    #
    # sentry-native's default transport on macOS is curl (only Windows uses
    # winhttp), so its find_package(CURL) must resolve. macOS ships libcurl as a
    # system library (dyld shared cache /usr/lib/libcurl.4.dylib); point CMake at
    # the SDK's stub (.tbd) + headers so the .app links the SYSTEM curl — no
    # /nix/store dylib to scrub, exactly like the CUPS handling above.
    ++ lib.optionals (sentryNativeSrc != null) [
      "-DSENTRY_NATIVE_SOURCE_DIR=${sentryNativeSrc}"
      "-DDAEMON_APP_SENTRY_DSN=${sentryDsn}"
      "-DCURL_INCLUDE_DIR=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include"
      "-DCURL_LIBRARY=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/lib/libcurl.tbd"
    ];

    CCACHE_DISABLE = "1";

    dontWrapQtApps = true;
    dontFixup = true;

    installPhase = ''
      runHook preInstall
      cmake --install . --prefix "$out"
      runHook postInstall
    '';
  };

  # --- .dmg packaging via hdiutil (drag-and-drop layout) ---
  # The static build's shippable product is the .app bundle that cmake's
  # install lays out (BUNDLE DESTINATION . in src/DaemonApp/App/CMakeLists.txt):
  #   ${appStatic}/daemon-app.app/Contents/MacOS/daemon-app     (static GUI, ~74 MB)
  #   ${appStatic}/daemon-app.app/Contents/MacOS/daemon-updater (self-update helper)
  #   ${appStatic}/daemon-app.app/Contents/Resources/...        (microtex-res, icon, license)
  # (daemon-tui installs to ${appStatic}/bin, deliberately NOT inside the .app,
  # so it is not part of the drag-to-Applications payload.)
  #
  # No macdeployqt is needed (Qt is compiled in). We stage a writable copy of
  # that bundle, scrub the baked /nix/store references, re-seal it, and wrap it
  # in a compressed, mountable DMG with an /Applications alias.
  dmg = pkgs.runCommand "daemon-macos-dmg-${baseVersion}" {
    nativeBuildInputs = with pkgs; [
      removeReferencesTo
    ];
  } ''
    export PATH="$PATH:/usr/bin:/usr/sbin"
    mkdir -p "$out"

    # Stage a writable copy of the real .app bundle (the store copy is
    # read-only, and scrubbing/signing rewrites bytes in place).
    stage="$TMPDIR/stage"
    mkdir -p "$stage"
    cp -a "${appStatic}/daemon-app.app" "$stage/daemon-app.app"
    chmod -R u+w "$stage/daemon-app.app"

    # Sanitize baked /nix/store references from every executable in the bundle
    # so the .app runs once dragged off the nix store. Two distinct kinds:
    #
    #  (a) DYNAMIC deps (LC_LOAD_DYLIB): the static build still links a couple of
    #      macOS *system* libraries from nixpkgs (libz, libresolv) by their
    #      absolute /nix/store path, which dangles off-store. Repoint them at
    #      their dyld-shared-cache home under /usr/lib (same backstop the
    #      dynamic build applies to libiconv in cmake/Packaging.cmake). Any
    #      OTHER residual store dylib has no system mapping and must not ship
    #      silently -> hard-fail so it gets bundled or mapped deliberately.
    #  (b) INERT string refs: the compiled-in Qt bakes QLibraryInfo prefix
    #      strings pointing at the static Qt store paths. Neutralize them with
    #      remove-references-to (Qt then resolves paths app-relative).
    for macho in "$stage/daemon-app.app/Contents/MacOS"/*; do
      [ -f "$macho" ] || continue
      for ref in $(otool -L "$macho" | awk 'NR>1 {print $1}' \
                     | grep '^/nix/store/.*\.dylib$' || true); do
        base="$(basename "$ref")"
        case "$base" in
          libz.*|libresolv.*|libiconv.*|libcharset.*)
            install_name_tool -change "$ref" "/usr/lib/$base" "$macho"
            ;;
          *)
            echo "FATAL: $macho has an unmapped /nix/store dynamic dep: $ref" >&2
            echo "       (no macOS system-lib mapping; it would dangle off-store)" >&2
            exit 1
            ;;
        esac
      done
      for ref in ${qtbaseStatic} ${qtshadertoolsStatic} ${qtdeclarativeStatic} \
                 ${qtsvgStatic} ${qtwebsocketsStatic} ${qt5compatStatic} \
                 ${tinyxml2Static} ${qtkeychainStatic} ${tuiWidgetsStatic} \
                 ${termpaintStatic} ${posixSignalManagerStatic} \
                 ${qmltermwidgetStatic} ${pkgs.zlib}; do
        remove-references-to -t "$ref" "$macho"
      done
      # install_name_tool + remove-references-to both void the ad-hoc signature.
      codesign --force --sign - "$macho"
    done
    # Re-seal the bundle now that its executables changed.
    codesign --force --sign - "$stage/daemon-app.app"

    # Drag-and-drop layout: the .app next to an /Applications alias.
    ln -s /Applications "$stage/Applications"

    # Build the compressed, mountable disk image. hdiutil lives in /usr/bin;
    # this mac's daemon build user runs with the sandbox disabled.
    hdiutil create \
      -volname "Daemon" \
      -srcfolder "$stage" \
      -format UDZO \
      -ov \
      "$out/daemon-app-${baseVersion}.dmg"

    # Checksum sidecar alongside the image.
    ( cd "$out" && shasum -a 256 "daemon-app-${baseVersion}.dmg" \
        > "daemon-app-${baseVersion}.dmg.sha256" )
  '';

  # Boot smoke (offscreen, mock service mode — same contract as Linux).
  bootSmoke = pkgs.runCommand "macos-boot-smoke" { } ''
    mkdir -p "$out"
    bin="${appStatic}/daemon-app.app/Contents/MacOS/daemon-app"
    report="$out/report.txt"

    echo "== macos-boot-smoke: $bin" | tee "$report"

    # 1. Verify no /nix/store Qt references in the binary.
    echo "-- otool -L (checking for nix store Qt refs) --" | tee -a "$report"
    otool -L "$bin" > otool.txt 2>&1 || true
    cat otool.txt >> "$report"
    if grep -q '/nix/store.*libQt' otool.txt; then
      echo "FAIL: /nix/store Qt reference found in binary" >&2
      cat otool.txt >&2
      exit 1
    fi
    echo "otool: no /nix/store Qt references (fully static)" | tee -a "$report"

    # 2. Offscreen boot.
    export HOME="$TMPDIR/home"; mkdir -p "$HOME"
    export XDG_RUNTIME_DIR="$TMPDIR/xdg"; mkdir -p "$XDG_RUNTIME_DIR"; chmod 700 "$XDG_RUNTIME_DIR"
    touch "$TMPDIR/daemon-sock-stub"
    export DAEMON_APP_SOCKET="$TMPDIR/daemon-sock-stub"
    export QT_QPA_PLATFORM=offscreen
    export QT_QUICK_BACKEND=software
    export DAEMON_APP_SERVICE_MODE=mock
    export DAEMON_APP_WAIT_READY=20000

    echo "-- boot (offscreen, mock service mode) --" | tee -a "$report"
    if "$bin" > boot.log 2>&1; then
      status=0
    else
      status=$?
    fi
    tail -n 20 boot.log >> "$report"
    grep -q 'DAEMON_APP_READY ok' boot.log || { echo "FAIL: no ready sentinel; log:" >&2; cat boot.log >&2; exit 1; }
    [ "$status" = 0 ] || { echo "FAIL: exit $status; log:" >&2; cat boot.log >&2; exit 1; }
    echo "boot: DAEMON_APP_READY ok (exit 0)" | tee -a "$report"
  '';
in
{
  qtbase = qtbaseStatic;
  qtshadertools = qtshadertoolsStatic;
  qtdeclarative = qtdeclarativeStatic;
  qtsvg = qtsvgStatic;
  qtwebsockets = qtwebsocketsStatic;
  qt5compat = qt5compatStatic;
  qtStatic = qtStaticJoined;
  tinyxml2 = tinyxml2Static;
  qtkeychain = qtkeychainStatic;
  termpaint = termpaintStatic;
  posixsignalmanager = posixSignalManagerStatic;
  tuiwidgets = tuiWidgetsStatic;
  qmltermwidget = qmltermwidgetStatic;
  app = appStatic;
  inherit dmg bootSmoke;
  scrubPrefixes = [
    (toString qtbaseStatic)
    (toString qtshadertoolsStatic)
    (toString qtdeclarativeStatic)
    (toString qtsvgStatic)
    (toString qtwebsocketsStatic)
    (toString tinyxml2Static)
    (toString qt5compatStatic)
    (toString qtkeychainStatic)
  ];
}
