{
  description = "daemon-app - Qt 6 QML/C++ development environment, build, and vendored dependencies";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
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
    { self, nixpkgs, flake-utils, md4qt, earcut, ksyntaxhighlighting, microtex, posixsignalmanager, tuiwidgets, qwindowkit, qsimpleupdater, qautostart, qxtglobalshortcut, qmltermwidget, ... }:
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
          qtwebsockets    # Browser transport (native + wasm)
          qt5compat       # Core5Compat - required by the Qt6 build of Tui Widgets
        ];

        qtWasmHostPackages = with pkgs.qt6; [
          qtbase
          qtdeclarative
          qttools
          qtshadertools
          qtsvg
          qtwebsockets
        ];

        qtHostPrefix = pkgs.symlinkJoin {
          name = "qt-host-${pkgs.qt6.qtbase.version}";
          paths = qtWasmHostPackages;
        };

        # Qt 6.11.1 targets Emscripten 4.0.7; nixos-unstable currently carries a newer Emscripten,
        # so the wasm stack pins the compiler source explicitly instead of using pkgs.emscripten.
        emscripten_4_0_7 = pkgs.emscripten.overrideAttrs (old: rec {
          version = "4.0.7";
          src = pkgs.fetchFromGitHub {
            owner = "emscripten-core";
            repo = "emscripten";
            rev = version;
            hash = "sha256-0GezurqCblsvQPvXpUaHV5bFiUHa4aD3QXNhS4b9Ix4=";
          };
          patches = [ ];
          postPatch = (old.postPatch or "") + ''
            ${pkgs.python3}/bin/python - <<'PY'
from pathlib import Path

path = Path("tools/system_libs.py")
text = path.read_text()
old = """def copytree_exist_ok(src, dst):
  shutil.copytree(src, dst, dirs_exist_ok=True)
"""
new = """def copytree_exist_ok(src, dst):
  for root, dirs, files in os.walk(src):
    rel = os.path.relpath(root, src)
    out_dir = dst if rel == '.' else os.path.join(dst, rel)
    os.makedirs(out_dir, exist_ok=True)
    os.chmod(out_dir, 0o755)
    for name in dirs:
      target_dir = os.path.join(out_dir, name)
      os.makedirs(target_dir, exist_ok=True)
      os.chmod(target_dir, 0o755)
    for name in files:
      target = os.path.join(out_dir, name)
      shutil.copyfile(os.path.join(root, name), target)
      os.chmod(target, 0o644)
"""
if old not in text:
    raise SystemExit("copytree_exist_ok pattern not found")
path.write_text(text.replace(old, new))
PY
          '';
          doCheck = false;
          checkPhase = "true";
          installPhase =
            let
              beforeTest = builtins.elemAt (pkgs.lib.splitString "    export PYTHON=" old.installPhase) 0;
              afterTest = builtins.elemAt (
                pkgs.lib.splitString "    # fail if any .py files still have unpatched shebangs"
                  (builtins.elemAt (pkgs.lib.splitString "    export PYTHON=" old.installPhase) 1)
              ) 1;
            in
            beforeTest + "    # fail if any .py files still have unpatched shebangs" + afterTest;
          postInstall = (old.postInstall or "") + ''
            cat > "$out/.emscripten" <<'EOF'
EMSCRIPTEN_ROOT = 'share/emscripten'
EOF
          '';
        });

        wasmQtNativeBuildInputs = with pkgs; [
          cmake
          ninja
          perl
          python3
          emscripten_4_0_7
        ];

        qtbase-wasm = pkgs.stdenv.mkDerivation {
          pname = "qtbase-wasm";
          version = pkgs.qt6.qtbase.version;
          src = pkgs.qt6.qtbase.src;
          nativeBuildInputs = wasmQtNativeBuildInputs;
          EMSDK = "${emscripten_4_0_7}";
          configurePhase = ''
            runHook preConfigure
            export EM_CACHE="$TMPDIR/emscripten-cache"
            mkdir -p "$EM_CACHE"
            ./configure \
              -qt-host-path ${qtHostPrefix} \
              -platform wasm-emscripten \
              -prefix "$out" \
              -release \
              -opensource \
              -confirm-license \
              -no-feature-egl \
              -nomake examples \
              -nomake tests
            runHook postConfigure
          '';
          buildPhase = ''
            runHook preBuild
            cmake --build . --parallel 2
            runHook postBuild
          '';
          installPhase = ''
            runHook preInstall
            cmake --install .
            runHook postInstall
          '';
        };

        mkQtWasmModule =
          { pname, src, extraBuildInputs ? [ ] }:
          pkgs.stdenv.mkDerivation {
            inherit pname src;
            version = pkgs.qt6.qtbase.version;
            nativeBuildInputs = wasmQtNativeBuildInputs;
            buildInputs = [ qtbase-wasm ] ++ extraBuildInputs;
            EMSDK = "${emscripten_4_0_7}";
            enableParallelBuilding = false;
            preConfigure = ''
              export EM_CACHE="$TMPDIR/emscripten-cache"
              mkdir -p "$EM_CACHE"
            '';
            cmakeFlags = [
              "-DCMAKE_TOOLCHAIN_FILE=${qtbase-wasm}/lib/cmake/Qt6/qt.toolchain.cmake"
              "-DQT_HOST_PATH=${qtHostPrefix}"
              "-DCMAKE_PREFIX_PATH=${qtbase-wasm}"
              "-DCMAKE_INSTALL_PREFIX=${placeholder "out"}"
              "-DBUILD_TESTING=OFF"
            ];
          };

        qtshadertools-wasm = mkQtWasmModule {
          pname = "qtshadertools-wasm";
          src = pkgs.qt6.qtshadertools.src;
        };
        qtdeclarative-wasm = mkQtWasmModule {
          pname = "qtdeclarative-wasm";
          src = pkgs.qt6.qtdeclarative.src;
          extraBuildInputs = [ qtshadertools-wasm ];
        };
        qtsvg-wasm = mkQtWasmModule {
          pname = "qtsvg-wasm";
          src = pkgs.qt6.qtsvg.src;
          extraBuildInputs = [ qtdeclarative-wasm ];
        };
        qtwebsockets-wasm = mkQtWasmModule {
          pname = "qtwebsockets-wasm";
          src = pkgs.qt6.qtwebsockets.src;
          extraBuildInputs = [ qtdeclarative-wasm ];
        };

        qtWasmPackages = [
          qtbase-wasm
          qtshadertools-wasm
          qtdeclarative-wasm
          qtsvg-wasm
          qtwebsockets-wasm
        ];

        qtWasmPrefix = pkgs.symlinkJoin {
          name = "qt-wasm-${pkgs.qt6.qtbase.version}";
          paths = qtWasmPackages;
        };

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
        wasmDepFlags = [
          "-DMD4QT_SOURCE_DIR=${md4qt}"
          "-DEARCUT_SOURCE_DIR=${earcut}"
          "-DKSYNTAXHIGHLIGHTING_SOURCE_DIR=${ksyntaxhighlighting}"
          "-DMICROTEX_SOURCE_DIR=${microtex}"
          "-DQWINDOWKIT_SOURCE_DIR=${qwindowkit}"
          "-DQSIMPLEUPDATER_SOURCE_DIR=${qsimpleupdater}"
          "-DQAUTOSTART_SOURCE_DIR=${qautostart}"
          "-DQXTGLOBALSHORTCUT_SOURCE_DIR=${qxtglobalshortcut}"
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

        daemon-app-wasm = pkgs.stdenv.mkDerivation {
          pname = "daemon-app-wasm";
          version = baseVersion;
          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            pkg-config
            kdePackages.extra-cmake-modules
            perl
            emscripten_4_0_7
          ];

          buildInputs = qtWasmPackages ++ [ pkgs.tinyxml-2 ];

          QT_HOST_PATH = qtHostPrefix;
          QT_WASM_TOOLCHAIN = "${qtbase-wasm}/lib/cmake/Qt6/qt.toolchain.cmake";
          EMSDK = "${emscripten_4_0_7}";
          enableParallelBuilding = false;
          preConfigure = ''
            export EM_CACHE="$TMPDIR/emscripten-cache"
            mkdir -p "$EM_CACHE"
          '';

          cmakeFlags = wasmDepFlags ++ [
            "-DCMAKE_TOOLCHAIN_FILE=${./cmake/toolchains/wasm-emscripten.cmake}"
            "-DQt6_DIR=${qtWasmPrefix}/lib/cmake/Qt6"
            "-DQt6Core_DIR=${qtWasmPrefix}/lib/cmake/Qt6Core"
            "-DQt6CorePrivate_DIR=${qtWasmPrefix}/lib/cmake/Qt6CorePrivate"
            "-DQt6Gui_DIR=${qtWasmPrefix}/lib/cmake/Qt6Gui"
            "-DQt6GuiPrivate_DIR=${qtWasmPrefix}/lib/cmake/Qt6GuiPrivate"
            "-DQt6Widgets_DIR=${qtWasmPrefix}/lib/cmake/Qt6Widgets"
            "-DQt6PrintSupport_DIR=${qtWasmPrefix}/lib/cmake/Qt6PrintSupport"
            "-DQt6Network_DIR=${qtWasmPrefix}/lib/cmake/Qt6Network"
            "-DQt6Sql_DIR=${qtWasmPrefix}/lib/cmake/Qt6Sql"
            "-DQt6Test_DIR=${qtWasmPrefix}/lib/cmake/Qt6Test"
            "-DQt6OpenGL_DIR=${qtWasmPrefix}/lib/cmake/Qt6OpenGL"
            "-DQt6Svg_DIR=${qtWasmPrefix}/lib/cmake/Qt6Svg"
            "-DQt6BundledZLIB_DIR=${qtWasmPrefix}/lib/cmake/Qt6BundledZLIB"
            "-DQt6BundledPcre2_DIR=${qtWasmPrefix}/lib/cmake/Qt6BundledPcre2"
            "-DQt6BundledLibpng_DIR=${qtWasmPrefix}/lib/cmake/Qt6BundledLibpng"
            "-DQt6BundledLibjpeg_DIR=${qtWasmPrefix}/lib/cmake/Qt6BundledLibjpeg"
            "-DQt6BundledLibjpeg12bits_DIR=${qtWasmPrefix}/lib/cmake/Qt6BundledLibjpeg12bits"
            "-DQt6BundledLibjpeg16bits_DIR=${qtWasmPrefix}/lib/cmake/Qt6BundledLibjpeg16bits"
            "-DQt6BundledFreetype_DIR=${qtWasmPrefix}/lib/cmake/Qt6BundledFreetype"
            "-DQt6BundledHarfbuzz_DIR=${qtWasmPrefix}/lib/cmake/Qt6BundledHarfbuzz"
            "-DQt6BundledGlslang_Glslang_DIR=${qtWasmPrefix}/lib/cmake/Qt6BundledGlslang_Glslang"
            "-DQt6BundledGlslang_Spirv_DIR=${qtWasmPrefix}/lib/cmake/Qt6BundledGlslang_Spirv"
            "-DQt6BundledGlslang_Osdependent_DIR=${qtWasmPrefix}/lib/cmake/Qt6BundledGlslang_Osdependent"
            "-DQt6BundledSpirv_Cross_DIR=${qtWasmPrefix}/lib/cmake/Qt6BundledSpirv_Cross"
            "-DQt6ZlibPrivate_DIR=${qtWasmPrefix}/lib/cmake/Qt6ZlibPrivate"
            "-DQt6QmlIntegration_DIR=${qtWasmPrefix}/lib/cmake/Qt6QmlIntegration"
            "-DQt6Qml_DIR=${qtWasmPrefix}/lib/cmake/Qt6Qml"
            "-DQt6QmlCore_DIR=${qtWasmPrefix}/lib/cmake/Qt6QmlCore"
            "-DQt6QmlMeta_DIR=${qtWasmPrefix}/lib/cmake/Qt6QmlMeta"
            "-DQt6QmlNetwork_DIR=${qtWasmPrefix}/lib/cmake/Qt6QmlNetwork"
            "-DQt6QmlModels_DIR=${qtWasmPrefix}/lib/cmake/Qt6QmlModels"
            "-DQt6QmlLocalStorage_DIR=${qtWasmPrefix}/lib/cmake/Qt6QmlLocalStorage"
            "-DQt6QmlXmlListModel_DIR=${qtWasmPrefix}/lib/cmake/Qt6QmlXmlListModel"
            "-DQt6Quick_DIR=${qtWasmPrefix}/lib/cmake/Qt6Quick"
            "-DQt6QuickControls2_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickControls2"
            "-DQt6QuickControls2Impl_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickControls2Impl"
            "-DQt6QuickControls2Basic_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickControls2Basic"
            "-DQt6QuickControls2BasicStyleImpl_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickControls2BasicStyleImpl"
            "-DQt6QuickControls2Fusion_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickControls2Fusion"
            "-DQt6QuickControls2FusionStyleImpl_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickControls2FusionStyleImpl"
            "-DQt6QuickControls2Imagine_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickControls2Imagine"
            "-DQt6QuickControls2ImagineStyleImpl_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickControls2ImagineStyleImpl"
            "-DQt6QuickControls2Material_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickControls2Material"
            "-DQt6QuickControls2MaterialStyleImpl_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickControls2MaterialStyleImpl"
            "-DQt6QuickControls2Universal_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickControls2Universal"
            "-DQt6QuickControls2UniversalStyleImpl_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickControls2UniversalStyleImpl"
            "-DQt6QuickControls2FluentWinUI3StyleImpl_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickControls2FluentWinUI3StyleImpl"
            "-DQt6QuickLayouts_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickLayouts"
            "-DQt6QuickTemplates2_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickTemplates2"
            "-DQt6QuickDialogs2_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickDialogs2"
            "-DQt6QuickDialogs2QuickImpl_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickDialogs2QuickImpl"
            "-DQt6QuickDialogs2Utils_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickDialogs2Utils"
            "-DQt6QuickShapes_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickShapes"
            "-DQt6QuickShapesDesignHelpersPrivate_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickShapesDesignHelpersPrivate"
            "-DQt6QuickParticles_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickParticles"
            "-DQt6QuickParticlesPrivate_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickParticlesPrivate"
            "-DQt6QuickEffects_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickEffects"
            "-DQt6QuickTest_DIR=${qtWasmPrefix}/lib/cmake/Qt6QuickTest"
            "-DQt6LabsAnimation_DIR=${qtWasmPrefix}/lib/cmake/Qt6LabsAnimation"
            "-DQt6LabsFolderListModel_DIR=${qtWasmPrefix}/lib/cmake/Qt6LabsFolderListModel"
            "-DQt6LabsPlatform_DIR=${qtWasmPrefix}/lib/cmake/Qt6LabsPlatform"
            "-DQt6LabsQmlModels_DIR=${qtWasmPrefix}/lib/cmake/Qt6LabsQmlModels"
            "-DQt6LabsSettings_DIR=${qtWasmPrefix}/lib/cmake/Qt6LabsSettings"
            "-DQt6LabsStyleKit_DIR=${qtWasmPrefix}/lib/cmake/Qt6LabsStyleKit"
            "-DQt6LabsStyleKitImpl_DIR=${qtWasmPrefix}/lib/cmake/Qt6LabsStyleKitImpl"
            "-DQt6LabsSynchronizer_DIR=${qtWasmPrefix}/lib/cmake/Qt6LabsSynchronizer"
            "-DQt6LabsWavefrontMesh_DIR=${qtWasmPrefix}/lib/cmake/Qt6LabsWavefrontMesh"
            "-DQt6WebSockets_DIR=${qtWasmPrefix}/lib/cmake/Qt6WebSockets"
            "-DQt6LinguistTools_DIR=${pkgs.qt6.qttools}/lib/cmake/Qt6LinguistTools"
            "-DECM_DIR=${pkgs.kdePackages.extra-cmake-modules}/share/ECM/cmake"
            "-DCMAKE_PREFIX_PATH=${qtWasmPrefix}"
            "-DDAEMON_APP_WASM=ON"
            "-DDAEMON_APP_TUI=OFF"
            "-DBUILD_TESTING=OFF"
            "-DKF_IGNORE_PLATFORM_CHECK=ON"
            "-DCMAKE_EXE_LINKER_FLAGS=-sGLOBAL_BASE=16777216"
            "-DDAEMON_APP_VERSION_STR=${versionStr}"
          ];

          dontWrapQtApps = true;
        };
      in
      {
        packages.default = daemon-app;
        packages.tui = daemon-tui;
        packages.wasm = daemon-app-wasm;
        packages.qt-wasm = qtbase-wasm;
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

        devShells.wasm = pkgs.mkShell {
          packages = with pkgs; [
            cmake
            ninja
            pkg-config
            kdePackages.extra-cmake-modules
            perl
            python3
            nodejs
            just
            qt6.qttools
            emscripten_4_0_7
          ] ++ qtWasmPackages;

          shellHook = ''
            export QT_HOST_PATH="${qtHostPrefix}"
            export QT_WASM_TOOLCHAIN="${qtbase-wasm}/lib/cmake/Qt6/qt.toolchain.cmake"
            export EMSDK="${emscripten_4_0_7}"
            export Qt6_DIR="${qtbase-wasm}/lib/cmake/Qt6"
            export QT_PLUGIN_PATH="${pkgs.lib.makeSearchPath pkgs.qt6.qtbase.qtPluginPrefix qtWasmPackages}:$QT_PLUGIN_PATH"
            export QML_IMPORT_PATH="${pkgs.lib.makeSearchPath pkgs.qt6.qtbase.qtQmlPrefix qtWasmPackages}:$QML_IMPORT_PATH"
            export QML2_IMPORT_PATH="$QML_IMPORT_PATH:$QML2_IMPORT_PATH"
            export CMAKE_PREFIX_PATH="${pkgs.lib.makeSearchPath "lib/cmake" qtWasmPackages}:$CMAKE_PREFIX_PATH"
            export MD4QT_SOURCE_DIR="${md4qt}"
            export EARCUT_SOURCE_DIR="${earcut}"
            export KSYNTAXHIGHLIGHTING_SOURCE_DIR="${ksyntaxhighlighting}"
            export MICROTEX_SOURCE_DIR="${microtex}"
            export QWINDOWKIT_SOURCE_DIR="${qwindowkit}"
            export QSIMPLEUPDATER_SOURCE_DIR="${qsimpleupdater}"
            export QAUTOSTART_SOURCE_DIR="${qautostart}"
            export QXTGLOBALSHORTCUT_SOURCE_DIR="${qxtglobalshortcut}"
          '';
        };
      }
    );
}
