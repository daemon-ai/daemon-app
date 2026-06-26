{
  description = "daemon-app - Qt 6 QML/C++ development environment, build, and vendored dependencies";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";

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
    { nixpkgs, flake-utils, md4qt, earcut, ksyntaxhighlighting, microtex, posixsignalmanager, tuiwidgets, qwindowkit, qsimpleupdater, qautostart, qxtglobalshortcut, qmltermwidget, ... }:
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

        qtPackages = with pkgs.qt6; [
          qtbase          # Core, Gui, Widgets, Network, Sql
          qtdeclarative   # Qml, Quick, QuickControls2, QuickTest
          qttools
          qtshadertools
          qtsvg
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
          version = "0.1.0";
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
          buildInputs = qtPackages ++ [ pkgs.tinyxml-2 qmltermwidget-qt6 ];

          cmakeFlags = depFlags;
        };

        # The experimental Tui Widgets TUI frontend (daemon-tui executable),
        # gated behind -DDAEMON_APP_TUI=ON. Reuses the GUI build, adding the TUI
        # dependency stack.
        daemon-tui = daemon-app.overrideAttrs (old: {
          pname = "daemon-tui";
          buildInputs = old.buildInputs ++ tuiDeps;
          cmakeFlags = depFlags ++ [ "-DDAEMON_APP_TUI=ON" ];
        });
      in
      {
        packages.default = daemon-app;
        packages.tui = daemon-tui;
        # Exposed for debugging the Meson dependency stack in isolation.
        packages.tuiwidgets = tuiwidgets-qt6;
        packages.posixsignalmanager = posixsignalmanager-qt6;

        apps.default = {
          type = "app";
          program = "${daemon-app}/bin/daemon-app";
          meta.description = "Run the daemon-app Qt Quick application";
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
      }
    );
}
