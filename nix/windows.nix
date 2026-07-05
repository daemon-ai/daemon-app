# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# Windows desktop build (x86_64-w64-mingw32 cross): a statically-linked-Qt
# daemon-app.exe that runs on a stock Windows machine (only the Windows system
# DLL floor), plus its packaging (portable layout + NSIS installer via CPack),
# a PE sanity check, and a best-effort wine smoke.
#
# The Qt stack is built from source through the shared qt-from-source.nix
# builder (same recipe as the wasm/linux-static stacks, "mingw-static"
# flavor): static, Release, size-trimmed, windows + offscreen platform
# plugins compiled in. The cross toolchain is nixpkgs' pkgsCross.mingwW64
# from the logos-co mingw-integration fork (see flake.nix).
#
# Static/dynamic decisions (the shipped exe's runtime floor):
#  * INTO the binary (static): all Qt modules + the windows/offscreen QPA
#    plugins, Qt's bundled zlib/libpng/libjpeg/harfbuzz/pcre2/
#    double-conversion/freetype, bundled sqlite (same driver pin as the other
#    flavors), the FULL mingw runtime - libgcc, libstdc++ and the gcc thread
#    shim (nixpkgs mingw gcc uses mcfgthread, LGPL-3.0 WITH
#    GCC-exception-3.1, so the static link conveys cleanly like libgcc
#    itself) via Qt's FEATURE_static_runtime (-static on every executable
#    link).
#  * DYNAMIC against the Windows system floor: kernel32/user32/gdi32/shell32
#    and friends, ws2_32 (sockets), crypt32/bcrypt/secur32 (Schannel TLS),
#    opengl32 (the RHI's GL path; D3D11 loads at runtime via LoadLibrary),
#    dwrite/dwmapi/imm32/winmm/uxtheme... - all inbox DLLs on any supported
#    Windows. checks.windows-sanity pins the full import list.
#  * TLS: Schannel (FEATURE_schannel=ON, INPUT_openssl=no) - the native
#    Windows TLS stack, no OpenSSL to ship or service.
#  * Fonts: Qt's bundled freetype + the DirectWrite/GDI system font database
#    (fontconfig is a non-concept on Windows and stays OFF).
#  * OFF entirely: icu (Qt locale tables suffice, ~30 MB), dbus (no session
#    bus on Windows), glib, zstd (keeps rcc/QRC on zlib like the other
#    static flavors), brotli, vulkan (RHI rides D3D11/GL here).
{
  pkgs,
  appSrc,
  baseVersion,
  versionStr,
  depSources,
}:
let
  inherit (pkgs) lib;

  mingw = pkgs.pkgsCross.mingwW64;

  # ccacheStdenv like the other from-source Qt stacks: the shared
  # /var/cache/ccache softens iterating on the biggest compile in the repo.
  # pkgsCross inherits the flake's ccache overlay, so this wraps the CROSS
  # gcc with the same cache config.
  crossStdenv = mingw.ccacheStdenv;

  qtVersion = pkgs.qt6.qtbase.version;

  # --- Host-tools prefix (QT_HOST_PATH) -----------------------------------
  # Same shape as the wasm stack's host prefix: Qt cross builds want ONE host
  # prefix carrying moc/rcc/syncqt/qmlcachegen/qmltyperegistrar/lrelease/qsb
  # + their Qt6*Tools CMake packages; nixpkgs splits Qt per module, so join
  # the tool-carrying modules into one tree. Host and target track the same
  # nixpkgs qt6 release, satisfying Qt's exact host==target version check.
  qtHost = pkgs.symlinkJoin {
    name = "qt-host-tools-${qtVersion}";
    paths = with pkgs.qt6; [
      qtbase # moc, rcc, syncqt, qmake, Qt6HostInfo
      qtdeclarative # qmlcachegen, qmltyperegistrar, qmlimportscanner
      qttools # lrelease/lupdate (Qt6LinguistTools for qt_add_translations)
      qtshadertools # qsb (Qt6ShaderToolsTools, needed by Quick's shader baking)
    ];
  };

  # --- MinGW cross toolchain file ------------------------------------------
  # The builder configures qtbase with a plain `cmake` call (no nixpkgs cmake
  # hook), so the cross target is selected the same way the wasm flavor does
  # it: an explicit CMAKE_TOOLCHAIN_FILE. qtbase records this file into its
  # generated qt.toolchain.cmake and every consumer (modules, the app)
  # chainloads it again. Tool names stay bare: the cross cc/binutils wrappers
  # are on PATH in every derivation built with the mingw stdenv.
  mingwToolchain = pkgs.writeText "mingw-w64-toolchain.cmake" ''
    set(CMAKE_SYSTEM_NAME Windows)
    set(CMAKE_SYSTEM_PROCESSOR AMD64)
    set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
    set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
    set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
    # Host programs (perl, python, the QT_HOST_PATH tools) resolve normally;
    # libraries/headers/packages only from target prefixes handed in via
    # CMAKE_FIND_ROOT_PATH (Qt's toolchain appends its own prefix there).
    # Host-arch-independent CMake packages (ECM) are passed by explicit
    # <pkg>_DIR, exactly like the wasm build.
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
  '';

  # --- The mingw Qt stack via the shared builder -----------------------------
  # qtbase: the CMake equivalents of
  # `configure -platform win32-g++ -static -static-runtime -release
  #  -qt-host-path ... -schannel -no-openssl -no-dbus -no-icu`.
  qtFromSource = import ./qt-from-source.nix { inherit pkgs; } {
    flavor = "mingw-static";
    stdenv = crossStdenv;
    qtbaseFlags = [
      "-DQT_QMAKE_TARGET_MKSPEC=win32-g++"
      "-DCMAKE_TOOLCHAIN_FILE=${mingwToolchain}"
      "-DQT_HOST_PATH=${qtHost}"
      "-DBUILD_SHARED_LIBS=OFF"
      "-DQT_BUILD_EXAMPLES=OFF"
      "-DQT_BUILD_TESTS=OFF"
      "-DQT_GENERATE_SBOM=OFF"
      # Same supported size-optimized build as the wasm/linux-static flavors
      # (-Os on every module; global, so declarative/svg/websockets inherit).
      "-DFEATURE_optimize_size=ON"
      # -static on every executable link (Qt-built tools AND consuming apps
      # via _qt_internal_set_up_static_runtime_library): libgcc, libstdc++
      # and the gcc thread shim (mcfgthread) all statically linked - the
      # mingw analogue of MSVC's /MT, and the whole point of this flavor.
      "-DFEATURE_static_runtime=ON"
      # windows is the QPA default; offscreen rides along for the wine/CI
      # smoke (a static binary cannot load what is not linked in). Both are
      # marked DEFAULT plugins, which is what a static Qt auto-links into
      # consuming executables.
      "\"-DQT_QPA_PLATFORMS=windows;offscreen\""
      # Qt's bundled third-party copies compile into the static archives.
      # freetype stays bundled too: on Windows Qt pairs it with the
      # DirectWrite/GDI system font database - no fontconfig anywhere.
      "-DFEATURE_system_zlib=OFF"
      "-DFEATURE_system_png=OFF"
      "-DFEATURE_system_jpeg=OFF"
      "-DFEATURE_system_harfbuzz=OFF"
      "-DFEATURE_system_pcre2=OFF"
      "-DFEATURE_system_doubleconversion=OFF"
      "-DFEATURE_freetype=ON"
      "-DFEATURE_system_freetype=OFF"
      "-DFEATURE_fontconfig=OFF"
      # The app uses QSqlDatabase/QSQLITE; pin the bundled-sqlite driver
      # exactly like the other flavors. ODBC would otherwise auto-detect ON
      # for Windows and the static driver plugin auto-links into consumers,
      # dragging an ODBC32.dll import nothing uses.
      "-DFEATURE_sql_sqlite=ON"
      "-DFEATURE_system_sqlite=OFF"
      "-DFEATURE_sql_odbc=OFF"
      # TLS = Schannel (native Windows), no OpenSSL in the artifact.
      "-DINPUT_openssl=no"
      "-DFEATURE_schannel=ON"
      # Trims (rationale in the header).
      "-DFEATURE_dbus=OFF"
      "-DFEATURE_icu=OFF"
      "-DFEATURE_glib=OFF"
      "-DFEATURE_zstd=OFF"
      "-DFEATURE_brotli=OFF"
      "-DFEATURE_vulkan=OFF"
    ];
    # No target libraries: everything Qt needs beyond the bundled sources is
    # the mingw-w64 sysroot (Windows system headers + import libs), which the
    # cross cc wrapper carries.
    qtbaseBuildInputs = [ ];
    moduleBuildInputs = [ ];
  };
  inherit (qtFromSource) mkQtModule;
  qtbaseMingw = qtFromSource.qtbase;

  qtshadertoolsMingw = mkQtModule {
    name = "qtshadertools";
    src = pkgs.qt6.qtshadertools.src;
  };

  qtdeclarativeMingw = mkQtModule {
    name = "qtdeclarative";
    src = pkgs.qt6.qtdeclarative.src;
    qtDeps = [ qtshadertoolsMingw ];
    # Basic only, mirroring the wasm/linux-static trim: the app pins
    # QQuickStyle::setStyle("Basic") and a static Quick Controls build links
    # EVERY built style plugin into the executable. fluentwinui3 and the
    # native windows style both condition on fusion, so the same four flags
    # trim them too.
    extraFlags = [
      "-DFEATURE_quickcontrols2_fusion=OFF"
      "-DFEATURE_quickcontrols2_imagine=OFF"
      "-DFEATURE_quickcontrols2_material=OFF"
      "-DFEATURE_quickcontrols2_universal=OFF"
    ];
  };

  qtsvgMingw = mkQtModule {
    name = "qtsvg";
    src = pkgs.qt6.qtsvg.src;
  };

  qtwebsocketsMingw = mkQtModule {
    name = "qtwebsockets";
    src = pkgs.qt6.qtwebsockets.src;
    # qtwebsockets ships a QML plugin; Quick pulls shadertools transitively.
    qtDeps = [
      qtdeclarativeMingw
      qtshadertoolsMingw
    ];
  };

  # One prefix for the app configure: bin/qt-cmake + the Qt6 config see every
  # module (the toolchain resolves its install prefix from its own symlinked
  # location, and CMake does not resolve symlinks for list files).
  qtMingwJoined = pkgs.symlinkJoin {
    name = "qt-mingw-static-${qtVersion}";
    paths = [
      qtbaseMingw
      qtshadertoolsMingw
      qtdeclarativeMingw
      qtsvgMingw
      qtwebsocketsMingw
    ];
  };

  # MicroTeX links tinyxml2 via pkg-config; cross-compile the same pinned
  # source statically (the wasm and linux-static stacks do the identical
  # thing with their toolchains).
  tinyxml2Mingw = crossStdenv.mkDerivation {
    pname = "tinyxml2-mingw-static";
    version = pkgs.tinyxml-2.version;
    src = pkgs.tinyxml-2.src;

    nativeBuildInputs = with pkgs; [
      cmake
      ninja
    ];

    dontUseCmakeConfigure = true;

    configurePhase = ''
      runHook preConfigure
      cmake -S . -B build -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE=${mingwToolchain} \
        -DCMAKE_INSTALL_PREFIX="$out" \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=OFF \
        -Dtinyxml2_BUILD_TESTING=OFF \
        -DBUILD_TESTING=OFF
      cd build
      runHook postConfigure
    '';
  };

  # Cross-compiling KSyntaxHighlighting needs a NATIVE katehighlightingindexer
  # (it bakes the syntax-definition index into the QRC resources at build
  # time; the freshly cross-compiled one cannot run on the build host). Same
  # host-build + KATEHIGHLIGHTINGINDEXER_EXECUTABLE injection as the wasm
  # stack - see nix/qt-wasm.nix for the full rationale.
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

  # find_package(ECM) cannot see host prefixes under the toolchain's
  # CMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY; an explicit ECM_DIR bypasses the
  # re-rooted search (ECM is host-arch-independent CMake modules).
  ecmDir = "${pkgs.kdePackages.extra-cmake-modules}/share/ECM/cmake";

  # --- daemon-app.exe against the static mingw Qt ---------------------------
  # DAEMON_APP_STATIC=ON is the same app-side gate as the linux portable
  # build: TerminalPanelStub instead of the QMLTermWidget plugin, QSettings
  # token store instead of qtkeychain, static platform-plugin imports. GUI
  # only, tests off (no target test runtime on the build host).
  #
  # The cmake install tree IS the portable Windows layout: bin/daemon-app.exe
  # + share/daemon-app/microtex-res (WIN32 install rule in
  # src/DaemonApp/App/CMakeLists.txt) + share/doc/daemon-app licensing
  # (cmake/Packaging.cmake), which is exactly what CPack stages for NSIS.
  appMingw = crossStdenv.mkDerivation {
    pname = "daemon-app-mingw";
    version = baseVersion;
    src = appSrc;

    nativeBuildInputs = with pkgs; [
      cmake
      ninja
      pkg-config
      # Vendored KSyntaxHighlighting needs ECM + perl on the host (the PHP
      # syntax definition is generated by perl at build time); lrelease
      # comes from QT_HOST_PATH's qttools.
      kdePackages.extra-cmake-modules
      perl
    ];

    dontUseCmakeConfigure = true;

    configurePhase = ''
      runHook preConfigure
      export PKG_CONFIG_PATH="${tinyxml2Mingw}/lib/pkgconfig"
      # MicroTeX's CMake links the raw pkg-config library name instead of the
      # PkgConfig:: imported target, so tinyxml2's include/lib dirs never
      # reach its compile/link lines - ride the global flags, exactly like
      # the wasm build. BUILD_SHARED_LIBS=OFF: KDECMakeSettings (vendored
      # KSyntaxHighlighting) defaults it ON, but everything links into the
      # executable on this static config.
      # -static -static-libgcc -static-libstdc++: the FULL mingw runtime into
      # the exe (winpthreads/mcfgthread included - -static makes ld refuse
      # DLL fallbacks outright). Qt's FEATURE_static_runtime adds -static via
      # target_link_options, but that lands too late on the cc-wrapper's
      # command line to cover every runtime lib; the verified-working spot is
      # CMAKE_EXE_LINKER_FLAGS (checks.windows-sanity pins the result).
      ${qtMingwJoined}/bin/qt-cmake -S . -B build -G Ninja \
        -DCMAKE_INSTALL_PREFIX="$out" \
        -DCMAKE_BUILD_TYPE=Release \
        "-DCMAKE_CXX_FLAGS=-isystem ${tinyxml2Mingw}/include" \
        "-DCMAKE_EXE_LINKER_FLAGS=-L${tinyxml2Mingw}/lib -static -static-libgcc -static-libstdc++" \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_TESTING=OFF \
        -DDAEMON_APP_TUI=OFF \
        -DDAEMON_APP_STATIC=ON \
        -DECM_DIR=${ecmDir} \
        -DKATEHIGHLIGHTINGINDEXER_EXECUTABLE=${ksyntaxIndexerHost}/bin/katehighlightingindexer \
        -DMD4QT_SOURCE_DIR=${depSources.md4qt} \
        -DEARCUT_SOURCE_DIR=${depSources.earcut} \
        -DKSYNTAXHIGHLIGHTING_SOURCE_DIR=${depSources.ksyntaxhighlighting} \
        -DMICROTEX_SOURCE_DIR=${depSources.microtex} \
        -DDAEMON_APP_VERSION_STR=${versionStr} \
        -DDAEMON_APP_UPDATE_CAPABILITY=SelfApply \
        -DDAEMON_APP_UPDATE_FEED_URL=https://github.com/daemon-ai/daemon/releases/latest/download/manifest.json \
        -DDAEMON_APP_UPDATE_PUBKEY=RWRXpowS90Fy+TYhRsrBbQNSDvjbtJpqi9T89OGqSNTLkOa5vn62hK0o \
        -DDAEMON_APP_UPDATE_ARTIFACT_KIND=nsis
      cd build
      runHook postConfigure
    '';
  };

  # --- packages.windows-portable ---------------------------------------------
  # The distributable layout: the stripped exe + MicroTeX runtime resources +
  # third-party notices, mirroring the linux portable tree. Store-path
  # strings QLibraryInfo bakes into libQt6Core.a are scrubbed so this
  # output's closure stays the resources instead of the whole Qt stack
  # (nothing resolves through them at runtime - plugins and QML are compiled
  # in; /nix/store does not exist on Windows anyway).
  windowsPortable =
    pkgs.runCommand "daemon-app-windows-portable-${baseVersion}"
      {
        nativeBuildInputs = [
          mingw.stdenv.cc.bintools.bintools # x86_64-w64-mingw32-strip (PE-aware)
          pkgs.removeReferencesTo
        ];
      }
      ''
        mkdir -p "$out/bin"
        # The exe + the share/ tree only: the mingw stdenv fixup symlinks gcc
        # runtime DLLs next to installed exes as a convenience, which a fully
        # static artifact must not ship (the sanity check pins the import
        # list; stray store symlinks would also drag the toolchain into this
        # output's closure).
        cp ${appMingw}/bin/daemon-app.exe "$out/bin/daemon-app.exe"
        cp -r ${appMingw}/share "$out/share"
        chmod -R u+w "$out"

        x86_64-w64-mingw32-strip --strip-all "$out/bin/daemon-app.exe"

        for ref in ${qtbaseMingw} ${qtshadertoolsMingw} ${qtdeclarativeMingw} \
                   ${qtsvgMingw} ${qtwebsocketsMingw} ${tinyxml2Mingw}; do
          remove-references-to -t "$ref" "$out/bin/daemon-app.exe"
        done
      '';

  # --- packages.nsis -----------------------------------------------------------
  # NSIS installer via CPack, mirroring the mkLinuxArtifacts shape: reuse the
  # cross app build, re-point the install dirs at relative paths (they
  # already are - the manual configure never saw the nix cmake hook), then
  # run `cpack -G NSIS` from the same build tree with makensis (host nsis)
  # on PATH. `bundledFrom` is the same contract as the linux artifacts: the
  # superproject fills in prebuilt SIBLING WINDOWS binaries
  # ({ daemon; daemon-infer; daemon-cli; } exe store paths) once its Rust
  # cross workstream lands; empty means the installer ships the app alone
  # (a complete product: the connection picker speaks to remote daemons).
  mkNsisInstaller =
    { bundledFrom ? { } }:
    appMingw.overrideAttrs (old: {
      pname = "daemon-windows-nsis";

      nativeBuildInputs = old.nativeBuildInputs ++ [ pkgs.nsis ]; # makensis

      # Same configure + the bundling knobs (unset by default, same as the
      # linux artifact builder's bundledFrom).
      configurePhase =
        let
          bundledFlags = lib.concatStringsSep " " (
            lib.optional (bundledFrom ? daemon) "-DDAEMON_APP_BUNDLED_DAEMON=${bundledFrom.daemon}"
            ++ lib.optional (bundledFrom ? daemon-infer)
              "-DDAEMON_APP_BUNDLED_DAEMON_INFER=${bundledFrom.daemon-infer}"
            ++ lib.optional (bundledFrom ? daemon-cli) "-DDAEMON_APP_BUNDLED_DAEMON_CLI=${bundledFrom.daemon-cli}"
          );
        in
        # The base configurePhase ends inside the build dir; a bare re-run
        # with extra -D flags updates the existing cache.
        old.configurePhase
        + lib.optionalString (bundledFlags != "") ''
          cmake ${bundledFlags} .
        '';

      # The output is the installer, not an installed app tree; fixup must
      # not strip the finished artifact (CPACK_STRIP_FILES already strips the
      # staged payload).
      dontFixup = true;
      installPhase = ''
        runHook preInstall

        echo "=== cpack -G NSIS ==="
        cpack -G NSIS || {
          echo "--- cpack NSIS failed; dumping logs ---" >&2
          find _CPack_Packages -name '*.log' | while read -r f; do
            echo "--- $f ---" >&2
            tail -n 60 "$f" >&2
          done
          exit 1
        }

        mkdir -p "$out"
        ls ./daemon-*-win64.exe > /dev/null || {
          echo "missing artifact: daemon-*-win64.exe" >&2
          exit 1
        }
        cp -v ./daemon-*-win64.exe "$out"/
        cp -v ./daemon-*-win64.exe.sha256 "$out"/ 2>/dev/null || true

        runHook postInstall
      '';
    });

  nsisInstaller = mkNsisInstaller { };

  # --- checks.windows-sanity ---------------------------------------------------
  # Pure, sandbox-safe PE contract over the portable exe + the installer:
  # PE32+ magic, the DLL import floor (Windows inbox DLLs only - anything new
  # must be argued into the allowlist rather than silently shipped), the
  # embedded icon resource, and installer existence/size plausibility.
  #
  # The allowlist is lowercase; api-ms-win-* / ext-ms-win-* API set DLLs are
  # accepted by pattern (stable OS contracts, resolved by the loader).
  allowedDlls = [
    # Core Windows
    "kernel32.dll"
    "user32.dll"
    "gdi32.dll"
    "shell32.dll"
    "advapi32.dll"
    "ole32.dll"
    "oleaut32.dll"
    "shlwapi.dll"
    "comdlg32.dll"
    "rpcrt4.dll"
    "version.dll"
    "setupapi.dll"
    "userenv.dll"
    "wtsapi32.dll"
    "shcore.dll"
    "ntdll.dll"
    "msvcrt.dll" # mingw CRT baseline (an inbox DLL on every Windows)
    # Sockets + TLS (Schannel) + crypto
    "ws2_32.dll"
    "iphlpapi.dll"
    "dnsapi.dll"
    "winhttp.dll"
    "crypt32.dll"
    "bcrypt.dll"
    "ncrypt.dll"
    "secur32.dll"
    "authz.dll"
    "netapi32.dll"
    # Graphics / windowing / text
    "opengl32.dll"
    "d3d9.dll"
    "d3d11.dll"
    "d3d12.dll"
    "dxgi.dll"
    "dwrite.dll"
    "d2d1.dll"
    "dwmapi.dll"
    "uxtheme.dll"
    "imm32.dll"
    "winmm.dll"
    "comctl32.dll"
    "dcomp.dll"
  ];

  windowsSanity =
    pkgs.runCommand "windows-sanity"
      {
        nativeBuildInputs = [
          pkgs.file
          mingw.stdenv.cc.bintools.bintools # x86_64-w64-mingw32-objdump (PE-aware)
        ];
      }
      ''
        set -euo pipefail
        exe=${windowsPortable}/bin/daemon-app.exe
        installer=$(ls ${nsisInstaller}/daemon-*-win64.exe)
        mkdir -p "$out"
        report="$out/report.txt"

        echo "== windows-sanity: $exe" | tee "$report"

        echo "-- PE32+ magic --" | tee -a "$report"
        file "$exe" | tee -a "$report"
        file "$exe" | grep -q "PE32+ executable" || { echo "FAIL: not a PE32+ executable" >&2; exit 1; }
        file "$exe" | grep -q "x86-64" || { echo "FAIL: not x86-64" >&2; exit 1; }
        x86_64-w64-mingw32-objdump -f "$exe" | grep -q "pei-x86-64" || {
          echo "FAIL: objdump does not see pei-x86-64" >&2; exit 1;
        }
        # GUI subsystem (no console window): PE optional header Subsystem 2.
        x86_64-w64-mingw32-objdump -p "$exe" | grep -Eq "^Subsystem.*\(Windows GUI\)" || {
          echo "FAIL: subsystem is not Windows GUI" >&2
          x86_64-w64-mingw32-objdump -p "$exe" | grep -i subsystem >&2
          exit 1
        }

        echo "-- DLL imports --" | tee -a "$report"
        x86_64-w64-mingw32-objdump -p "$exe" \
          | awk '/DLL Name:/ { print tolower($3) }' | sort -u | tee dlls.txt >> "$report"
        fail=0
        while read -r dll; do
          case "$dll" in
            api-ms-win-*|ext-ms-win-*) continue ;;
          esac
          case " ${lib.concatStringsSep " " allowedDlls} " in
            *" $dll "*) ;;
            *) echo "FAIL: import '$dll' is outside the Windows system floor" >&2; fail=1 ;;
          esac
        done < dlls.txt
        [ "$fail" = 0 ] || exit 1
        # A static Qt exe must not import Qt/gcc runtime DLLs.
        if grep -Eq "qt6|libstdc|libgcc|libwinpthread|mcfgthread" dlls.txt; then
          echo "FAIL: runtime DLL leaked into the imports (static link regressed)" >&2
          exit 1
        fi

        echo "-- icon resource (.rsrc) --" | tee -a "$report"
        rsrc_size=$(x86_64-w64-mingw32-objdump -h "$exe" | awk '$2 == ".rsrc" { print strtonum("0x" $3) }')
        echo "rsrc section size: ''${rsrc_size:-0}" | tee -a "$report"
        [ -n "$rsrc_size" ] && [ "$rsrc_size" -gt 10000 ] || {
          echo "FAIL: .rsrc section missing or too small for the embedded icon" >&2
          exit 1
        }

        echo "-- installer --" | tee -a "$report"
        ls -l "$installer" | tee -a "$report"
        file "$installer" | tee -a "$report"
        file "$installer" | grep -q "PE32 executable" || {
          echo "FAIL: installer is not a PE executable (NSIS stub)" >&2; exit 1;
        }
        size=$(stat -c%s "$installer")
        # Plausibility floor: the static exe alone is tens of MB; a tiny
        # installer means the payload went missing.
        [ "$size" -gt 15000000 ] || { echo "FAIL: installer suspiciously small ($size bytes)" >&2; exit 1; }
        echo "installer size: $size bytes" | tee -a "$report"

        echo "windows-sanity: OK" | tee -a "$report"
      '';

  # --- wine smoke harness --------------------------------------------------------
  # Best-effort wine boot (NOT a flake check, wine is not part of any gate):
  # runs the exe offscreen with the DAEMON_APP_WAIT_READY sentinel contract
  # (main.cpp prints "DAEMON_APP_READY ok" and exits 0), then a silent NSIS
  # install into a scratch prefix. Wine failures are reported honestly and do
  # not fail the workstream - a GUI-subsystem exe still writes the sentinel
  # to a redirected stdout, which is exactly how this harness consumes it.
  #
  # PER-USER install (todo u3-windows): the installer is per-user
  # (RequestExecutionLevel user, $LOCALAPPDATA\Programs\Daemon), so this asserts
  # the DEFAULT install lands under the user's Local AppData (no /D override) -
  # exactly the promptless location the auto-updater's SelfApply tier relies on.
  # It also asserts the daemon-updater.exe helper ships in bin\ (SelfApply spawns
  # it next to the app). A full nsis-silent helper exercise (wait-pid -> /S ->
  # relaunch) under wine is intentionally NOT attempted here: wine's process and
  # named-object semantics make the detached wait-pid/relaunch dance flaky, and
  # real-Windows validation is the runbook step (packaging/windows/UPDATE-VALIDATION.md).
  #
  # WoW64 wine is required: the NSIS stub is a PE32 i386 executable.
  #
  # `minimal` (not `stable`): the full `stable` closure pulls a large multimedia stack
  # (libcamera -> pipewire -> python3Packages.pybind11) whose pybind11 build-time TEST SUITE fails
  # on the pinned fork nixpkgs, so `stable` cannot even build here. `minimal` drops that entire
  # optional stack (no gstreamer/pipewire/sane/camera), which is irrelevant to this smoke: a
  # headless offscreen Qt app + an NSIS silent install + local named pipes only need the core win32
  # DLLs (kernel32/ntdll/user32/gdi32/advapi32/ws2_32), all present in minimal. Still WoW64, so the
  # i386 NSIS stub runs.
  wine = pkgs.wineWowPackages.minimal;

  # Parameterized hermetic wine harness. Both the child's app-only smoke and the
  # superproject's composed E2E smoke reuse the same prelude (fresh throwaway
  # WINEPREFIX + HOME, gecko/mono + their network fetches disabled via
  # WINEDLLOVERRIDES, offscreen QPA, and a `wineserver -k; wineserver -w`
  # teardown so no lingering wine process holds the prefix open before it is
  # removed).
  #
  #   installer : the NSIS installer derivation dir (holds daemon-*-win64.exe);
  #               bound to the shell var `$installerExe` (the resolved .exe).
  #   body      : the test steps; runs after the prefix is inited, with `wine`
  #               on PATH, `$tmp` (scratch), `$WINEPREFIX`, and `$installerExe`
  #               in scope, and `status` (0) it can set to fail the run.
  mkWindowsSmoke =
    {
      name,
      installer,
      body,
    }:
    pkgs.writeShellApplication {
      inherit name;
      runtimeInputs = [ wine ];
      text = ''
        tmp=$(mktemp -d)
        export HOME="$tmp/home" && mkdir -p "$HOME"
        export WINEPREFIX="$tmp/prefix"
        export WINEDEBUG=-all
        # Kill the gecko/mono install prompts AND their network fetches (no .NET /
        # embedded-HTML in this app), keeping the harness hermetic + offline.
        export WINEDLLOVERRIDES="mscoree,mshtml="
        # Headless boot: the offscreen QPA plugin is compiled into the exe.
        export QT_QPA_PLATFORM=offscreen
        export QT_QUICK_BACKEND=software
        # Teardown: the body may leave wine processes alive on purpose (the
        # composed E2E smoke leaves the app-spawned daemon.exe serving its named
        # pipe), so a bare `wineserver -w` would wait forever. Kill everything in
        # the prefix first (-k), then wait for the server to exit (-w) so nothing
        # races the rm and leaks the dir.
        trap 'wineserver -k 2>/dev/null || true; wineserver -w 2>/dev/null || true; rm -rf "$tmp"' EXIT

        installerExe=$(find ${installer} -maxdepth 1 -name 'daemon-*-win64.exe' -print -quit)
        status=0

        echo "== ${name}: wineboot (fresh prefix) =="
        wineboot --init >/dev/null 2>&1 || true

        ${body}

        exit "$status"
      '';
    };

  windowsSmoke = mkWindowsSmoke {
    name = "windows-smoke";
    installer = nsisInstaller;
    body = ''
      export DAEMON_APP_SERVICE_MODE=mock
      export DAEMON_APP_WAIT_READY=30000
      touch "$tmp/daemon-sock-stub"
      # wine maps / as Z:, so hand the mock's existence probe a Z: path.
      export DAEMON_APP_SOCKET="Z:$tmp/daemon-sock-stub"

      echo "== windows-smoke: daemon-app.exe offscreen boot (WAIT_READY) =="
      if wine ${windowsPortable}/bin/daemon-app.exe > "$tmp/boot.log" 2>&1; then
        boot_rc=0
      else
        boot_rc=$?
      fi
      tail -n 20 "$tmp/boot.log" || true
      if grep -q "DAEMON_APP_READY ok" "$tmp/boot.log" && [ "$boot_rc" = 0 ]; then
        echo "windows-smoke: boot OK (DAEMON_APP_READY ok, exit 0)"
      else
        echo "windows-smoke: boot FAILED under wine (exit $boot_rc) - best-effort, see log above"
        status=1
      fi

      echo "== windows-smoke: NSIS silent PER-USER install (/S, no /D) =="
      # No /D override: exercise the compiled-in per-user default install root
      # ($LOCALAPPDATA\Programs\Daemon). Under wine $LOCALAPPDATA maps to
      # drive_c/users/<user>/AppData/Local, so the tree lands under Programs/Daemon.
      if wine "$installerExe" /S > "$tmp/install.log" 2>&1; then
        inst_rc=0
      else
        inst_rc=$?
      fi
      # Resolve the per-user install dir by glob (the wine username varies).
      installDir=""
      for f in "$WINEPREFIX"/drive_c/users/*/AppData/Local/Programs/Daemon/bin/daemon-app.exe; do
        if [ -f "$f" ]; then installDir=$(dirname "$f"); break; fi
      done
      installed="$installDir/daemon-app.exe"
      if [ "$inst_rc" = 0 ] && [ -n "$installDir" ] && [ -f "$installed" ]; then
        echo "windows-smoke: per-user silent install OK ($installed)"
        # PROGRAMFILES regression guard: a per-machine install (the old default)
        # would have landed here and would mean the per-user switch regressed.
        if [ -f "$WINEPREFIX/drive_c/Program Files/Daemon/bin/daemon-app.exe" ]; then
          echo "windows-smoke: FAIL install landed in Program Files (per-user switch regressed)"
          status=1
        fi
        # The SelfApply helper must ship next to the app (bin\daemon-updater.exe).
        if [ -f "$installDir/daemon-updater.exe" ]; then
          echo "windows-smoke: daemon-updater helper present (SelfApply payload)"
        else
          echo "windows-smoke: daemon-updater.exe MISSING from bin\\ (SelfApply cannot spawn the helper)"
          status=1
        fi
        if wine "$installed" > "$tmp/installed-boot.log" 2>&1 \
           && grep -q "DAEMON_APP_READY ok" "$tmp/installed-boot.log"; then
          echo "windows-smoke: installed exe boot OK"
        else
          echo "windows-smoke: installed exe boot FAILED - best-effort"
          tail -n 10 "$tmp/installed-boot.log" || true
          status=1
        fi
        instRoot=$(dirname "$installDir")
        uninst=$(find "$instRoot" -maxdepth 1 -name 'Uninstall*.exe' -print -quit 2>/dev/null || true)
        if [ -n "$uninst" ]; then
          echo "windows-smoke: uninstaller present ($(basename "$uninst"))"
        else
          echo "windows-smoke: uninstaller MISSING"
          status=1
        fi
      else
        echo "windows-smoke: per-user silent install FAILED under wine (exit $inst_rc) - best-effort"
        echo "  (expected tree under drive_c/users/*/AppData/Local/Programs/Daemon/bin)"
        tail -n 20 "$tmp/install.log" || true
        status=1
      fi
    '';
  };
in
{
  qtbase = qtbaseMingw;
  qtshadertools = qtshadertoolsMingw;
  qtdeclarative = qtdeclarativeMingw;
  qtsvg = qtsvgMingw;
  qtwebsockets = qtwebsocketsMingw;
  qtMingw = qtMingwJoined;
  tinyxml2 = tinyxml2Mingw;
  app = appMingw;
  portable = windowsPortable;
  nsis = nsisInstaller;
  sanity = windowsSanity;
  smoke = windowsSmoke;
  # The parameterized wine harness + the (WoW64) wine derivation, reused by the
  # superproject's composed E2E smoke so it runs the same hardened prelude over
  # the bundled installer with the exact same nix-provided wine.
  mkSmoke = mkWindowsSmoke;
  inherit wine;
  inherit qtHost mingwToolchain;
}
