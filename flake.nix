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

        # --- Linux packaging artifacts (CPack: DEB / RPM / AppImage) --------
        # CAVEAT (v1 scaffolding): these package the DYNAMIC-Qt app as linked
        # by the Nix toolchain - interpreter + RUNPATH point into /nix/store,
        # so the artifacts only run where those store paths exist (the build
        # sandbox / NixOS). The sibling static-qt workstream swaps in the
        # portable payload later; what is load-bearing now is the CPack
        # skeleton, assets, install rules, per-generator config, and the
        # artifact checks below. See cmake/Packaging.cmake.
        appimageTooling = import ./nix/appimagetool.nix { inherit pkgs; };
        # CPack's AppImage generator needs CMake >= 4.2 (pinned nixpkgs ships
        # 4.1.2); nix/cmake-appimage.nix overrides the nixpkgs cmake with the
        # 4.2.7 source.
        cmake42 = import ./nix/cmake-appimage.nix { inherit pkgs; };

        # Build the app once with the packaging knobs, then run cpack per
        # generator from the same build tree. `bundledFrom` is the shape the
        # superproject fills in a later workstream: prebuilt sibling binaries
        # ({ daemon; daemon-infer; daemon-cli; } store paths) installed next
        # to bin/daemon-app (LocalDaemonLauncher discovers them there). Left
        # empty here, so these artifacts package the app alone.
        mkLinuxArtifacts =
          { bundledFrom ? { } }:
          daemon-app.overrideAttrs (old: {
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
                pkgs.patchelf # AppImage generator rewrites non-$ORIGIN RPATHs
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
            ]
            ++ pkgs.lib.optional (bundledFrom ? daemon) "-DDAEMON_APP_BUNDLED_DAEMON=${bundledFrom.daemon}"
            ++ pkgs.lib.optional (bundledFrom ? daemon-infer)
              "-DDAEMON_APP_BUNDLED_DAEMON_INFER=${bundledFrom.daemon-infer}"
            ++ pkgs.lib.optional (bundledFrom ? daemon-cli)
              "-DDAEMON_APP_BUNDLED_DAEMON_CLI=${bundledFrom.daemon-cli}";

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

        # Single-format views of the artifact set (`nix build .#deb` etc.).
        selectArtifact =
          name: glob:
          pkgs.runCommand "daemon-${name}" { } ''
            mkdir -p "$out"
            cp -v ${linuxArtifacts}/${glob} "$out"/
            cp -v ${linuxArtifacts}/${glob}.sha256 "$out"/ 2>/dev/null || true
          '';

        # Sanity gate over all three artifacts: metadata + payload layout
        # assertions, desktop/metainfo validation, an AppImage payload
        # extract + offscreen boot probe (DAEMON_APP_WAIT_READY contract from
        # src/DaemonApp/App/main.cpp), and the glibc symbol-version floor.
        artifactSanity =
          pkgs.runCommand "daemon-artifact-sanity"
            {
              nativeBuildInputs = with pkgs; [
                dpkg # dpkg-deb --info/--contents/-x
                rpm # rpm -qpl, rpm2cpio
                cpio
                binutils # readelf (glibc floor)
                desktop-file-utils
                appstream # appstreamcli validate
                squashfsTools # unsquashfs (AppImage payload extraction)
              ];
              # The Nix-linked payload resolves its libraries through
              # /nix/store RUNPATHs; keeping the app package as an input
              # pulls that closure into this check's sandbox (the store
              # references inside the compressed artifacts are invisible to
              # the nix scanner).
              appClosure = daemon-app;
              qtPluginPath = pkgs.lib.makeSearchPath pkgs.qt6.qtbase.qtPluginPrefix qtPackages;
              qtQmlPath = "${pkgs.lib.makeSearchPath pkgs.qt6.qtbase.qtQmlPrefix qtPackages}:${qmltermwidgetQmlDir}";
            }
            ''
              set -euo pipefail
              mkdir work && cd work

              echo "=== DEB: metadata + contents ==="
              deb=$(ls ${linuxArtifacts}/*.deb)
              dpkg-deb --info "$deb" | tee deb-info.txt
              grep -q "Package: daemon" deb-info.txt
              grep -q "Version: ${baseVersion}" deb-info.txt
              dpkg-deb --contents "$deb" > deb-contents.txt
              for path in \
                ./opt/daemon/bin/daemon-app \
                ./opt/daemon/lib/libKF6SyntaxHighlighting.so.6 \
                ./opt/daemon/share/applications/daemon-app.desktop \
                ./opt/daemon/share/metainfo/io.daemon.app.metainfo.xml \
                ./opt/daemon/share/icons/hicolor/256x256/apps/daemon-app.png \
                ./opt/daemon/share/icons/hicolor/scalable/apps/daemon-app.svg \
                ./opt/daemon/share/doc/daemon-app/LICENSE \
                ./opt/daemon/share/doc/daemon-app/THIRD-PARTY-NOTICES.md; do
                grep -qF " $path" deb-contents.txt || { echo "missing in deb: $path" >&2; exit 1; }
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

              echo "=== RPM: metadata + contents ==="
              rpmfile=$(ls ${linuxArtifacts}/*.rpm)
              # --dbpath: no /var/lib/rpm in the sandbox
              rpmq() { rpm --dbpath "$PWD/rpmdb" --nosignature "$@"; }
              rpmq -qpi "$rpmfile" | tee rpm-info.txt
              grep -q "^Name *: daemon" rpm-info.txt
              rpmq -qpl "$rpmfile" > rpm-list.txt
              for path in \
                /opt/daemon/bin/daemon-app \
                /opt/daemon/lib/libKF6SyntaxHighlighting.so.6 \
                /opt/daemon/share/applications/daemon-app.desktop \
                /opt/daemon/share/metainfo/io.daemon.app.metainfo.xml \
                /opt/daemon/share/icons/hicolor/256x256/apps/daemon-app.png; do
                grep -qxF "$path" rpm-list.txt || { echo "missing in rpm: $path" >&2; exit 1; }
              done
              if grep -q "nix/store" rpm-list.txt; then
                echo "rpm payload contains /nix/store paths" >&2
                exit 1
              fi
              rpmq -qp --scripts "$rpmfile" | grep -q "/usr/local/bin"
              mkdir rpm-root && (cd rpm-root && rpm2cpio "$rpmfile" | cpio -idm --quiet)

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

              echo "=== AppImage: offscreen boot probe ==="
              export HOME="$PWD/home" && mkdir -p "$HOME"
              export XDG_RUNTIME_DIR="$PWD/xdg" && mkdir -m 0700 -p "$XDG_RUNTIME_DIR"
              export QT_QPA_PLATFORM=offscreen
              # v1 payload runs against the sandbox's Qt store paths: plugins
              # and QML come from the build inputs, first-party QML modules
              # are linked into the binary, and the KSyntaxHighlighting QML
              # module ships inside the artifact (usr/lib/qt-6/qml).
              export QT_PLUGIN_PATH="$qtPluginPath"
              # Both KDEInstallDirs QML layouts (lib/qml, lib/qt-6/qml);
              # nonexistent entries are ignored.
              export QML2_IMPORT_PATH="$qtQmlPath:$PWD/squashfs-root/usr/lib/qml:$PWD/squashfs-root/usr/lib/qt-6/qml"
              # Mock service mode: the WAIT_READY probe drives a first-run
              # "local" connect, which the mock accepts iff the target file
              # exists - no daemon binary needed (none is bundled here).
              export DAEMON_APP_SERVICE_MODE=mock
              export DAEMON_APP_SOCKET="$PWD/mock-daemon.sock" && touch "$DAEMON_APP_SOCKET"
              export DAEMON_APP_WAIT_READY=8000
              # AppRun's `#!/usr/bin/env bash` shebang cannot resolve in the
              # sandbox; invoke through bash.
              bash squashfs-root/AppRun | tee boot.txt
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

        # Linux packaging artifacts (v1 scaffolding; see mkLinuxArtifacts).
        # `artifacts` carries all three; the per-format outputs are views of
        # the same build.
        packages.artifacts = linuxArtifacts;
        packages.appimage = selectArtifact "appimage" "*.AppImage";
        packages.deb = selectArtifact "deb" "*.deb";
        packages.rpm = selectArtifact "rpm" "*.rpm";
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

        # Proves the wasm stack end-to-end without the app: a static Qt Quick
        # hello-world through the joined prefix's qt-cmake, asserting the
        # .wasm/.js/.html artifact set.
        checks.qt-wasm-smoke = qtWasmStack.smoke;

        # Packaging artifact gate: DEB/RPM metadata + payload assertions,
        # desktop/metainfo validation, AppImage payload extract + offscreen
        # boot, glibc symbol-version floor (scripts/glibc-floor.sh).
        checks.artifact-sanity = artifactSanity;

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
