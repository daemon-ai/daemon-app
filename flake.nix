{
  description = "daemon-app - Qt 6 QML/C++ development environment, build, and vendored dependencies";

  # Pull built closures (incl. the heavy static-Qt stacks) from the daemon-ai Cachix cache
  # (public pull). CI feeds the cache via cachix-action; humans opt in with --accept-flake-config
  # (or as a trusted-user). Public pull key only — no secret lives here.
  nixConfig = {
    extra-substituters = [ "https://daemon-ai.cachix.org" ];
    extra-trusted-public-keys = [ "daemon-ai.cachix.org-1:jzeLmFDfgE5dzGT0RXF70IEU/tKsWdDV9LQ5zPGAnQs=" ];
  };

  inputs = {
    # logos-co fork of nixos-unstable carrying the MinGW Qt cross fixes the
    # Windows (mingw) outputs need (same Qt 6.11.1 as upstream unstable).
    nixpkgs.url = "github:logos-co/nixpkgs/mingw-integration";
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

    # QR module-matrix encoder for the generic auth component (Kit.QrCode + the
    # TUI half-block rendering). Nayuki's canonical upstream (MIT): two
    # dependency-free C++ files (cpp/qrcodegen.{hpp,cpp}) exposing the raw module
    # matrix via QrCode::getModule(x,y) - exactly what BOTH surfaces need. No C++
    # releases upstream, so pin a rev like the other vendored sources; built out
    # of the input by Dependencies.cmake (never committed into the repo).
    qrcodegen = {
      url = "github:nayuki/QR-Code-generator/2c9044de6b049ca25cb3cd1649ed7e27aa055138";
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

    # --- Mirror substrate (spec 09 §4, ADR-002/-008) -----------------------
    # Value-oriented immutable data structures + the severable reactive-core
    # header subset that back the client mirror store. All header-only and
    # BSL-1.0/MIT (see THIRD-PARTY-NOTICES.md), pinned like the other vendored
    # sources: `flake = false`, wired into the build via <DEP>_SOURCE_DIR by
    # cmake/Dependencies.cmake (header-only INTERFACE targets). Revs match the
    # references/ study clones so the ported behaviors track the audited code.
    #
    # immer 0.9.1+ (arximboldi/immer @ bd4fc74): normalized immer::table entity
    # tables + immer::diff. lager 0.1.3+ (arximboldi/lager @ 276c55a): ONLY the
    # reactive-core headers (state/commit/reader/cursor/writer/watch/setter +
    # lenses + detail/nodes + extra/qt) are compiled — store/effects/debugger
    # stay out (ADR-002). That subset needs zug (arximboldi/zug @ dd80433) and
    # header-only boost::intrusive; it does NOT need boost::hana (verified
    # references/architecture/lager/CMakeLists.txt: the INTERFACE `lager` target
    # links neither Boost nor hana; only tests/examples do).
    immer = {
      url = "github:arximboldi/immer/bd4fc749b97dfa2b66a8f3de00bbf234db4856ef";
      flake = false;
    };
    zug = {
      url = "github:arximboldi/zug/dd80433152c9fa5b16a710c8b530fb6131749132";
      flake = false;
    };
    lager = {
      url = "github:arximboldi/lager/276c55a20c675bd5a9b1cb3dd09263cba5632fa4";
      flake = false;
    };
  };

  outputs =
    { self, nixpkgs, nixpkgs-emscripten, flake-utils, md4qt, earcut, ksyntaxhighlighting, microtex, qrcodegen, posixsignalmanager, tuiwidgets, qwindowkit, qsimpleupdater, qautostart, qxtglobalshortcut, qmltermwidget, immer, zug, lager, ... }:
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

        # Qt modules, tiered by which closure actually needs them: everything in
        # a package's buildInputs is re-exported by wrapQtAppsHook into the
        # wrapped binary's plugin/QML search paths, so it ships in that
        # package's runtime closure. Keep each tier minimal.
        qtGuiPackages = with pkgs.qt6; [
          qtbase          # Core, Gui, Widgets, Network, Sql
          qtdeclarative   # Qml, Quick, QuickControls2, QuickTest
          qtshadertools
          qtsvg
          qtwebsockets    # WebSocket daemon transport (browser/wasm carrier, tested natively)
        ];
        # TUI-only runtime addition: the Qt6 build of Tui Widgets links Core5Compat.
        qtTuiPackages = with pkgs.qt6; [ qt5compat ];
        # The devShell further carries qttools (lupdate/lrelease for the
        # translation targets, Linguist, Qt Designer libs). The packages
        # instead pin Qt6LinguistTools_DIR in depFlags, keeping qttools'
        # libs out of the runtime closures.
        qtShellPackages = qtGuiPackages ++ qtTuiPackages ++ [ pkgs.qt6.qttools ];

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
          "-DQRCODEGEN_SOURCE_DIR=${qrcodegen}"
          "-DQWINDOWKIT_SOURCE_DIR=${qwindowkit}"
          "-DQSIMPLEUPDATER_SOURCE_DIR=${qsimpleupdater}"
          "-DQAUTOSTART_SOURCE_DIR=${qautostart}"
          "-DQXTGLOBALSHORTCUT_SOURCE_DIR=${qxtglobalshortcut}"
          "-DQMLTERMWIDGET_QML_DIR=${qmltermwidgetQmlDir}"
          # Mirror substrate (header-only): immer + zug + the lager reactive-core
          # subset. BOOST_INCLUDE_DIR feeds lager's boost::intrusive dependency
          # (header-only; the only Boost the compiled subset needs). See
          # cmake/Dependencies.cmake for the INTERFACE-target wiring.
          "-DIMMER_SOURCE_DIR=${immer}"
          "-DZUG_SOURCE_DIR=${zug}"
          "-DLAGER_SOURCE_DIR=${lager}"
          "-DBOOST_INCLUDE_DIR=${pkgs.boost.dev}/include"
          # Host Linguist tools (lupdate/lrelease for qt_add_translations),
          # pinned directly rather than listing qttools as an input: the qtbase
          # env hook folds every qttools input's plugin dir into the wrapped
          # app's QT_PLUGIN_PATH, dragging the Designer/Assistant libs into the
          # runtime closure for tools that only ever run at build time.
          "-DQt6LinguistTools_DIR=${pkgs.qt6.qttools}/lib/cmake/Qt6LinguistTools"
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
            # qttools (lupdate/lrelease for qt_add_translations) is deliberately
            # NOT an input; CMake gets it via the Qt6LinguistTools_DIR pin in
            # depFlags (see the comment there).
          ];

          # MicroTeX (LaTeX math renderer) links tinyxml2 via pkg-config.
          # qmltermwidget-qt6 ships the embedded-terminal QML plugin;
          # wrapQtAppsHook adds its lib/qml to the wrapped app's import path.
          # qtkeychain backs the OS-keychain server-token store (auth6); the build
          # falls back to a QSettings token store when it is absent.
          buildInputs = qtGuiPackages ++ [ pkgs.tinyxml-2 pkgs.qt6Packages.qtkeychain qmltermwidget-qt6 ];

          cmakeFlags = depFlags ++ [ "-DDAEMON_APP_VERSION_STR=${versionStr}" ];

          # The default fixup only runs `strip --strip-debug` over bin/, which
          # leaves the CMake Release binaries carrying full symbol tables
          # (~15 MB on daemon-app). Full-strip them; daemon-tui inherits this
          # via overrideAttrs. If symbolized crash reports are ever needed,
          # switch to RelWithDebInfo + separateDebugInfo instead.
          stripAllList = [ "bin" ];
        };

        # The experimental Tui Widgets TUI frontend (daemon-tui executable),
        # gated behind -DDAEMON_APP_TUI=ON. Reuses the GUI build, adding the TUI
        # dependency stack.
        daemon-tui = daemon-app.overrideAttrs (old: {
          pname = "daemon-tui";
          buildInputs = old.buildInputs ++ qtTuiPackages ++ tuiDeps;
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
          depSources = { inherit md4qt earcut ksyntaxhighlighting microtex qrcodegen; };
        };

        # --- Portable Linux desktop (static Qt) ------------------------------
        # The statically-linked-Qt GUI build + its patchelf'd/stripped
        # portable layout, tarball, and boot smoke (nix/portable.nix). Shares
        # the qt-from-source builder with the wasm stack; only these outputs
        # evaluate it, so the dynamic desktop outputs stay unchanged.
        portableStack = import ./nix/portable.nix {
          inherit pkgs versionStr baseVersion;
          appSrc = ./.;
          depSources = { inherit md4qt earcut ksyntaxhighlighting microtex qrcodegen; };
          # TUI + embedded-terminal sources for the static TUI / GUI builds
          # (mirrors the dynamic tuiwidgets/qmltermwidget derivations above).
          tuiSources = { inherit tuiwidgets posixsignalmanager qmltermwidget; };
        };

        # --- macOS desktop (static Qt, darwin-only) ----------------------------
        # Static-Qt macOS build mirroring the Linux portable stack: self-contained
        # .app with Qt compiled in (no macdeployqt), packaged as .dmg. Uses the
        # same qt-from-source.nix builder, flavor "macos-static". Pure (apple-sdk
        # from nixpkgs; no Xcode). Only evaluates on darwin.
        macosStack = pkgs.lib.optionalAttrs pkgs.stdenv.hostPlatform.isDarwin (
          import ./nix/macos.nix {
            inherit pkgs versionStr baseVersion;
            appSrc = ./.;
            depSources = { inherit md4qt earcut ksyntaxhighlighting microtex qrcodegen; };
            tuiSources = { inherit tuiwidgets posixsignalmanager qmltermwidget; };
          }
        );

        # --- Windows desktop (MinGW cross, static Qt) -------------------------
        # The x86_64-w64-mingw32 cross stack (pkgsCross.mingwW64 from the
        # logos-co fork) + the statically-linked daemon-app.exe, its portable
        # layout, the CPack NSIS installer, the PE sanity check, and the
        # best-effort wine smoke (nix/windows.nix). Only these outputs
        # evaluate it, so the native desktop outputs stay unchanged.
        windowsStack = import ./nix/windows.nix {
          inherit pkgs versionStr baseVersion;
          appSrc = ./.;
          depSources = { inherit md4qt earcut ksyntaxhighlighting microtex qrcodegen; };
        };

        # --- Qt for Android (arm64-v8a) ---------------------------------------
        # Cross-compiled Qt target stack via the shared qt-from-source builder
        # + the app cross-build, gradle-less APK assembly, sanity check, and
        # dev shell (nix/android.nix). The SDK/NDK (androidenv) are unfree, so
        # they come from a SECOND import of the same nixpkgs scoped to these
        # outputs only - the default `pkgs` (and with it every non-android
        # output) stays free.
        pkgsAndroid = import nixpkgs {
          inherit system;
          config = {
            allowUnfree = true;
            android_sdk.accept_license = true;
          };
        };
        qtAndroidStack = import ./nix/android.nix {
          inherit pkgs pkgsAndroid versionStr baseVersion;
          appSrc = ./.;
          depSources = { inherit md4qt earcut ksyntaxhighlighting microtex qrcodegen; };
        };

        # --- Qt for iOS simulator (arm64, static) -----------------------------
        # aarch64-darwin only: the iPhoneSimulator SDK lives in Xcode, not in
        # nixpkgs, so unlike the android stack this cannot be a pure derivation.
        # nix/ios.nix pins the Qt/tinyxml2 sources + host tools and exposes an
        # impure `qt-ios-builder` script (system Xcode via xcrun) plus the
        # devShell the app cross-configures in. Lazily forced only by the
        # darwin-gated outputs below, so the Linux package set is untouched.
        qtIosStack = import ./nix/ios.nix {
          inherit pkgs versionStr baseVersion;
          appSrc = ./.;
          depSources = { inherit md4qt earcut ksyntaxhighlighting microtex qrcodegen; };
        };

        # --- Linux packaging artifacts (CPack: DEB / RPM / AppImage) --------
        # The packaged payload is the STATIC-Qt app (portableStack.app): the
        # same build the portable tarball ships, packaged through the CPack
        # skeleton with the pre-build rewrite (cmake/PackagePortablePayload
        # .cmake) patching every staged ELF for the generic-distro floor -
        # generic /lib64 loader, $ORIGIN rpaths, store-prefix scrub, vendored
        # KSyntaxHighlighting side-payload pruned (the static binary compiles
        # it in). See cmake/Packaging.cmake + cmake/CPackPerGen.cmake for the
        # per-generator config and the hand-written system-floor Depends.
        appimageTooling = import ./nix/appimagetool.nix { inherit pkgs; };
        # CPack's AppImage generator needs CMake >= 4.2 (pinned nixpkgs ships
        # 4.1.2); nix/cmake-appimage.nix overrides the nixpkgs cmake with the
        # 4.2.7 source.
        cmake42 = import ./nix/cmake-appimage.nix { inherit pkgs; };

        # Build the STATIC app once with the packaging knobs, then run cpack
        # per generator from the same build tree. `bundledFrom` is the shape
        # the superproject fills: prebuilt sibling binaries ({ daemon;
        # daemon-infer; daemon-cli; } store paths) installed next to
        # bin/daemon-app (LocalDaemonLauncher discovers them there) plus
        # `libs` (runtime .so list for lib/, e.g. the libstdc++/libgomp pair
        # daemon-infer needs). Left empty here, so these artifacts package
        # the app alone.
        mkLinuxArtifacts =
          { bundledFrom ? { } }:
          portableStack.app.overrideAttrs (old: {
            pname = "daemon-linux-artifacts";

            # cmake42 must own the `cmake`/`cpack` names on PATH; drop the
            # pinned cmake from the inherited tool list instead of trusting
            # ordering. extra-cmake-modules is dropped too: its setup hook
            # appends absolute -DKDE_INSTALL_*=$out/... flags AFTER ours,
            # which would make the vendored KSyntaxHighlighting bypass the
            # relative-dir staging below (leaking store paths into the
            # packages); ECM_DIR in cmakeFlags replaces it for find_package.
            nativeBuildInputs =
              [ cmake42 ]
              ++ builtins.filter (
                p: !(builtins.elem (p.pname or "") [ "cmake" "extra-cmake-modules" ])
              ) old.nativeBuildInputs
              ++ [
                appimageTooling.appimagetool # cpack -G AppImage packs through it
                pkgs.patchelf # payload rewrite + AppImage RPATH handling
                pkgs.removeReferencesTo # store-prefix scrub in the payload rewrite
                pkgs.file
                pkgs.rpm # cpack -G RPM shells out to rpmbuild
                # (cpack -G DEB needs no dpkg: it writes the .deb natively)
              ];

            cmakeFlags = old.cmakeFlags ++ [
              # The nix cmake hook injects absolute store-path GNUInstallDirs
              # (-DCMAKE_INSTALL_BINDIR=$out/bin ...). Packages must stage
              # relative dirs under CPACK_PACKAGING_INSTALL_PREFIX instead,
              # so pin them back (later -D wins).
              "-DCMAKE_INSTALL_BINDIR=bin"
              "-DCMAKE_INSTALL_SBINDIR=sbin"
              "-DCMAKE_INSTALL_LIBDIR=lib"
              "-DCMAKE_INSTALL_LIBEXECDIR=libexec"
              "-DCMAKE_INSTALL_INCLUDEDIR=include"
              "-DCMAKE_INSTALL_DATADIR=share"
              "-DCMAKE_INSTALL_MANDIR=share/man"
              "-DCMAKE_INSTALL_INFODIR=share/info"
              "-DCMAKE_INSTALL_DOCDIR=share/doc/daemon-app"
              "-DCMAKE_INSTALL_LOCALEDIR=share/locale"
              # Pinned type2 runtime -> CPACK_APPIMAGE_RUNTIME_FILE (no
              # network at package time).
              "-DDAEMON_APP_APPIMAGE_RUNTIME=${appimageTooling.runtimeFile}"
              # find_package(ECM) without the ECM setup hook (see
              # nativeBuildInputs note above).
              "-DECM_DIR=${pkgs.kdePackages.extra-cmake-modules}/share/ECM/cmake"
              # Staged-payload rewrite for the generic-distro floor
              # (cmake/PackagePortablePayload.cmake): generic loader, $ORIGIN
              # rpaths, KSyntaxHighlighting side-payload prune, store-prefix
              # scrub of the static binary.
              "-DDAEMON_APP_PACKAGE_PORTABLE=ON"
              "-DDAEMON_APP_SCRUB_PREFIXES=${pkgs.lib.concatStringsSep ";" portableStack.scrubPrefixes}"
            ]
            ++ pkgs.lib.optional (bundledFrom ? daemon) "-DDAEMON_APP_BUNDLED_DAEMON=${bundledFrom.daemon}"
            ++ pkgs.lib.optional (bundledFrom ? daemon-infer)
              "-DDAEMON_APP_BUNDLED_DAEMON_INFER=${bundledFrom.daemon-infer}"
            ++ pkgs.lib.optional (bundledFrom ? daemon-cli)
              "-DDAEMON_APP_BUNDLED_DAEMON_CLI=${bundledFrom.daemon-cli}"
            ++ pkgs.lib.optional (bundledFrom ? libs)
              "-DDAEMON_APP_BUNDLED_LIBS=${pkgs.lib.concatStringsSep ";" bundledFrom.libs}";

            # The output is the artifact set, not an installed app tree:
            # nothing to wrap, and fixup must not patchelf/strip the finished
            # packages (it would rewrite the AppImage's embedded runtime ELF).
            dontWrapQtApps = true;
            dontFixup = true;
            installPhase = ''
              runHook preInstall

              # rpmbuild resolves %_topdir & friends under $HOME.
              export HOME="$TMPDIR"

              run_cpack() {
                echo "=== cpack -G $1 ==="
                cpack -G "$1" || {
                  echo "--- cpack $1 failed; dumping logs ---" >&2
                  find _CPack_Packages -name '*.log' -o -name '*.err' | while read -r f; do
                    echo "--- $f ---" >&2
                    tail -n 100 "$f" >&2
                  done
                  exit 1
                }
              }
              run_cpack DEB
              run_cpack RPM
              run_cpack AppImage

              mkdir -p "$out"
              # One artifact of each format must exist (deb naming uses
              # underscores - DEB-DEFAULT - so match on suffix only; ls fails
              # on an unmatched glob).
              for pattern in '*.deb' '*.rpm' '*.AppImage'; do
                ls ./$pattern > /dev/null || {
                  echo "missing artifact: $pattern" >&2
                  exit 1
                }
              done
              cp -v ./*.deb ./*.rpm ./*.AppImage ./*.sha256 "$out"/
              # zsyncmake control file (appimagetool emits it next to the
              # staged AppImage when zsync update info is embedded).
              find _CPack_Packages -name '*.zsync' -exec cp -v {} "$out"/ \; || true

              runHook postInstall
            '';
          });

        linuxArtifacts = mkLinuxArtifacts { };

        # AppImage SelfApply A->B end-to-end harness (nix/appimage-e2e.nix): a
        # parameterized test-AppImage builder (test pubkey + arbitrary version,
        # SelfApply) plus the impure `nix run` runner that drives the real swap.
        # mkImage is surfaced as a flake output so the runner can build A + B
        # with a runtime-generated pubkey via getFlake.
        appimageE2E = import ./nix/appimage-e2e.nix {
          inherit pkgs self system mkLinuxArtifacts;
          lib = pkgs.lib;
          bootLibPath = portableStack.bootLibPath;
          runtimeFile = appimageTooling.runtimeFile;
        };

        # --- macOS packaging artifact (CPack DragNDrop -> .dmg) -------------
        # Darwin mirror of mkLinuxArtifacts: same build with the packaging
        # knobs, then cpack -G DragNDrop from the build tree (the bundle,
        # Info.plist, Qt deploy script, and DMG config all live in
        # cmake/Packaging.cmake + cmake/CPackPerGen.cmake). Same `bundledFrom`
        # contract: prebuilt sibling binaries land in Contents/MacOS next to
        # the app executable.
        #
        # VALIDATED on an aarch64-darwin (M1) host: the pure `nix build`
        # path produces a mountable, codesign-clean DMG, and cpack's
        # DragNDrop hdiutil call works inside the darwin nix build sandbox.
        # See packaging/macos/README.md for the verification runbook and
        # the remaining manual steps (hardened-runtime signing, notarization).
        mkDarwinArtifacts =
          { bundledFrom ? { } }:
          daemon-app.overrideAttrs (old: {
            pname = "daemon-macos-artifacts";

            # Same ECM setup-hook eviction as mkLinuxArtifacts: its absolute
            # -DKDE_INSTALL_* flags would bypass the relative-dir staging and
            # leak store paths into the DMG; ECM_DIR replaces find_package.
            nativeBuildInputs = builtins.filter (
              p: (p.pname or "") != "extra-cmake-modules"
            ) old.nativeBuildInputs;

            cmakeFlags = old.cmakeFlags ++ [
              # Pin the nix cmake hook's absolute store-path GNUInstallDirs
              # back to relative dirs (later -D wins), mirroring
              # mkLinuxArtifacts: everything cpack stages must sit under the
              # DMG root, not under $out.
              "-DCMAKE_INSTALL_BINDIR=bin"
              "-DCMAKE_INSTALL_SBINDIR=sbin"
              "-DCMAKE_INSTALL_LIBDIR=lib"
              "-DCMAKE_INSTALL_LIBEXECDIR=libexec"
              "-DCMAKE_INSTALL_INCLUDEDIR=include"
              "-DCMAKE_INSTALL_DATADIR=share"
              "-DCMAKE_INSTALL_MANDIR=share/man"
              "-DCMAKE_INSTALL_INFODIR=share/info"
              "-DCMAKE_INSTALL_DOCDIR=share/doc/daemon-app"
              "-DCMAKE_INSTALL_LOCALEDIR=share/locale"
              "-DECM_DIR=${pkgs.kdePackages.extra-cmake-modules}/share/ECM/cmake"
              # Auto-updater dials (packaging/UPDATES.md): SelfApply for the DMG —
              # hdiutil stage + quarantine strip + guards + daemon-updater two-move
              # swap, validated E2E on the M1 (packaging/macos/README.md).
              "-DDAEMON_APP_UPDATE_CAPABILITY=SelfApply"
              "-DDAEMON_APP_UPDATE_FEED_URL=https://github.com/daemon-ai/daemon/releases/latest/download/manifest.json"
              "-DDAEMON_APP_UPDATE_PUBKEY=RWRXpowS90Fy+TYhRsrBbQNSDvjbtJpqi9T89OGqSNTLkOa5vn62hK0o"
              "-DDAEMON_APP_UPDATE_ARTIFACT_KIND=dmg"
            ]
            ++ pkgs.lib.optional (bundledFrom ? daemon) "-DDAEMON_APP_BUNDLED_DAEMON=${bundledFrom.daemon}"
            ++ pkgs.lib.optional (bundledFrom ? daemon-infer)
              "-DDAEMON_APP_BUNDLED_DAEMON_INFER=${bundledFrom.daemon-infer}"
            ++ pkgs.lib.optional (bundledFrom ? daemon-cli)
              "-DDAEMON_APP_BUNDLED_DAEMON_CLI=${bundledFrom.daemon-cli}";

            # The base daemon-app stdenv is pkgs.ccacheStdenv, whose compiler
            # links hardcode + export CCACHE_DIR=/var/cache/ccache. That cache
            # dir is provisioned on the Linux CI (programs.ccache, added to the
            # sandbox) but does not exist on a fresh mac, so every compile
            # aborts with "ccache: error: Permission denied". The DMG is a
            # one-shot build with no incremental reuse, so disable ccache and
            # let the wrapper exec the real clang directly.
            CCACHE_DISABLE = "1";

            # The output is the .dmg artifact set: nothing to wrap, and fixup
            # must not strip the deployed bundle (macdeployqt ad-hoc-signs
            # after rewriting install names; stripping breaks the seal).
            dontWrapQtApps = true;
            dontFixup = true;
            installPhase = ''
              runHook preInstall

              # cpack's DragNDrop generator and the macdeployqt deploy script
              # shell out to Apple system tools - hdiutil (DMG creation),
              # codesign (ad-hoc bundle seal), SetFile - that live in /usr/bin
              # + /usr/sbin, absent from the nix build PATH. This mac's daemon
              # build user has sandbox disabled and hdiutil was probed to work
              # there once findable, so append (not prepend, so nix's own
              # cctools still win) the system bindirs for the packaging step
              # only; the compile stays hermetic.
              export PATH="$PATH:/usr/bin:/usr/sbin"

              echo "=== cpack -G DragNDrop ==="
              cpack -G DragNDrop || {
                echo "--- cpack DragNDrop failed; dumping logs ---" >&2
                find _CPack_Packages -name '*.log' -o -name '*.err' | while read -r f; do
                  echo "--- $f ---" >&2
                  tail -n 100 "$f" >&2
                done
                exit 1
              }

              mkdir -p "$out"
              ls ./*.dmg > /dev/null || {
                echo "missing artifact: *.dmg" >&2
                exit 1
              }
              cp -v ./*.dmg ./*.sha256 "$out"/

              runHook postInstall
            '';
          });

        darwinArtifacts = mkDarwinArtifacts { };

        # Single-format views of the artifact set (`nix build .#deb` etc.).
        selectArtifact =
          name: glob:
          pkgs.runCommand "daemon-${name}" { } ''
            mkdir -p "$out"
            cp -v ${linuxArtifacts}/${glob} "$out"/
            cp -v ${linuxArtifacts}/${glob}.sha256 "$out"/ 2>/dev/null || true
          '';

        # Sanity gate over all three artifacts: metadata + payload layout
        # assertions, the portable ELF contract + CLOSURE COMPLETENESS on
        # every shipped ELF (each DT_NEEDED must resolve in-payload through
        # the $ORIGIN rpaths or sit on the excludelist-derived system floor -
        # same allowlist as checks.portable-boot-smoke; the class of loader
        # failure this kills: a user-reported libxcb-cursor.so.0 crash from
        # an AppImage whose binary still dynamically linked the xcb-util
        # family that no distro guarantees), desktop/metainfo validation, an
        # AppImage payload extract + offscreen boot probe
        # (DAEMON_APP_WAIT_READY contract from src/DaemonApp/App/main.cpp;
        # booted through the store glibc's loader since the sandbox has no
        # /lib64), and the glibc symbol-version floor.
        artifactSanity =
          pkgs.runCommand "daemon-artifact-sanity"
            {
              nativeBuildInputs = with pkgs; [
                dpkg # dpkg-deb --info/--contents/-x
                rpm # rpm -qpl, rpm2cpio
                cpio
                binutils # readelf (ELF contract + glibc floor)
                patchelf # interpreter/rpath asserts
                desktop-file-utils
                appstream # appstreamcli validate
                squashfsTools # unsquashfs (AppImage payload extraction)
              ];
            }
            ''
              set -euo pipefail
              mkdir work && cd work

              # Closure-completeness gate over one extracted payload root:
              # every ELF under bin/ or shipped as lib/*.so* must (a) carry
              # the portable rpath contract the pre-build script writes
              # (bin: generic /lib64 loader + $ORIGIN:$ORIGIN/../lib; lib:
              # $ORIGIN), and (b) have every DT_NEEDED soname either present
              # in-payload (resolvable through those rpaths) or on the
              # explicit system floor below. daemon-app additionally must
              # not re-grow a libstdc++ dependency (-static-libstdc++).
              floor="${pkgs.lib.concatStringsSep " " portableStack.allowedNeeded}"
              check_payload_closure() {
                label="$1"; root="$2"
                echo "--- closure completeness: $label ---"
                shipped=$(find "$root" -name '*.so*' -type f -exec basename {} \; | sort -u)
                fail=0
                while IFS= read -r -d "" f; do
                  [ "$(head -c4 "$f" | od -An -tx1 | tr -d ' ')" = "7f454c46" ] || continue
                  case "$f" in
                    */bin/*)
                      [ "$(patchelf --print-interpreter "$f")" = "/lib64/ld-linux-x86-64.so.2" ] \
                        || { echo "FAIL($label): $f keeps a non-generic interpreter" >&2; fail=1; }
                      [ "$(patchelf --print-rpath "$f")" = '$ORIGIN:$ORIGIN/../lib' ] \
                        || { echo "FAIL($label): $f has unexpected rpath" >&2; fail=1; }
                      ;;
                    *)
                      [ "$(patchelf --print-rpath "$f")" = '$ORIGIN' ] \
                        || { echo "FAIL($label): $f has unexpected rpath" >&2; fail=1; }
                      ;;
                  esac
                  readelf -d "$f" | awk '/NEEDED/ { gsub(/[][]/, "", $5); print $5 }' > needed-one.txt
                  while read -r so; do
                    if printf '%s\n' "$shipped" | grep -qxF "$so"; then continue; fi
                    case " $floor " in
                      *" $so "*) ;;
                      *)
                        echo "FAIL($label): $f needs '$so' - neither shipped in-payload nor on the system floor" >&2
                        fail=1
                        ;;
                    esac
                  done < needed-one.txt
                  if [ "$(basename "$f")" = "daemon-app" ] && grep -q 'libstdc++' needed-one.txt; then
                    echo "FAIL($label): libstdc++ leaked into daemon-app's DT_NEEDED" >&2
                    fail=1
                  fi
                done < <(find "$root" \( -path '*/bin/*' -o -name '*.so*' \) -type f -print0)
                [ "$fail" = 0 ] || exit 1
                echo "closure OK: $label"
              }

              echo "=== DEB: metadata + contents ==="
              deb=$(ls ${linuxArtifacts}/*.deb)
              dpkg-deb --info "$deb" | tee deb-info.txt
              grep -q "Package: daemon" deb-info.txt
              grep -q "Version: ${baseVersion}" deb-info.txt
              dpkg-deb --contents "$deb" > deb-contents.txt
              for path in \
                ./opt/daemon/bin/daemon-app \
                ./opt/daemon/share/applications/daemon-app.desktop \
                ./opt/daemon/share/metainfo/io.daemon.app.metainfo.xml \
                ./opt/daemon/share/icons/hicolor/256x256/apps/daemon-app.png \
                ./opt/daemon/share/icons/hicolor/scalable/apps/daemon-app.svg \
                ./opt/daemon/share/daemon-app/microtex-res/RES_README \
                ./opt/daemon/share/doc/daemon-app/LICENSE \
                ./opt/daemon/share/doc/daemon-app/THIRD-PARTY-NOTICES.md; do
                grep -qF " $path" deb-contents.txt || { echo "missing in deb: $path" >&2; exit 1; }
              done
              # The static payload compiles KSyntaxHighlighting in; its
              # vendored side-payload (headers, CLI, static archives, QML
              # scaffolding) must have been pruned at staging.
              for stray in libKF6SyntaxHighlighting ksyntaxhighlighter6 include/KF6 '\.a$'; do
                if grep -E "$stray" deb-contents.txt; then
                  echo "deb payload carries pruned KSyntaxHighlighting cruft: $stray" >&2
                  exit 1
                fi
              done
              # Everything must relocate under /opt/daemon: a store path here
              # means an install rule bypassed the relative-dir staging (the
              # ECM setup-hook leak this build guards against).
              if grep -q "nix/store" deb-contents.txt; then
                echo "deb payload contains /nix/store paths" >&2
                exit 1
              fi
              dpkg-deb --ctrl-tarfile "$deb" | tar -t > deb-control.txt
              grep -q "postinst" deb-control.txt
              grep -q "prerm" deb-control.txt
              dpkg-deb -x "$deb" deb-root
              desktop-file-validate deb-root/opt/daemon/share/applications/daemon-app.desktop
              appstreamcli validate --no-net \
                deb-root/opt/daemon/share/metainfo/io.daemon.app.metainfo.xml

              echo "=== DEB payload: portable ELF contract + closure ==="
              bin=deb-root/opt/daemon/bin/daemon-app
              echo "interpreter: $(patchelf --print-interpreter "$bin")"
              echo "rpath: $(patchelf --print-rpath "$bin")"
              readelf -d "$bin" | awk '/NEEDED/ { gsub(/[][]/, "", $5); print "NEEDED " $5 }'
              check_payload_closure deb deb-root/opt/daemon

              echo "=== RPM: metadata + contents ==="
              rpmfile=$(ls ${linuxArtifacts}/*.rpm)
              # --dbpath: no /var/lib/rpm in the sandbox
              rpmq() { rpm --dbpath "$PWD/rpmdb" --nosignature "$@"; }
              rpmq -qpi "$rpmfile" | tee rpm-info.txt
              grep -q "^Name *: daemon" rpm-info.txt
              rpmq -qpl "$rpmfile" > rpm-list.txt
              for path in \
                /opt/daemon/bin/daemon-app \
                /opt/daemon/share/applications/daemon-app.desktop \
                /opt/daemon/share/metainfo/io.daemon.app.metainfo.xml \
                /opt/daemon/share/icons/hicolor/256x256/apps/daemon-app.png; do
                grep -qxF "$path" rpm-list.txt || { echo "missing in rpm: $path" >&2; exit 1; }
              done
              if grep -Eq "libKF6SyntaxHighlighting|ksyntaxhighlighter6|include/KF6|\.a$" rpm-list.txt; then
                echo "rpm payload carries pruned KSyntaxHighlighting cruft" >&2
                exit 1
              fi
              if grep -q "nix/store" rpm-list.txt; then
                echo "rpm payload contains /nix/store paths" >&2
                exit 1
              fi
              rpmq -qp --scripts "$rpmfile" | grep -q "/usr/local/bin"
              mkdir rpm-root && (cd rpm-root && rpm2cpio "$rpmfile" | cpio -idm --quiet)
              check_payload_closure rpm rpm-root/opt/daemon

              echo "=== AppImage: extract + layout ==="
              appimage=$(ls ${linuxArtifacts}/*.AppImage)
              # Host binfmt_misc registrations (e.g. NixOS appimage-run) leak
              # into the build sandbox and intercept executing the AppImage
              # (even `--appimage-extract`), so unpack the squashfs payload at
              # the embedded runtime's byte size instead. The type2 magic
              # ("AI\x02" at ELF offset 8) plus the squashfs superblock
              # sitting exactly at the pinned runtime's size also pin the
              # embedded runtime's provenance (appimagetool patches
              # update-info/digest sections into it, so a full byte compare
              # is not possible).
              runtime_size=$(stat -c%s ${appimageTooling.runtimeFile})
              [ "$(dd if="$appimage" bs=1 skip=8 count=3 status=none | od -An -tx1)" = " 41 49 02" ] || {
                echo "not a type2 AppImage (missing AI magic)" >&2
                exit 1
              }
              [ "$(dd if="$appimage" bs=1 skip="$runtime_size" count=4 status=none)" = "hsqs" ] || {
                echo "squashfs payload not at pinned runtime offset $runtime_size" >&2
                exit 1
              }
              unsquashfs -q -d squashfs-root -o "$runtime_size" "$appimage"
              test -x squashfs-root/AppRun
              test -f squashfs-root/usr/bin/daemon-app
              test -e squashfs-root/daemon-app.desktop
              test -f squashfs-root/usr/share/metainfo/io.daemon.app.metainfo.xml
              check_payload_closure appimage squashfs-root/usr

              echo "=== AppImage: offscreen boot probe (static payload) ==="
              export HOME="$PWD/home" && mkdir -p "$HOME"
              export XDG_RUNTIME_DIR="$PWD/xdg" && mkdir -m 0700 -p "$XDG_RUNTIME_DIR"
              export QT_QPA_PLATFORM=offscreen
              export QT_QUICK_BACKEND=software
              # Mock service mode: the WAIT_READY probe drives a first-run
              # "local" connect, which the mock accepts iff the target file
              # exists - no daemon binary needed (none is bundled here).
              export DAEMON_APP_SERVICE_MODE=mock
              export DAEMON_APP_SOCKET="$PWD/mock-daemon.sock" && touch "$DAEMON_APP_SOCKET"
              export DAEMON_APP_WAIT_READY=20000
              # The payload binary is fully self-contained (static Qt: no
              # QT_PLUGIN_PATH / QML import path needed) but its generic
              # /lib64 interpreter does not exist in the build sandbox, so
              # boot it through the store glibc's loader with the portable
              # smoke's library path - exactly what portable-boot-smoke does.
              ${pkgs.glibc}/lib/ld-linux-x86-64.so.2 \
                --library-path "${portableStack.bootLibPath}" \
                squashfs-root/usr/bin/daemon-app | tee boot.txt
              grep -q "DAEMON_APP_READY ok" boot.txt

              echo "=== glibc floor over all extracted payloads ==="
              bash ${./scripts/glibc-floor.sh} deb-root rpm-root squashfs-root

              touch "$out"
            '';
      in
      {
        packages.default = daemon-app;
        packages.tui = daemon-tui;
        # Exposed for debugging the Meson dependency stack in isolation.
        packages.tuiwidgets = tuiwidgets-qt6;
        packages.posixsignalmanager = posixsignalmanager-qt6;

        # Linux packaging artifacts (static-Qt payload; see mkLinuxArtifacts).
        # `artifacts` carries all three; the per-format outputs are views of
        # the same build.
        packages.artifacts = linuxArtifacts;
        packages.appimage = selectArtifact "appimage" "*.AppImage";
        packages.deb = selectArtifact "deb" "*.deb";
        packages.rpm = selectArtifact "rpm" "*.rpm";
        # macOS DMG, exposed on the darwin systems only (best-effort: eval
        # only, never built - no darwin host in this repo's loop yet). The
        # null dynamic attr name elides the attribute entirely on Linux, so
        # the Linux package set is byte-identical and this dotted packages.*
        # block needs no restructuring into a single literal (which is what
        # lib.optionalAttrs merging would force, and what the sibling
        # packaging branches also append to).
        packages.${if pkgs.stdenv.hostPlatform.isDarwin then "macos-dmg" else null} =
          macosStack.dmg;
        packages.${if pkgs.stdenv.hostPlatform.isDarwin then "qt-macos-static" else null} =
          macosStack.qtStatic;
        packages.${if pkgs.stdenv.hostPlatform.isDarwin then "app-static-darwin" else null} =
          macosStack.app;
        # Exposed for debugging the packaging toolchain in isolation.
        packages.appimagetool = appimageTooling.appimagetool;
        packages.cmake-appimage = cmake42;

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

        # Portable Linux desktop: the static-Qt GUI in a distributable layout
        # (bin/daemon-app patchelf'd to the generic /lib64 loader + stripped,
        # share/daemon-app/microtex-res, THIRD-PARTY-NOTICES) and the .tar.zst
        # of exactly that tree. `qt-linux-static` exposes the joined Qt prefix
        # for debugging the stack in isolation (per-module outputs live in
        # nix/portable.nix).
        packages.portable = portableStack.portable;
        packages.portable-tarball = portableStack.tarball;
        packages.qt-linux-static = portableStack.qtStatic;
        # The raw static GUI+TUI app (nix-store loader; runs on NixOS) - what the
        # superproject bundle consumes, distinct from the patchelf'd `portable`.
        packages.app-static = portableStack.app;
        # The static TUI binary alone (from the same build): a thin view for the
        # superproject's `bundledTui` to point at without pulling the GUI.
        packages.tui-static = pkgs.runCommand "daemon-tui-static-${baseVersion}" { } ''
          mkdir -p "$out/bin"
          ln -s ${portableStack.app}/bin/daemon-tui "$out/bin/daemon-tui"
        '';
        # Static desktop deps exposed for isolated debugging / cache seeding.
        packages.qtkeychain-static = portableStack.qtkeychain;
        packages.tuiwidgets-static = portableStack.tuiwidgets;
        packages.qmltermwidget-static = portableStack.qmltermwidget;

        # Windows desktop (MinGW cross, static Qt): the portable layout
        # (bin/daemon-app.exe + share/daemon-app/microtex-res + notices) and
        # the CPack NSIS installer. `qt-mingw-static` exposes the joined Qt
        # prefix for debugging the stack in isolation.
        packages.windows-portable = windowsStack.portable;
        packages.nsis = windowsStack.nsis;
        packages.qt-mingw-static = windowsStack.qtMingw;
        # The (WoW64) wine used by apps.windows-smoke, exposed so the superproject's
        # composed E2E smoke runs the same hermetic prelude over the bundled installer
        # with the exact same nix-provided wine (fork nixpkgs) rather than a second pin.
        packages.windows-smoke-wine = windowsStack.wine;

        # Android (arm64-v8a): the debug-signed APK, the joined Qt-for-Android
        # prefix (bin/qt-cmake + lib/cmake/Qt6/qt.toolchain.cmake), and the
        # androiddeployqt-staged package tree (nix/android.nix). The APK is a
        # thin remote/WebSocket client like the wasm build - no daemon is
        # bundled (packaging/android/README.md).
        packages.apk = qtAndroidStack.apk;
        packages.qt-android = qtAndroidStack.qtAndroid;
        packages.qtbase-android = qtAndroidStack.qtbase;
        packages.qtshadertools-android = qtAndroidStack.qtshadertools;
        packages.qtdeclarative-android = qtAndroidStack.qtdeclarative;
        packages.qtsvg-android = qtAndroidStack.qtsvg;
        packages.qtwebsockets-android = qtAndroidStack.qtwebsockets;
        packages.android-staged = qtAndroidStack.app;

        # iOS simulator (aarch64-darwin only, elided on Linux via the null
        # dynamic attr name - same pattern as macos-dmg). `qt-ios-builder` is
        # the impure Qt-for-iOS-simulator build script (run it on the mac:
        # `nix run .#qt-ios-builder`); `qt-ios-host` is the host-tools prefix
        # exposed for debugging. The Qt libs themselves cannot be a nix package
        # (impure Xcode SDK), so there is no qtbase-ios/qt-ios output.
        packages.${if pkgs.stdenv.hostPlatform.isDarwin then "qt-ios-builder" else null} =
          qtIosStack.qtIosBuilder;
        packages.${if pkgs.stdenv.hostPlatform.isDarwin then "qt-ios-host" else null} =
          qtIosStack.qtHost;

        # Proves the wasm stack end-to-end without the app: a static Qt Quick
        # hello-world through the joined prefix's qt-cmake, asserting the
        # .wasm/.js/.html artifact set.
        checks.qt-wasm-smoke = qtWasmStack.smoke;

        # Portable contract check (sandbox-safe half): ELF loader/rpath/
        # DT_NEEDED-floor asserts, the glibc-floor readout, and an offscreen
        # DAEMON_APP_WAIT_READY boot through the store loader. The FHS-root
        # boot (needs user namespaces, so it cannot run in the build sandbox)
        # is apps.portable-smoke below.
        checks.portable-boot-smoke = portableStack.bootSmoke;

        # macOS static boot smoke (darwin only): otool check + offscreen boot.
        checks.${if pkgs.stdenv.hostPlatform.isDarwin then "macos-boot-smoke" else null} =
          macosStack.bootSmoke;

        # Packaging artifact gate: DEB/RPM metadata + payload assertions,
        # desktop/metainfo validation, AppImage payload extract + offscreen
        # boot, glibc symbol-version floor (scripts/glibc-floor.sh).
        checks.artifact-sanity = artifactSanity;

        # Windows artifact gate: PE32+ magic + GUI subsystem, the DLL import
        # floor (Windows inbox DLLs only), the embedded icon resource, and
        # NSIS installer existence/size. Wine is deliberately NOT part of
        # this check - the boot smoke is apps.windows-smoke below.
        checks.windows-sanity = windowsStack.sanity;

        # APK structural gate: manifest identity (package id, versions, sdk
        # floor), native lib set, MicroTeX assets, Qt resource bundle,
        # apksigner verification (nix/android.nix).
        checks.apk-sanity = qtAndroidStack.apkSanity;

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

        # Boot the portable binary inside a buildFHSEnv simulating a generic
        # distro root (real /lib64 loader + X/GL floor, no Nix store deps) -
        # the interactive half of the portable smoke that the build sandbox
        # cannot run (buildFHSEnv needs user namespaces).
        apps.portable-smoke = {
          type = "app";
          program = "${portableStack.fhsSmoke}/bin/portable-smoke";
          meta.description = "Boot the portable (static Qt) build in a generic FHS root";
        };

        # Best-effort wine smoke for the Windows build: offscreen WAIT_READY
        # boot of daemon-app.exe + a silent NSIS install into a scratch
        # prefix. Wine is emulation, not Windows: failures here are recorded,
        # not gating (`nix run .#windows-smoke`).
        apps.windows-smoke = {
          type = "app";
          program = "${windowsStack.smoke}/bin/windows-smoke";
          meta.description = "Boot the Windows (MinGW) build + NSIS installer under wine";
        };

        # AppImage SelfApply A->B swap E2E (`nix run .#updater-appimage-e2e`):
        # builds two real AppImages (test pubkey, SelfApply), serves a signed
        # feed for the newer one on loopback, runs the older one offscreen, and
        # asserts the daemon-updater helper swapped the on-disk .AppImage +
        # relaunched the new version. Impure (host nix + loopback), like the
        # portable/windows smokes; not a flake check.
        apps.updater-appimage-e2e = {
          type = "app";
          program = "${appimageE2E.runner}/bin/updater-appimage-e2e";
          meta.description = "End-to-end AppImage self-apply A->B swap against a local signed feed";
        };

        # The parameterized test-AppImage builder, surfaced so the E2E runner can
        # build A + B with a runtime-generated test pubkey (getFlake + call). Not
        # a package (it is a function); built on demand by the runner.
        appimageSelfApplyImage = appimageE2E.mkImage;

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
            # --- release-feed tooling (scripts/release-manifest.sh + sync-updates-json.sh) ---
            minisign # Ed25519 manifest signing/verification (packaging/UPDATES.md)
            jq # manifest.json assembly + UPDATES.json mirror sync
            shellcheck-minimal # lint for scripts/*.sh (doc-free: avoids pandoc/haddock build chain)
            nodejs # provides npx for jscpd duplicate detection (not packaged in nixpkgs)
            just # task runner: the justfile recipes (lint / build / qmllint)
            qt6Packages.qtkeychain # OS keychain for the server-token store (auth6)
          ] ++ qtShellPackages ++ tuiDeps ++ [ qmltermwidget-qt6 ];

          shellHook = ''
            export QT_PLUGIN_PATH="${pkgs.lib.makeSearchPath pkgs.qt6.qtbase.qtPluginPrefix qtShellPackages}:$QT_PLUGIN_PATH"
            export QML_IMPORT_PATH="${pkgs.lib.makeSearchPath pkgs.qt6.qtbase.qtQmlPrefix qtShellPackages}:${qmltermwidgetQmlDir}:$QML_IMPORT_PATH"
            export QML2_IMPORT_PATH="$QML_IMPORT_PATH:$QML2_IMPORT_PATH"
            # nixpkgs' patched Qt resolves QML modules through NIXPKGS_QT6_QML_IMPORT_PATH ahead of
            # the standard import paths. A NixOS desktop session (e.g. Plasma) exports it with the
            # HOST Qt's module dirs, which then shadow this shell's pinned Qt for any dev-built
            # binary run in here — the GUI dies at QML root load with "module
            # QtQuick.Controls.Basic version X.Y is not installed". Prefix the pinned dirs, exactly
            # like the installed app's binary wrapper does for the same variable.
            export NIXPKGS_QT6_QML_IMPORT_PATH="$QML_IMPORT_PATH:$NIXPKGS_QT6_QML_IMPORT_PATH"
            export CMAKE_PREFIX_PATH="${pkgs.lib.makeSearchPath "lib/cmake" qtShellPackages}:$CMAKE_PREFIX_PATH"
            export MD4QT_SOURCE_DIR="${md4qt}"
            export EARCUT_SOURCE_DIR="${earcut}"
            export KSYNTAXHIGHLIGHTING_SOURCE_DIR="${ksyntaxhighlighting}"
            export MICROTEX_SOURCE_DIR="${microtex}"
            export QRCODEGEN_SOURCE_DIR="${qrcodegen}"
            export QWINDOWKIT_SOURCE_DIR="${qwindowkit}"
            export QSIMPLEUPDATER_SOURCE_DIR="${qsimpleupdater}"
            export QAUTOSTART_SOURCE_DIR="${qautostart}"
            export QXTGLOBALSHORTCUT_SOURCE_DIR="${qxtglobalshortcut}"
            export QMLTERMWIDGET_QML_DIR="${qmltermwidgetQmlDir}"
            export IMMER_SOURCE_DIR="${immer}"
            export ZUG_SOURCE_DIR="${zug}"
            export LAGER_SOURCE_DIR="${lager}"
            export BOOST_INCLUDE_DIR="${pkgs.boost.dev}/include"
          '';
        };

        # Wasm cross-development: emscripten + the wasm Qt stack + host tools.
        # Exports DAEMON_APP_QT_WASM for the wasm-release CMake preset, which
        # consumes $env{DAEMON_APP_QT_WASM}/lib/cmake/Qt6/qt.toolchain.cmake.
        #
        # Plus a hermetic headless chromium for the wasm boot smoke and the
        # reload-survival e2e (scripts/wasm-boot-smoke.py honours $CHROMIUM / a
        # `chromium` on PATH). The host chromium is frequently a firejail wrapper
        # that aborts under --headless, so both `just wasm-smoke` / `just e2e-web`
        # take their browser from here. overrideAttrs keeps the shell's
        # emscripten/Qt-wasm shellHook intact (an `inputsFrom` splice would drop it).
        devShells.wasm = qtWasmStack.devShell.overrideAttrs (old: {
          nativeBuildInputs = (old.nativeBuildInputs or [ ]) ++ [ pkgs.chromium ];
          # Surface the header-only mirror-substrate sources in the wasm shell too,
          # so the A1 WASM smoke probe (immer + lager subset compiled under
          # emscripten, spec 09 §11) can resolve them. Append to the existing
          # emscripten/Qt-wasm shellHook rather than replacing it.
          shellHook = (old.shellHook or "") + ''
            export IMMER_SOURCE_DIR="${immer}"
            export ZUG_SOURCE_DIR="${zug}"
            export LAGER_SOURCE_DIR="${lager}"
            export BOOST_INCLUDE_DIR="${pkgs.boost.dev}/include"
          '';
        });

        # Android cross-development: SDK/NDK + the android Qt stack + host
        # tools + the stock androiddeployqt Gradle path (network allowed
        # outside the sandbox); see scripts/build-apk-gradle.sh.
        devShells.android = qtAndroidStack.devShell;

        # iOS cross-development (aarch64-darwin only): nix cmake/ninja + host
        # Qt tools + the vendored-dep wiring, with DAEMON_APP_QT_IOS pointing
        # at the qt-ios-builder output prefix. Elided on Linux (null attr).
        devShells.${if pkgs.stdenv.hostPlatform.isDarwin then "ios" else null} =
          qtIosStack.devShell;
      }
    );
}
