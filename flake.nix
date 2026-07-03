{
  description = "daemon-app - Qt 6 QML/C++ development environment, build, and vendored dependencies";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";

    # Emscripten pin for the Qt-for-WebAssembly outputs ONLY (see
    # nix/qt-wasm.nix). Qt 6.11 documents emscripten 4.0.7 as its supported
    # toolchain; the primary pin ships 5.0.7 (an unsupported major jump) and
    # nixpkgs never carried 4.0.7 - this rev holds the closest match, 4.0.8.
    # The desktop outputs never touch this input.
    nixpkgs-emscripten.url = "github:NixOS/nixpkgs/e6f23dc08d3624daab7094b701aa3954923c6bbb";

    # --- Content renderer dependencies (all platforms; wired when the
    #     markdown/diagram renderer module lands) ---
    md4qt = {
      url = "gitlab:libraries/md4qt/345bba20c225c105dc9050dbb150bcc0d4c0c84f?host=invent.kde.org";
      flake = false;
    };
    earcut = {
      url = "git+https://github.com/mapbox/earcut.hpp?allRefs=1&rev=64530eabb07cfbdf48b2577a763ca26f1afc0b76&submodules=1";
      flake = false;
    };
    ksyntaxhighlighting = {
      url = "gitlab:frameworks/syntax-highlighting/ccb31f722406d5b980ba57cf71a3ffab70a82847?host=invent.kde.org";
      flake = false;
    };
    microtex = {
      url = "github:NanoMichael/MicroTeX/0e3707f6dafebb121d98b53c64364d16fefe481d";
      flake = false;
    };

    # --- TUI frontend dependencies (Tui Widgets, Meson-built, consumed by the
    #     CMake build via pkg-config; see src/tui/CMakeLists.txt). termpaint
    #     itself is taken from nixpkgs; these two are not packaged there. ---
    posixsignalmanager = {
      url = "github:textshell/posixsignalmanager/6fa40e48380d1179fcd6f5175b8bc9fb1e2a455d";
      flake = false;
    };
    tuiwidgets = {
      url = "github:tuiwidgets/tuiwidgets/31ccc7af4ff47ec209e8e7b3508988fec5d2e2e9";
      flake = false;
    };

    # --- Desktop-only dependencies ---
    qwindowkit = {
      url = "git+https://github.com/stdware/qwindowkit?allRefs=1&rev=c783b89d119a672b0aa4262b520c0783d0612689&submodules=1";
      flake = false;
    };
    qsimpleupdater = {
      url = "github:alex-spataru/QSimpleUpdater/6c78600eab46e307eb0706a52b847516eb72a146";
      flake = false;
    };
    qautostart = {
      url = "github:Skycoder42/QAutoStart/a27a028f0ac9e49a0c90732d7ea090e2bd31c2c3";
      flake = false;
    };
    qxtglobalshortcut = {
      url = "github:hluk/qxtglobalshortcut/0e47e8c6bddfe42ecb1f3706ead805d3abc56a7e";
      flake = false;
    };

    # Embedded terminal: QML port of qtermwidget (qmake-built, Qt6 master).
    # Built into a standalone QML plugin (import QMLTermWidget) and surfaced on
    # the QML import path; it cannot use the CMake add_subdirectory vendoring
    # path the other deps use.
    qmltermwidget = {
      url = "github:Swordfish90/qmltermwidget";
      flake = false;
    };
  };

  outputs =
    { self, nixpkgs, nixpkgs-emscripten, flake-utils, md4qt, earcut, ksyntaxhighlighting, microtex, posixsignalmanager, tuiwidgets, qwindowkit, qsimpleupdater, qautostart, qxtglobalshortcut, qmltermwidget, ... }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        # Shared on-disk cache for C/C++ builds (ccache).
        # Expose it to the build sandbox with:
        #   nix build .# --extra-sandbox-paths /var/cache/ccache
        ccacheDir = "/var/cache/ccache";

        ccacheOverlay = final: prev: {
          ccacheWrapper = prev.ccacheWrapper.override {
            extraConfig = ''
              export CCACHE_COMPRESS=1
              export CCACHE_DIR="${ccacheDir}"
              export CCACHE_UMASK=007
              export CCACHE_SLOPPINESS=random_seed
            '';
          };
        };

        pkgs = import nixpkgs {
          inherit system;
          overlays = [ ccacheOverlay ];
        };

        # Version: the SemVer base lives in `./VERSION` (the single source CMake reads via
        # file(STRINGS) into project(VERSION)). The build-metadata id is derived from the flake
        # source revision (no `.git` in the sandbox), retaining the off-tag / dirty marker like
        # phosphor: `g<rev>` clean, `g<rev>.dirty` dirty, else a narHash fallback. Passed to CMake as
        # -DDAEMON_APP_VERSION_STR so daemon_app_version.h bakes in the exact revision.
        baseVersion = pkgs.lib.strings.trim (builtins.readFile ./VERSION);
        buildId =
          if self ? shortRev then
            "g${self.shortRev}"
          else if self ? dirtyShortRev then
            "g${pkgs.lib.removeSuffix "-dirty" self.dirtyShortRev}.dirty"
          else
            "nar${builtins.substring 0 8 (pkgs.lib.removePrefix "sha256-" (self.narHash or "sha256-unknown"))}";
        versionStr = "${baseVersion}+${buildId}";

        qtPackages = with pkgs.qt6; [
          qtbase          # Core, Gui, Widgets, Network, Sql
          qtdeclarative   # Qml, Quick, QuickControls2, QuickTest
          qttools
          qtshadertools
          qtsvg
          qtwebsockets    # WebSocket daemon transport (browser/wasm carrier, tested natively)
          qt5compat       # Core5Compat - required by the Qt6 build of Tui Widgets
        ];

        # --- Tui Widgets stack (Meson) -----------------------------------------
        # Built as plain Nix derivations that emit pkg-config files; the daemon-app
        # CMake build then consumes only TuiWidgetsQt6 via pkg_check_modules. This
        # is the clean Meson<->CMake bridge: no driving Meson from CMake.
        #
        # Dependency chain: termpaint (nixpkgs) + posixsignalmanager (here) feed
        # tuiwidgets. All are Boost-licensed and link Qt6 Core (+ Core5Compat).
        posixsignalmanager-qt6 = pkgs.stdenv.mkDerivation {
          pname = "posixsignalmanager-qt6";
          version = "0.3.1";
          src = posixsignalmanager;
          nativeBuildInputs = with pkgs; [ meson ninja pkg-config qt6.qtbase qt6.wrapQtAppsHook ];
          buildInputs = [ pkgs.qt6.qtbase ];
          mesonFlags = [ "-Dqt=qt6" "-Dtests=false" ];
        };

        tuiwidgets-qt6 = pkgs.stdenv.mkDerivation {
          pname = "tuiwidgets-qt6";
          version = "0.2.3";
          src = tuiwidgets;
          nativeBuildInputs = with pkgs; [ meson ninja pkg-config python3 qt6.qtbase qt6.wrapQtAppsHook ];
          buildInputs = [ pkgs.termpaint pkgs.qt6.qtbase pkgs.qt6.qt5compat posixsignalmanager-qt6 ];
          mesonFlags = [ "-Dqt=qt6" "-Dtests=false" "-Dsystem-posixsignalmanager=enabled" ];
          # The Qt6 symbol-version generator (src/symver_qt6.py) is run at build
          # time via its `/usr/bin/env python3` shebang, which the sandbox lacks.
          postPatch = ''
            patchShebangs .
          '';
        };

        # The deps the TUI frontend links, surfaced to both the package and the
        # devShell so pkg-config resolves TuiWidgetsQt6.
        tuiDeps = [ pkgs.termpaint posixsignalmanager-qt6 tuiwidgets-qt6 ];

        # --- Embedded terminal QML plugin --------------------------------------
        # qmltermwidget is a qmake project (not CMake), so it builds as its own
        # derivation rather than via the add_subdirectory vendoring used for the
        # CMake deps below. It emits a QML plugin (import QMLTermWidget) which we
        # surface on the QML import path for the build, devShell, and the wrapped
        # app. The .pro installs into $$[QT_INSTALL_QML] (qtbase's store path);
        # redirect that into our own $out so it is relocatable.
        qmltermwidget-qt6 = pkgs.qt6.qtbase.stdenv.mkDerivation {
          pname = "qmltermwidget-qt6";
          version = "unstable";
          src = qmltermwidget;
          nativeBuildInputs = with pkgs; [ qt6.qmake qt6.wrapQtAppsHook ];
          buildInputs = with pkgs.qt6; [ qtbase qtdeclarative ];
          postPatch = ''
            substituteInPlace qmltermwidget.pro \
              --replace-fail '$$[QT_INSTALL_QML]' "$out/${pkgs.qt6.qtbase.qtQmlPrefix}"
          '';
          # Ship our per-theme color schemes alongside the bundled ones, in the
          # dir the plugin points COLORSCHEMES_DIR at, so TerminalPanel can select
          # them by name (colorScheme: "daemon-<Theme>").
          postInstall = ''
            cp ${./src/DaemonApp/Terminal/color-schemes}/*.colorscheme \
               "$out/${pkgs.qt6.qtbase.qtQmlPrefix}/QMLTermWidget/color-schemes/"
          '';
          dontWrapQtApps = true;
        };

        # The directory holding the QMLTermWidget plugin, reused by the package
        # buildInputs, the devShell import-path exports, and the CMake flag that
        # lets qmllint resolve `import QMLTermWidget`.
        qmltermwidgetQmlDir = "${qmltermwidget-qt6}/${pkgs.qt6.qtbase.qtQmlPrefix}";

        # Source dirs exported to CMake as <DEP>_SOURCE_DIR.
        depFlags = [
          "-DMD4QT_SOURCE_DIR=${md4qt}"
          "-DEARCUT_SOURCE_DIR=${earcut}"
          "-DKSYNTAXHIGHLIGHTING_SOURCE_DIR=${ksyntaxhighlighting}"
          "-DMICROTEX_SOURCE_DIR=${microtex}"
          "-DQWINDOWKIT_SOURCE_DIR=${qwindowkit}"
          "-DQSIMPLEUPDATER_SOURCE_DIR=${qsimpleupdater}"
          "-DQAUTOSTART_SOURCE_DIR=${qautostart}"
          "-DQXTGLOBALSHORTCUT_SOURCE_DIR=${qxtglobalshortcut}"
          "-DQMLTERMWIDGET_QML_DIR=${qmltermwidgetQmlDir}"
        ];

        daemon-app = pkgs.ccacheStdenv.mkDerivation {
          pname = "daemon-app";
          version = baseVersion;
          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            pkg-config
            # KSyntaxHighlighting builds from source via add_subdirectory: it needs
            # ECM (find_package(ECM REQUIRED)) and perl (find_package(Perl REQUIRED),
            # used to auto-generate the PHP syntax definition at build time). The
            # top-level extra-cmake-modules alias was removed with KDE5; the Qt6 ECM
            # lives under kdePackages.
            kdePackages.extra-cmake-modules
            perl
            qt6.wrapQtAppsHook
            # Linguist tools (lupdate/lrelease) for qt_add_translations: needed
            # on the host at build time to compile i18n/*.ts -> embedded .qm.
            qt6.qttools
          ];

          # MicroTeX (LaTeX math renderer) links tinyxml2 via pkg-config.
          # qmltermwidget-qt6 ships the embedded-terminal QML plugin;
          # wrapQtAppsHook adds its lib/qml to the wrapped app's import path.
          # qtkeychain backs the OS-keychain server-token store (auth6); the build
          # falls back to a QSettings token store when it is absent.
          buildInputs = qtPackages ++ [ pkgs.tinyxml-2 pkgs.qt6Packages.qtkeychain qmltermwidget-qt6 ];

          cmakeFlags = depFlags ++ [ "-DDAEMON_APP_VERSION_STR=${versionStr}" ];
        };

        # The experimental Tui Widgets TUI frontend (daemon-tui executable),
        # gated behind -DDAEMON_APP_TUI=ON. Reuses the GUI build, adding the TUI
        # dependency stack.
        daemon-tui = daemon-app.overrideAttrs (old: {
          pname = "daemon-tui";
          buildInputs = old.buildInputs ++ tuiDeps;
          cmakeFlags = depFlags ++ [
            "-DDAEMON_APP_TUI=ON"
            "-DDAEMON_APP_VERSION_STR=${versionStr}"
          ];
        });

        # --- Qt for WebAssembly ---------------------------------------------
        # Cross-compiled Qt target stack (emscripten 4.0.8 from the dedicated
        # pin) + the wasm app build + smoke check + dev shell. Everything
        # lives in nix/qt-wasm.nix to keep this file readable; only the wasm
        # outputs below evaluate it, so the desktop outputs stay unchanged.
        qtWasmStack = import ./nix/qt-wasm.nix {
          inherit pkgs versionStr baseVersion;
          pkgsEmscripten = import nixpkgs-emscripten { inherit system; };
          appSrc = ./.;
          depSources = { inherit md4qt earcut ksyntaxhighlighting microtex; };
        };
      in
      {
        packages.default = daemon-app;
        packages.tui = daemon-tui;
        # Exposed for debugging the Meson dependency stack in isolation.
        packages.tuiwidgets = tuiwidgets-qt6;
        packages.posixsignalmanager = posixsignalmanager-qt6;

        # Qt-for-WebAssembly toolchain (nix/qt-wasm.nix). `qt-wasm` is the
        # joined target stack a consumer points QT_HOST_PATH-style tooling at
        # (bin/qt-cmake + lib/cmake/Qt6/qt.toolchain.cmake); the per-module
        # packages are exposed for debugging individual build layers.
        packages.qt-wasm = qtWasmStack.qtWasm;
        packages.qtbase-wasm = qtWasmStack.qtbase;
        packages.qtshadertools-wasm = qtWasmStack.qtshadertools;
        packages.qtdeclarative-wasm = qtWasmStack.qtdeclarative;
        packages.qtsvg-wasm = qtWasmStack.qtsvg;
        packages.qtwebsockets-wasm = qtWasmStack.qtwebsockets;
        # The daemon-app browser build: the emscripten artifact set
        # (html/js/wasm/data + qtloader.js) under share/daemon-app/wasm,
        # served as-is by apps.serve-wasm below.
        packages.wasm = qtWasmStack.app;

        # Proves the wasm stack end-to-end without the app: a static Qt Quick
        # hello-world through the joined prefix's qt-cmake, asserting the
        # .wasm/.js/.html artifact set.
        checks.qt-wasm-smoke = qtWasmStack.smoke;

        apps.default = {
          type = "app";
          program = "${daemon-app}/bin/daemon-app";
          meta.description = "Run the daemon-app Qt Quick application";
        };

        # Serve the browser build over plain HTTP for a local smoke test
        # (`nix run .#serve-wasm`, then open http://localhost:8000/daemon-app.html).
        # The wasm stack is single-threaded, so no COOP/COEP headers are needed
        # and python's stock http.server suffices.
        apps.serve-wasm = {
          type = "app";
          program = "${pkgs.writeShellApplication {
            name = "serve-wasm";
            runtimeInputs = [ pkgs.python3 ];
            text = ''
              port="''${1:-8000}"
              echo "Serving daemon-app (wasm) at http://localhost:$port/daemon-app.html"
              exec python3 -m http.server "$port" --bind 127.0.0.1 \
                --directory ${qtWasmStack.app}/share/daemon-app/wasm
            '';
          }}/bin/serve-wasm";
          meta.description = "Serve the daemon-app WebAssembly build on localhost";
        };

        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            cmake
            ninja
            pkg-config
            clang-tools # clang-tidy, clang-format, run-clang-tidy (see justfile `lint-cpp`)
            gcc
            gdb
            kdePackages.extra-cmake-modules
            perl
            tinyxml-2
            # --- code-quality tooling (see justfile `lint-cpp` / `audit-cleanup`) ---
            cppcheck # whole-program dead code (--enable=unusedFunction) + extra static analysis
            gersemi # CMake formatter
            gitleaks # secret scanning
            typos # source spell-checker
            nodejs # provides npx for jscpd duplicate detection (not packaged in nixpkgs)
            just # task runner: the justfile recipes (lint / build / qmllint)
            qt6Packages.qtkeychain # OS keychain for the server-token store (auth6)
          ] ++ qtPackages ++ tuiDeps ++ [ qmltermwidget-qt6 ];

          shellHook = ''
            export QT_PLUGIN_PATH="${pkgs.lib.makeSearchPath pkgs.qt6.qtbase.qtPluginPrefix qtPackages}:$QT_PLUGIN_PATH"
            export QML_IMPORT_PATH="${pkgs.lib.makeSearchPath pkgs.qt6.qtbase.qtQmlPrefix qtPackages}:${qmltermwidgetQmlDir}:$QML_IMPORT_PATH"
            export QML2_IMPORT_PATH="$QML_IMPORT_PATH:$QML2_IMPORT_PATH"
            export CMAKE_PREFIX_PATH="${pkgs.lib.makeSearchPath "lib/cmake" qtPackages}:$CMAKE_PREFIX_PATH"
            export MD4QT_SOURCE_DIR="${md4qt}"
            export EARCUT_SOURCE_DIR="${earcut}"
            export KSYNTAXHIGHLIGHTING_SOURCE_DIR="${ksyntaxhighlighting}"
            export MICROTEX_SOURCE_DIR="${microtex}"
            export QWINDOWKIT_SOURCE_DIR="${qwindowkit}"
            export QSIMPLEUPDATER_SOURCE_DIR="${qsimpleupdater}"
            export QAUTOSTART_SOURCE_DIR="${qautostart}"
            export QXTGLOBALSHORTCUT_SOURCE_DIR="${qxtglobalshortcut}"
            export QMLTERMWIDGET_QML_DIR="${qmltermwidgetQmlDir}"
          '';
        };

        # Wasm cross-development: emscripten + the wasm Qt stack + host tools.
        # Exports DAEMON_APP_QT_WASM for the wasm-release CMake preset, which
        # consumes $env{DAEMON_APP_QT_WASM}/lib/cmake/Qt6/qt.toolchain.cmake.
        devShells.wasm = qtWasmStack.devShell;
      }
    );
}
