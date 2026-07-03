# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# Qt-for-WebAssembly cross toolchain + the daemon-app wasm build.
#
# nixpkgs ships no prebuilt Qt-for-wasm, so the target stack is cross-compiled
# from source here: the SAME qt6 6.x sources as the host Qt (Qt requires the
# host tools' version to match the target version), configured for
# `-platform wasm-emscripten` - static, Release, single-threaded (Qt's default
# for wasm; threaded wasm needs COOP/COEP headers and is a bring-up risk).
#
# Emscripten comes from a dedicated nixpkgs pin (`nixpkgs-emscripten`): Qt 6.11
# documents emscripten 4.0.7 as its supported toolchain, the primary nixpkgs
# pin already ships 5.0.7 (an unsupported major jump), and 4.0.8 at the pinned
# rev is the closest version nixpkgs ever carried. The same emscripten builds
# every layer (Qt, third-party, app), which also satisfies Qt's strict
# "built-with == used-with" emcc version check baked into qconfig.h.
{
  pkgs,
  pkgsEmscripten,
  appSrc,
  baseVersion,
  versionStr,
  depSources,
}:
let
  inherit (pkgs) lib;

  # The ONLY thing taken from the emscripten pin; all other build tools come
  # from the primary nixpkgs so the wasm outputs share the desktop toolchain.
  emscripten = pkgsEmscripten.emscripten;

  qtVersion = pkgs.qt6.qtbase.version;

  # --- Host-tools prefix (QT_HOST_PATH) -----------------------------------
  # Qt cross builds want ONE host prefix carrying moc/rcc/syncqt/qmlcachegen/
  # qmltyperegistrar/lrelease/qsb + their Qt6*Tools CMake packages; nixpkgs
  # splits Qt per module, so join the tool-carrying modules into one tree.
  # The Qt6*ToolsConfig files resolve tool paths relative to themselves
  # (_IMPORT_PREFIX), which follows the symlinks into the real store paths.
  qtHost = pkgs.symlinkJoin {
    name = "qt-host-tools-${qtVersion}";
    paths = with pkgs.qt6; [
      qtbase # moc, rcc, syncqt, qmake, Qt6HostInfo
      qtdeclarative # qmlcachegen, qmltyperegistrar, qmlimportscanner
      qttools # lrelease/lupdate (Qt6LinguistTools for qt_add_translations)
      qtshadertools # qsb (Qt6ShaderToolsTools, needed by Quick's shader baking)
    ];
  };

  # --- emsdk-shaped root ---------------------------------------------------
  # Qt's wasm auto-detection (qt_auto_detect_wasm + the per-target
  # _qt_test_emscripten_version check in Qt6WasmMacros) hard-requires an EMSDK
  # env var pointing at an emsdk layout: it reads `$EMSDK/.emscripten`,
  # extracts the quoted path suffix from the EMSCRIPTEN_ROOT line and runs
  # `$EMSDK/<suffix>/emcc --version`. nixpkgs' emscripten is not an emsdk, and
  # its own .emscripten records an absolute EMSCRIPTEN_ROOT (the suffix trick
  # would produce a doubled path), so fake the minimal emsdk shape around it.
  emsdkRoot = pkgs.runCommand "emsdk-root-${emscripten.version}" { } ''
    mkdir -p $out/upstream
    ln -s ${emscripten}/share/emscripten $out/upstream/emscripten
    echo "EMSCRIPTEN_ROOT = emsdk_path + '/upstream/emscripten'" > $out/.emscripten
  '';

  # The vanilla emscripten CMake toolchain; qtbase records it into its
  # generated qt.toolchain.cmake, so downstream builds chainload it again.
  emToolchain = "${emscripten}/share/emscripten/cmake/Modules/Platform/Emscripten.cmake";

  # Shared per-derivation environment: emscripten's compiler cache lives in
  # the read-only store, but emcc compiles missing system-library variants on
  # demand for the exact flag set Qt uses - give every build a writable copy.
  # The Emscripten CMake toolchain drives the UNWRAPPED emcc (next to the
  # toolchain file), whose launcher shim finds python via $PYTHON/$PATH; pin
  # the same interpreter nixpkgs bakes into the wrapped bin/emcc.
  emBuildEnv = ''
    export EM_CACHE="$TMPDIR/em-cache"
    cp -r --no-preserve=mode ${emscripten}/share/emscripten/cache "$EM_CACHE"
    export EMSDK=${emsdkRoot}
    export PYTHON=${pkgsEmscripten.python3}/bin/python3
  '';

  # --- qtbase for wasm ------------------------------------------------------
  # Configured with plain CMake (the documented equivalents of
  # `configure -platform wasm-emscripten -static -release -qt-host-path ...`).
  # Qt's wasm auto-detect forces the static config; thread stays at Qt's wasm
  # default (OFF). Feature trims are conservative:
  #  * optimize-size ON: Qt's supported size-optimized build (-Os on every
  #    module; the feature is global, so qtdeclarative/qtsvg/... inherit it).
  #  * ltcg (LTO) evaluated and dropped: FEATURE_ltcg=ON aborts (Qt's
  #    __qt_ltcg_detected probe fails under this emscripten), and forcing
  #    thin-LTO via -flto=thin on Qt + app compile flags built and booted but
  #    measured -74 KB raw / +27 KB brotli on daemon-app.wasm - noise-level
  #    raw, strictly worse compressed, so not carried.
  #  * dbus OFF: no session bus in a browser, saves a large module.
  #  * sql-sqlite ON + system-sqlite OFF (explicit): the app uses
  #    QSqlDatabase/QSQLITE, so pin the bundled-sqlite driver rather than rely
  #    on autodetection.
  # Widgets stays ON, measured verdict: the shipped binary already contains
  # ZERO Widgets/PrintSupport code - the app never links Qt6::Widgets on wasm
  # and vendored MicroTeX's LaTeX library links Qt6::Gui only (its
  # find_package asks for Widgets+PrintSupport, but solely for the
  # LaTeXQtSample demo, which is EXCLUDE_FROM_ALL and never built; verified:
  # no libQt6Widgets.a/libQt6PrintSupport.a on the final link line). Turning
  # the feature OFF would save 0 shipped bytes and would require shimming the
  # vendored find_package call just to configure - not worth it.
  qtbaseWasm = pkgs.stdenv.mkDerivation {
    pname = "qtbase-wasm";
    version = qtVersion;
    src = pkgs.qt6.qtbase.src; # pristine upstream tarball (nixpkgs' desktop patches don't apply to wasm)

    nativeBuildInputs = with pkgs; [
      cmake
      ninja
    ];

    # The cmake hook would inject -DCMAKE_C_COMPILER=$CC (host gcc) and Nix
    # install-dir flags, both of which fight the emscripten toolchain file;
    # configure manually instead. ninja's hook still drives build + install.
    dontUseCmakeConfigure = true;

    configurePhase = ''
      runHook preConfigure
      ${emBuildEnv}
      cmake -S . -B build -G Ninja \
        -DCMAKE_INSTALL_PREFIX="$out" \
        -DCMAKE_BUILD_TYPE=Release \
        -DQT_QMAKE_TARGET_MKSPEC=wasm-emscripten \
        -DCMAKE_TOOLCHAIN_FILE=${emToolchain} \
        -DQT_HOST_PATH=${qtHost} \
        -DQT_BUILD_EXAMPLES=OFF \
        -DQT_BUILD_TESTS=OFF \
        -DQT_GENERATE_SBOM=OFF \
        -DFEATURE_optimize_size=ON \
        -DFEATURE_dbus=OFF \
        -DFEATURE_sql_sqlite=ON \
        -DFEATURE_system_sqlite=OFF
      cd build
      runHook postConfigure
    '';
  };

  # --- Qt modules on top of qtbase-wasm ------------------------------------
  # Each module configures through the wasm qtbase's qt-cmake (which exports
  # the generated qt.toolchain.cmake: emscripten chainload + QT_HOST_PATH).
  # Sibling wasm modules live in separate store prefixes, so they are handed
  # to the toolchain via QT_ADDITIONAL_PACKAGES_PREFIX_PATH, Qt's supported
  # mechanism for extra module prefixes under cross-compilation (it folds them
  # into both CMAKE_PREFIX_PATH and CMAKE_FIND_ROOT_PATH).
  mkQtWasmModule =
    {
      pname,
      src,
      wasmDeps ? [ ],
      extraFlags ? [ ],
    }:
    pkgs.stdenv.mkDerivation {
      inherit pname src;
      version = qtVersion;

      nativeBuildInputs = with pkgs; [
        cmake
        ninja
        python3 # qtdeclarative's qml configure requires a host Python
      ];

      dontUseCmakeConfigure = true;

      configurePhase = ''
        runHook preConfigure
        ${emBuildEnv}
        export QT_ADDITIONAL_PACKAGES_PREFIX_PATH="${lib.concatStringsSep ":" (map toString wasmDeps)}"
        ${qtbaseWasm}/bin/qt-cmake -S . -B build -G Ninja \
          -DCMAKE_INSTALL_PREFIX="$out" \
          -DCMAKE_BUILD_TYPE=Release \
          -DQT_BUILD_EXAMPLES=OFF \
          -DQT_BUILD_TESTS=OFF \
          -DQT_GENERATE_SBOM=OFF \
          ${lib.concatStringsSep " " extraFlags}
        cd build
        runHook postConfigure
      '';
    };

  qtshadertoolsWasm = mkQtWasmModule {
    pname = "qtshadertools-wasm";
    src = pkgs.qt6.qtshadertools.src;
  };

  qtdeclarativeWasm = mkQtWasmModule {
    pname = "qtdeclarative-wasm";
    src = pkgs.qt6.qtdeclarative.src;
    wasmDeps = [ qtshadertoolsWasm ];
    # The app pins QQuickStyle::setStyle("Basic") in main() and imports no
    # other style, but a static Quick Controls build links EVERY built style
    # plugin (+ its compiled QML and shader resources) into the executable
    # because style selection is a runtime decision. Build only Basic: the
    # other styles were 3.2 MB raw / 1.2 MB brotli of the shipped binary.
    # (fluentwinui3 follows fusion's condition, so it needs no explicit flag;
    # macos/windows/ios never build on wasm.)
    extraFlags = [
      "-DFEATURE_quickcontrols2_fusion=OFF"
      "-DFEATURE_quickcontrols2_imagine=OFF"
      "-DFEATURE_quickcontrols2_material=OFF"
      "-DFEATURE_quickcontrols2_universal=OFF"
    ];
  };

  qtsvgWasm = mkQtWasmModule {
    pname = "qtsvg-wasm";
    src = pkgs.qt6.qtsvg.src;
  };

  qtwebsocketsWasm = mkQtWasmModule {
    pname = "qtwebsockets-wasm";
    src = pkgs.qt6.qtwebsockets.src;
    # qtwebsockets ships a QML plugin; Quick pulls shadertools transitively.
    wasmDeps = [
      qtdeclarativeWasm
      qtshadertoolsWasm
    ];
  };

  # One prefix for consumers: `$qtWasmJoined/bin/qt-cmake` (and the CMake
  # toolchain at lib/cmake/Qt6/qt.toolchain.cmake) sees every module because
  # the toolchain resolves its install prefix from its own (symlinked)
  # location, and CMake does not resolve symlinks for list files.
  qtWasmJoined = pkgs.symlinkJoin {
    name = "qt-wasm-${qtVersion}";
    paths = [
      qtbaseWasm
      qtshadertoolsWasm
      qtdeclarativeWasm
      qtsvgWasm
      qtwebsocketsWasm
    ];
  };

  # --- Third-party for the wasm app ----------------------------------------
  # MicroTeX (vendored into the app build) links tinyxml2 via pkg-config;
  # nixpkgs' tinyxml-2 is a host library, so cross-compile the same pinned
  # source for wasm and surface its .pc file to the app's configure.
  tinyxml2Wasm = pkgs.stdenv.mkDerivation {
    pname = "tinyxml2-wasm";
    version = pkgs.tinyxml-2.version;
    src = pkgs.tinyxml-2.src;

    nativeBuildInputs = with pkgs; [
      cmake
      ninja
    ];

    dontUseCmakeConfigure = true;

    configurePhase = ''
      runHook preConfigure
      ${emBuildEnv}
      cmake -S . -B build -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE=${emToolchain} \
        -DCMAKE_INSTALL_PREFIX="$out" \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=OFF \
        -Dtinyxml2_BUILD_TESTING=OFF \
        -DBUILD_TESTING=OFF
      cd build
      runHook postConfigure
    '';
  };

  # --- End-to-end smoke test ------------------------------------------------
  # A trivial Qt Quick hello-world (qt_add_qml_module + static plugins) built
  # with the JOINED prefix's qt-cmake - proving exactly the consumption path
  # the app build and the sibling branch's CMake preset use - and asserting
  # the emscripten artifact set (.wasm/.js/.html) exists.
  smokeSrc = pkgs.linkFarm "qt-wasm-smoke-src" [
    {
      name = "CMakeLists.txt";
      path = pkgs.writeText "qt-wasm-smoke-CMakeLists.txt" ''
        cmake_minimum_required(VERSION 3.21)
        project(qt-wasm-smoke LANGUAGES CXX)
        find_package(Qt6 6.11 REQUIRED COMPONENTS Quick)
        qt_standard_project_setup(REQUIRES 6.11)
        qt_add_executable(hello main.cpp)
        qt_add_qml_module(hello URI Smoke VERSION 1.0 QML_FILES Main.qml)
        target_link_libraries(hello PRIVATE Qt6::Quick)
      '';
    }
    {
      name = "main.cpp";
      path = pkgs.writeText "qt-wasm-smoke-main.cpp" ''
        #include <QGuiApplication>
        #include <QQmlApplicationEngine>

        int main(int argc, char *argv[])
        {
            QGuiApplication app(argc, argv);
            QQmlApplicationEngine engine;
            engine.loadFromModule("Smoke", "Main");
            return app.exec();
        }
      '';
    }
    {
      name = "Main.qml";
      path = pkgs.writeText "qt-wasm-smoke-Main.qml" ''
        import QtQuick

        Window {
            visible: true

            Text {
                anchors.centerIn: parent
                text: "qt-wasm smoke"
            }
        }
      '';
    }
  ];

  smoke = pkgs.stdenv.mkDerivation {
    pname = "qt-wasm-smoke";
    version = qtVersion;
    src = smokeSrc;

    nativeBuildInputs = with pkgs; [
      cmake
      ninja
    ];

    dontUseCmakeConfigure = true;

    configurePhase = ''
      runHook preConfigure
      ${emBuildEnv}
      ${qtWasmJoined}/bin/qt-cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release
      cd build
      runHook postConfigure
    '';

    installPhase = ''
      runHook preInstall
      for artifact in hello.wasm hello.js hello.html qtloader.js; do
        if [ ! -s "$artifact" ]; then
          echo "qt-wasm smoke: expected emscripten artifact '$artifact' is missing" >&2
          exit 1
        fi
      done
      mkdir -p "$out"
      cp hello.wasm hello.js hello.html qtloader.js "$out/"
      runHook postInstall
    '';
  };

  # --- daemon-app for wasm --------------------------------------------------
  # The full browser build: compiles the app against the wasm Qt stack and
  # installs the emscripten artifact set (html/js/wasm/data + qtloader.js)
  # under share/daemon-app/wasm, ready to serve as-is (see apps.serve-wasm).
  # Desktop-only inputs are deliberately absent: no qmltermwidget (its QML dir
  # flag would drag the desktop plugin build in), no qtkeychain (the token
  # store falls back to QSettings), no TUI stack, no wrapQtAppsHook.
  wasmDepFlags = [
    "-DMD4QT_SOURCE_DIR=${depSources.md4qt}"
    "-DEARCUT_SOURCE_DIR=${depSources.earcut}"
    "-DKSYNTAXHIGHLIGHTING_SOURCE_DIR=${depSources.ksyntaxhighlighting}"
    "-DMICROTEX_SOURCE_DIR=${depSources.microtex}"
  ];

  # KSyntaxHighlighting's find_package(ECM) cannot see host prefixes under the
  # emscripten toolchain's CMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY; an explicit
  # ECM_DIR bypasses the re-rooted search (ECM is host-arch-independent CMake
  # modules, so using the host copy is correct). ECM's platform allowlist does
  # not know Emscripten and offers KF_IGNORE_PLATFORM_CHECK as the sanctioned
  # override; KSyntaxHighlighting itself is plain portable C++/Qt.
  ecmDir = "${pkgs.kdePackages.extra-cmake-modules}/share/ECM/cmake";

  # Cross-compiling KSyntaxHighlighting needs a NATIVE katehighlightingindexer
  # (it bakes the syntax-definition index into the QRC resources). Its own
  # ExternalProject fallback re-configures ${CMAKE_SOURCE_DIR} on the host -
  # which in the vendored add_subdirectory context is the whole daemon-app
  # tree, so it can never work here. Host-build the indexer from the SAME
  # pinned source instead and hand it in via KATEHIGHLIGHTINGINDEXER_EXECUTABLE
  # (the upstream-supported knob for exactly this). GUI off: the indexer only
  # links Qt6::Core, and this skips host OpenGL requirements.
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

  app = pkgs.stdenv.mkDerivation {
    pname = "daemon-app-wasm";
    version = baseVersion;
    src = appSrc;

    nativeBuildInputs = with pkgs; [
      cmake
      ninja
      pkg-config
      # Vendored KSyntaxHighlighting needs ECM + perl on the host (see the
      # desktop package's note); lrelease comes from QT_HOST_PATH's qttools.
      kdePackages.extra-cmake-modules
      perl
    ];

    # A post-link `wasm-opt -Oz` re-run (binaryen from the emscripten pin) was
    # evaluated and dropped: emcc already runs wasm-opt -Oz at link, and a
    # second pass with emcc 4.0.8's default feature set converges to -17 KB
    # raw / -9 KB brotli (~0.1%). The only way to shrink further (-1.5 MB raw)
    # is letting binaryen emit tail-call + GC call_ref encodings, which raises
    # the artifact's browser feature floor for a -12 KB brotli delta - not a
    # trade a "low-risk" pipeline should make. Keep the single link-time pass.

    dontUseCmakeConfigure = true;

    configurePhase = ''
      runHook preConfigure
      ${emBuildEnv}
      export PKG_CONFIG_PATH="${tinyxml2Wasm}/lib/pkgconfig"
      # MicroTeX's CMake links the raw pkg-config library name instead of the
      # PkgConfig:: imported target, so tinyxml2's include/lib dirs never reach
      # its compile/link lines. The desktop build gets them implicitly from
      # nixpkgs' cc wrapper; emcc has no such wrapper, so ride the global flags.
      # BUILD_SHARED_LIBS=OFF: KDECMakeSettings (vendored KSyntaxHighlighting)
      # defaults it ON, but shared libraries don't exist on this static
      # wasm-emscripten config - everything links into the executable.
      ${qtWasmJoined}/bin/qt-cmake -S . -B build -G Ninja \
        -DCMAKE_INSTALL_PREFIX="$out" \
        -DCMAKE_BUILD_TYPE=MinSizeRel \
        "-DCMAKE_C_FLAGS_MINSIZEREL=-Oz -DNDEBUG" \
        "-DCMAKE_CXX_FLAGS_MINSIZEREL=-Oz -DNDEBUG" \
        "-DCMAKE_CXX_FLAGS=-isystem ${tinyxml2Wasm}/include" \
        "-DCMAKE_EXE_LINKER_FLAGS=-L${tinyxml2Wasm}/lib" \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_TESTING=OFF \
        -DDAEMON_APP_TUI=OFF \
        -DECM_DIR=${ecmDir} \
        -DKF_IGNORE_PLATFORM_CHECK=ON \
        -DKATEHIGHLIGHTINGINDEXER_EXECUTABLE=${ksyntaxIndexerHost}/bin/katehighlightingindexer \
        ${lib.concatStringsSep " " wasmDepFlags} \
        -DDAEMON_APP_VERSION_STR=${versionStr}
      cd build
      runHook postConfigure
    '';

    # Precompressed siblings for every shipped artifact so any real webserver
    # (nginx gzip_static/brotli_static, caddy precompressed) can serve them;
    # the network cost of a deploy is the .br column. brotli/gzip come from
    # the MAIN nixpkgs pin - they only transform the output, not the build.
    # The python serve-wasm helper ignores them and serves the raw files,
    # which is fine for dev.
    postInstall = ''
      for f in "$out"/share/daemon-app/wasm/*; do
        case "$f" in
          *.br | *.gz) continue ;;
        esac
        ${pkgs.brotli}/bin/brotli -q 11 -k "$f"
        ${pkgs.gzip}/bin/gzip -9 -k "$f"
      done
    '';
  };

  # --- Dev shell -------------------------------------------------------------
  # `DAEMON_APP_QT_WASM` is the contract with the CMake preset landing on a
  # sibling branch: it consumes
  #   $env{DAEMON_APP_QT_WASM}/lib/cmake/Qt6/qt.toolchain.cmake
  # The emscripten cache seed lives under XDG cache (the store copy is
  # read-only, and emcc needs to compile missing library variants on demand).
  devShell = pkgs.mkShell {
    packages =
      (with pkgs; [
        cmake
        ninja
        pkg-config
        kdePackages.extra-cmake-modules
        perl
        qt6.qttools # host lupdate/lrelease on PATH for translation upkeep
      ])
      ++ [ emscripten ];

    shellHook = ''
      # The host qttools package drags in nixpkgs' qtbase setup hook, which
      # exports QT_ADDITIONAL_PACKAGES_PREFIX_PATH = the whole host
      # CMAKE_PREFIX_PATH. Qt's wasm qt.toolchain.cmake folds that variable
      # into CMAKE_FIND_ROOT_PATH, so the cross configure would resolve HOST
      # desktop libraries (e.g. libglvnd's libEGL.so) and feed them to
      # wasm-ld. The joined wasm prefix already carries every Qt module, so
      # the variable is not needed here at all - scrub it.
      unset QT_ADDITIONAL_PACKAGES_PREFIX_PATH
      export DAEMON_APP_QT_WASM=${qtWasmJoined}
      export QT_HOST_PATH=${qtHost}
      export EMSDK=${emsdkRoot}
      export EM_CACHE="''${XDG_CACHE_HOME:-$HOME/.cache}/daemon-app/em-cache-${emscripten.version}"
      if [ ! -d "$EM_CACHE" ]; then
        mkdir -p "$(dirname "$EM_CACHE")"
        cp -r --no-preserve=mode ${emscripten}/share/emscripten/cache "$EM_CACHE"
        chmod -R u+w "$EM_CACHE"
      fi
      # Exact, not appended: the shell's host Qt tools (qttools -> qtbase)
      # surface desktop .pc files (libglvnd's egl among them), and Qt6Gui's
      # FindEGL would otherwise hand the host libEGL.so to wasm-ld. Pinning
      # to the wasm tinyxml2 mirrors the sandboxed app derivation exactly.
      export PKG_CONFIG_PATH="${tinyxml2Wasm}/lib/pkgconfig"
      export ECM_DIR="${ecmDir}"
      # Consumed by the wasm-release preset so a devShell configure carries
      # the same tinyxml2 injection + native indexer as the app derivation.
      export DAEMON_APP_WASM_CXX_FLAGS="-isystem ${tinyxml2Wasm}/include"
      export DAEMON_APP_WASM_LINKER_FLAGS="-L${tinyxml2Wasm}/lib"
      export DAEMON_APP_KATE_INDEXER="${ksyntaxIndexerHost}/bin/katehighlightingindexer"
      export MD4QT_SOURCE_DIR="${depSources.md4qt}"
      export EARCUT_SOURCE_DIR="${depSources.earcut}"
      export KSYNTAXHIGHLIGHTING_SOURCE_DIR="${depSources.ksyntaxhighlighting}"
      export MICROTEX_SOURCE_DIR="${depSources.microtex}"
    '';
  };
in
{
  inherit
    qtHost
    emsdkRoot
    smoke
    app
    devShell
    ;
  qtbase = qtbaseWasm;
  qtshadertools = qtshadertoolsWasm;
  qtdeclarative = qtdeclarativeWasm;
  qtsvg = qtsvgWasm;
  qtwebsockets = qtwebsocketsWasm;
  qtWasm = qtWasmJoined;
  tinyxml2 = tinyxml2Wasm;
}
