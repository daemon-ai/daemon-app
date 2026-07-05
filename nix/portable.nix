# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# Portable Linux desktop build: a statically-linked-Qt daemon-app GUI that
# boots on a generic distro root (only the system loader + the X/GL floor),
# plus its packaging (patchelf'd layout + tarball) and boot smoke.
#
# The Qt stack is built from source through the shared qt-from-source.nix
# builder (same recipe as the wasm stack, "linux-static" flavor): static,
# Release, size-trimmed like wasm, but multi-threaded and with the desktop
# platform plugins (xcb AND wayland) compiled in statically.
#
# Static/dynamic decisions (the portable binary's runtime floor):
#  * INTO the binary (static): all Qt modules + platform plugins, Qt's
#    bundled zlib/libpng/libjpeg/harfbuzz/pcre2/double-conversion, bundled
#    sqlite (FEATURE_system_sqlite=OFF, same driver pin as wasm), libstdc++/
#    libgcc (-static-libstdc++ -static-libgcc: the classic portability
#    failure is a GLIBCXX floor, so don't have one), and the platform
#    integration LEAVES no distro guarantees on a base install: the
#    xcb-util family (cursor/icccm/ewmh/image/keysyms/render-util/util -
#    Qt 6.5+'s xcb plugin hard-requires libxcb-cursor, a real user-hit
#    AppImage loader failure), libxkbcommon(+x11), and the wayland client
#    libs (client/cursor/egl, with libffi following wayland-client). See
#    staticLeaves below; the floor mirrors the AppImage community
#    excludelist's system-guaranteed set.
#  * DYNAMIC against the system floor (the distributor treats these as
#    system-provided): glibc, libGL/libEGL (linking GL dynamically is the
#    norm even for static Qt - drivers are inherently host-side), libX11/
#    libX11-xcb and libxcb CORE incl. its own sub-libs (GL drivers must
#    share the process's libxcb copy), and fontconfig+freetype. fontconfig
#    CANNOT go static cleanly: Qt's feature graph hard-requires
#    system_freetype with fontconfig (two freetype copies would fight over
#    the same FT_Face handles), and system font discovery is the whole
#    point of fontconfig - so both stay dynamic, documented as floor.
#  * RUNTIME dlopen (no DT_NEEDED entry, degrade gracefully when absent):
#    dbus (FEATURE_dbus=ON + dbus_linked=OFF - the tray's StatusNotifier
#    path on modern desktops speaks DBus; QSystemTrayIcon/notify degrade to
#    "no tray" without it, which the app already handles) and openssl
#    (FEATURE_openssl=ON + openssl_linked=OFF - TLS resolves the host's
#    libssl.so.3 on demand).
#  * OFF entirely: icu (Qt's own locale tables suffice, saves ~30 MB),
#    glib event-loop glue, zstd (keeps rcc/QRC on zlib, matching the
#    feature-less wasm precedent), brotli, sessionmanager (libSM/ICE),
#    vulkan (the scenegraph rides GL here), gtk3 theme, cups, dbus-linked.
{
  pkgs,
  appSrc,
  baseVersion,
  versionStr,
  depSources,
}:
let
  inherit (pkgs) lib;

  # ccacheStdenv like the rest of the repo: Qt-from-source is the single
  # biggest compile in the project, and the shared cache makes iterating on
  # feature flags survivable.
  staticStdenv = pkgs.ccacheStdenv;

  # --- Static platform-integration leaves --------------------------------------
  # The xcb-util family, xkbcommon, and the wayland client libs are LINKED INTO
  # the binary: they are the DT_NEEDED entries the AppImage community
  # excludelist does NOT guarantee on a base system (Qt 6.5+'s xcb platform
  # hard-requires libxcb-cursor.so.0, which many distros do not install by
  # default - the exact loader failure a user hit running the AppImage).
  # Static leaves shrink the binary's dynamic closure to ~the excludelist, so
  # the packages bundle nothing.
  #
  # NOT pkgsStatic (musl): these override the pinned glibc packages to emit
  # only PIC archives (--with-pic / meson b_staticpic default: they must link
  # into a PIE executable). Only the .a form changes - same sources, same
  # versions.
  mkAutotoolsStatic =
    p:
    p.overrideAttrs (old: {
      pname = old.pname + "-static";
      # Pin the already-computed src: several upstream expressions derive the
      # tarball URL from finalAttrs.pname, which the -static rename would
      # corrupt (wrong URL on any machine that has to actually fetch).
      src = old.src;
      configureFlags = (old.configureFlags or [ ]) ++ [
        "--enable-static"
        "--disable-shared"
        "--with-pic"
      ];
      dontDisableStatic = true;
      doCheck = false;
    });
  xcbUtilStatic = mkAutotoolsStatic pkgs.libxcb-util;
  xcbImageStatic = mkAutotoolsStatic pkgs.libxcb-image;
  xcbKeysymsStatic = mkAutotoolsStatic pkgs.libxcb-keysyms;
  xcbRenderUtilStatic = mkAutotoolsStatic pkgs.libxcb-render-util;
  xcbWmStatic = mkAutotoolsStatic pkgs.libxcb-wm;
  xcbCursorStatic = mkAutotoolsStatic pkgs.libxcb-cursor;
  # wayland-client marshals requests through libffi; a static wayland-client
  # would otherwise surface libffi.so as a NEW dynamic dep (not on the
  # excludelist), so libffi goes static with it. Resolved onto the app link
  # line via CMAKE_CXX_STANDARD_LIBRARIES (appStatic below): CMake's Wayland
  # find module records only the wayland archives, not their Requires.private.
  #
  # src re-pin: libffi/wayland derive the tarball URL from finalAttrs.pname,
  # which the -static rename would corrupt through the overrideAttrs
  # fixpoint; take the original package's already-resolved src instead.
  libffiStatic = (mkAutotoolsStatic pkgs.libffi).overrideAttrs { src = pkgs.libffi.src; };
  # STATIC LIBXKBCOMMON TRAP: the library bakes its default XKB data root at
  # build time. The nixpkgs default is a /nix/store path - useless on target
  # systems and a store-path leak into the shipped binary - so pin the FHS
  # locations every target distro provides (XKB_CONFIG_ROOT / XLOCALEDIR env
  # still override at runtime).
  xkbcommonStatic = (pkgs.libxkbcommon.override { withWaylandTools = false; }).overrideAttrs (old: {
    pname = old.pname + "-static";
    src = old.src;
    outputs = [
      "out"
      "dev"
    ];
    # No xkeyboard_config in buildInputs: its optional xkeyboard-config-2
    # detection would bake that package's /nix/store path into the archives
    # as an extra include root (dead on target systems and a store-path leak
    # into the shipped binary). With it absent, only the FHS paths below
    # remain baked.
    buildInputs = with pkgs; [
      libxcb
      libxml2
    ];
    mesonFlags = [
      "-Ddefault_library=static"
      "-Dxkb-config-root=/usr/share/X11/xkb"
      "-Dxkb-config-extra-path=/etc/xkb"
      "-Dx-locale-root=/usr/share/X11/locale"
      "-Denable-docs=false"
      "-Denable-wayland=false"
    ];
    doCheck = false;
  });
  waylandStatic =
    (pkgs.wayland.override {
      withDocumentation = false;
      withTests = false;
    }).overrideAttrs
      (old: {
        pname = old.pname + "-static";
        src = pkgs.wayland.src; # see libffiStatic: pname-derived URL
        outputs = [
          "out"
          "dev"
        ];
        separateDebugInfo = false;
        mesonFlags = (old.mesonFlags or [ ]) ++ [ "-Ddefault_library=static" ];
        buildInputs = [ libffiStatic ];
        doCheck = false;
        # Fold libffi's objects into libwayland-client.a: wayland-client.pc
        # declares libffi only in Requires.private, which CMake find modules
        # (Qt's FindWayland included) never read - every static consumer
        # would otherwise die with undefined ffi_* symbols (Qt module tools
        # link Qt6Gui -> wayland-client without pkg-config's --static
        # closure). Folding makes the archive self-contained for any
        # consumer.
        postFixup = (old.postFixup or "") + ''
          ar -M <<MRI
          OPEN $out/lib/libwayland-client.a
          ADDLIB ${libffiStatic}/lib/libffi.a
          SAVE
          END
          MRI
          ranlib "$out/lib/libwayland-client.a"
        '';
      });
  staticLeaves = [
    xcbUtilStatic
    xcbImageStatic
    xcbKeysymsStatic
    xcbRenderUtilStatic
    xcbWmStatic
    xcbCursorStatic
    xkbcommonStatic
    waylandStatic
    libffiStatic
  ];

  # The platform-integration floor the static Qt configure detects at build
  # time. The static leaves above resolve to archives absorbed at app link
  # time; the rest stays dynamic and the portable binary resolves it from the
  # host at runtime. Shared by the Qt build and the app link (the app's
  # configure re-runs Qt's find_dependency calls for the static plugins'
  # interface libs).
  platformLibs =
    staticLeaves
    ++ (with pkgs; [
      # X11/xcb CORE stays dynamic on purpose: it is on the AppImage
      # excludelist, ships with every X-capable distro, and GL drivers must
      # share the process's libxcb copy. (libxau/libxdmcp are
      # Requires.private of xcb.pc, so pkg-config insists on seeing them
      # even for a dynamic link.)
      libxcb
      libx11
      libxau
      libxdmcp
      # GL/EGL stubs; the real driver always comes from the host
      libglvnd
      # Font discovery + rasterization (see the header: forced-dynamic pair)
      fontconfig
      freetype
      # Headers-only at build for the runtime-dlopen features (no DT_NEEDED)
      dbus
      openssl
    ]);

  qtFromSource = import ./qt-from-source.nix { inherit pkgs; } {
    flavor = "linux-static";
    stdenv = staticStdenv;
    qtbaseFlags = [
      "-DBUILD_SHARED_LIBS=OFF"
      "-DQT_BUILD_EXAMPLES=OFF"
      "-DQT_BUILD_TESTS=OFF"
      "-DQT_GENERATE_SBOM=OFF"
      # Same supported size-optimized build as wasm (-Os on every module;
      # global, so the declarative/svg/websockets modules inherit it).
      "-DFEATURE_optimize_size=ON"
      # Desktop platform integration, all compiled in statically. Since 6.10
      # the wayland CLIENT (QtWaylandClient + the qwayland platform plugin +
      # xdg-shell, with vendored protocol XMLs) lives in qtbase itself, so no
      # qtwayland module is needed. QT_QPA_PLATFORMS marks every listed
      # platform plugin as a DEFAULT plugin - which is what a static Qt
      # auto-links into consuming executables - and its first entry becomes
      # QT_QPA_DEFAULT_PLATFORM (xcb: the conservative launch default;
      # wayland-session users get wayland via Qt's normal runtime selection
      # or QT_QPA_PLATFORM=wayland). offscreen rides along for headless
      # smoke tests and CI (it is NOT a default plugin otherwise, and a
      # static binary cannot load what is not linked in).
      "\"-DQT_QPA_PLATFORMS=xcb;wayland;offscreen;minimal\""
      "-DFEATURE_xcb=ON"
      "-DFEATURE_xcb_xlib=ON"
      "-DFEATURE_xkbcommon=ON"
      "-DFEATURE_wayland_client=ON"
      "-DFEATURE_fontconfig=ON"
      "-DFEATURE_system_freetype=ON" # required by fontconfig (see header)
      "-DFEATURE_opengl=ON"
      "-DFEATURE_egl=ON"
      # Qt's bundled third-party copies compile into the static archives.
      "-DFEATURE_system_zlib=OFF"
      "-DFEATURE_system_png=OFF"
      "-DFEATURE_system_jpeg=OFF"
      "-DFEATURE_system_harfbuzz=OFF"
      "-DFEATURE_system_pcre2=OFF"
      "-DFEATURE_system_doubleconversion=OFF"
      # The app uses QSqlDatabase/QSQLITE; pin the bundled-sqlite driver
      # exactly like the wasm stack.
      "-DFEATURE_sql_sqlite=ON"
      "-DFEATURE_system_sqlite=OFF"
      # Runtime-dlopen features (rationale in the header).
      "-DFEATURE_dbus=ON"
      "-DFEATURE_dbus_linked=OFF"
      "-DFEATURE_openssl=ON"
      "-DFEATURE_openssl_linked=OFF"
      # Trims (rationale in the header).
      "-DFEATURE_icu=OFF"
      "-DFEATURE_glib=OFF"
      "-DFEATURE_zstd=OFF"
      "-DFEATURE_brotli=OFF"
      "-DFEATURE_sessionmanager=OFF"
      "-DFEATURE_vulkan=OFF"
      "-DFEATURE_gtk3=OFF"
      "-DFEATURE_cups=OFF"
    ];
    qtbaseNativeBuildInputs = with pkgs; [
      cmake
      ninja
      pkg-config # Qt's configure locates the xcb/fontconfig floor via .pc files
      wayland-scanner # generates the client protocol wrappers (XMLs are vendored)
    ];
    qtbaseBuildInputs = platformLibs;
    moduleNativeBuildInputs = with pkgs; [
      cmake
      ninja
      python3 # qtdeclarative's qml configure requires a host Python
      pkg-config
      # Every consumer of the static Qt6Gui must resolve Qt6WaylandClient,
      # whose Qt6WaylandScannerTools dependency hard-requires the
      # wayland-scanner executable at configure time.
      wayland-scanner
    ];
    # The static Qt6Gui re-resolves its private platform deps (OpenGL, xcb,
    # fontconfig, ...) in every consumer configure; without them the module
    # silently configures to "nothing to build" and the install fails.
    moduleBuildInputs = platformLibs;
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
    # Basic only, mirroring the wasm trim: the app pins
    # QQuickStyle::setStyle("Basic") in main() and imports no other style,
    # and a static Quick Controls build links EVERY built style plugin into
    # the executable because style selection is a runtime decision.
    extraFlags = [
      "-DFEATURE_quickcontrols2_fusion=OFF"
      "-DFEATURE_quickcontrols2_imagine=OFF"
      "-DFEATURE_quickcontrols2_material=OFF"
      "-DFEATURE_quickcontrols2_universal=OFF"
    ];
  };

  qtsvgStatic = mkQtModule {
    name = "qtsvg";
    src = pkgs.qt6.qtsvg.src;
  };

  qtwebsocketsStatic = mkQtModule {
    name = "qtwebsockets";
    src = pkgs.qt6.qtwebsockets.src;
    # qtwebsockets ships a QML plugin; Quick pulls shadertools transitively.
    qtDeps = [
      qtdeclarativeStatic
      qtshadertoolsStatic
    ];
  };

  # One prefix for the app configure: find_package(Qt6 ...) sees every
  # module because the Qt6 config resolves its install prefix from its own
  # (symlinked) location, and CMake does not resolve symlinks for list files.
  qtStaticJoined = pkgs.symlinkJoin {
    name = "qt-linux-static-${qtVersion}";
    paths = [
      qtbaseStatic
      qtshadertoolsStatic
      qtdeclarativeStatic
      qtsvgStatic
      qtwebsocketsStatic
    ];
  };

  # MicroTeX links tinyxml2 via pkg-config; nixpkgs' tinyxml-2 is a shared
  # library that would put a /nix/store path into the portable binary's
  # runtime, so build the same pinned source statically (the wasm stack does
  # the identical thing with the emscripten toolchain).
  tinyxml2Static = staticStdenv.mkDerivation {
    pname = "tinyxml2-linux-static";
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
      # tinyxml2's .pc template joins ${prefix} with these verbatim, so the
      # nixpkgs cmake hook's absolute defaults would produce broken paths
      # (nixpkgs#144170); keep them relative.
      "-DCMAKE_INSTALL_INCLUDEDIR=include"
      "-DCMAKE_INSTALL_LIBDIR=lib"
    ];
  };

  # --- daemon-app against the static Qt --------------------------------------
  # DAEMON_APP_STATIC=ON is the app-side gate: TerminalPanelStub instead of
  # the shared QMLTermWidget plugin (GPL-2.0 stays out of this artifact, see
  # THIRD-PARTY-NOTICES.md), no qtkeychain (QSettings token-store fallback),
  # and the static platform-plugin imports. GUI only - the TUI stays a
  # dynamic build.
  #
  # Unlike the wasm app derivation this one keeps the normal nixpkgs cmake
  # hook: the toolchain is the host toolchain, and the hook's buildInputs ->
  # CMAKE_PREFIX_PATH wiring is exactly what the static Qt6*Config's
  # find_dependency(XCB/Fontconfig/...) calls need. Qt6LinguistTools comes
  # from the host qttools package pinned by _DIR (same 6.x version) instead
  # of buildInputs: the nixpkgs qtbase setup hook it drags along would
  # export the SHARED desktop Qt onto the prefix path and shadow the static
  # one.
  appStatic = staticStdenv.mkDerivation {
    pname = "daemon-app-static";
    version = baseVersion;
    src = appSrc;

    nativeBuildInputs = with pkgs; [
      cmake
      ninja
      pkg-config
      kdePackages.extra-cmake-modules # vendored KSyntaxHighlighting: find_package(ECM)
      perl # KSyntaxHighlighting generates the PHP syntax definition at build
      wayland-scanner # static Qt6Gui consumers re-resolve Qt6WaylandScannerTools
    ];

    buildInputs = [
      qtStaticJoined
      tinyxml2Static
    ]
    ++ platformLibs;

    cmakeFlags = [
      "-DCMAKE_BUILD_TYPE=Release"
      "-DBUILD_SHARED_LIBS=OFF"
      "-DBUILD_TESTING=OFF"
      "-DDAEMON_APP_TUI=OFF"
      "-DDAEMON_APP_STATIC=ON"
      "-DQt6LinguistTools_DIR=${pkgs.qt6.qttools}/lib/cmake/Qt6LinguistTools"
      "-DMD4QT_SOURCE_DIR=${depSources.md4qt}"
      "-DEARCUT_SOURCE_DIR=${depSources.earcut}"
      "-DKSYNTAXHIGHLIGHTING_SOURCE_DIR=${depSources.ksyntaxhighlighting}"
      "-DMICROTEX_SOURCE_DIR=${depSources.microtex}"
      "-DDAEMON_APP_VERSION_STR=${versionStr}"
      # Auto-updater dials (packaging/UPDATES.md). Notify tier: this one static
      # binary feeds the AppImage, deb, rpm, and portable tarball, so the
      # compiled artifact kind is portable-tar and the AppImage case is detected
      # at runtime ($APPIMAGE). deb/rpm share the binary and report portable-tar;
      # at Notify that only affects which row is matched (it degrades to a
      # notify-with-notes when no exact row matches).
      "-DDAEMON_APP_UPDATE_CAPABILITY=Notify"
      "-DDAEMON_APP_UPDATE_FEED_URL=https://github.com/daemon-ai/daemon/releases/latest/download/manifest.json"
      "-DDAEMON_APP_UPDATE_PUBKEY=RWRXpowS90Fy+TYhRsrBbQNSDvjbtJpqi9T89OGqSNTLkOa5vn62hK0o"
      "-DDAEMON_APP_UPDATE_ARTIFACT_KIND=portable-tar"
    ];

    # Portability: no GLIBCXX_x/GCC_x floor in the shipped binary (glibc stays
    # the only versioned dependency; the smoke prints that floor). Array form:
    # the flag pair must survive the cmake hook's whitespace splitting.
    #
    # CMAKE_CXX_STANDARD_LIBRARIES appends the static-leaf archives AFTER
    # every target's recorded link libraries. Two jobs: (1) libffi.a - Qt's
    # FindWayland records only the wayland archives, never their
    # Requires.private, so wayland-client's marshalling dependency must be
    # closed here; (2) archive-order insurance - ld resolves a trailing
    # archive group left-to-right, so any inter-archive undefined left by the
    # CMake-recorded order (xcb-cursor -> xcb-render-util/xcb-image, ...)
    # picks its symbols up here. Redundant archives contribute nothing to a
    # static link, so over-listing is free.
    preConfigure = ''
      cmakeFlagsArray+=("-DCMAKE_EXE_LINKER_FLAGS=-static-libstdc++ -static-libgcc")
      cmakeFlagsArray+=("-DCMAKE_CXX_STANDARD_LIBRARIES=${
        lib.concatMapStringsSep " " (a: "${a}") [
          "${xcbCursorStatic}/lib/libxcb-cursor.a"
          "${xcbWmStatic}/lib/libxcb-icccm.a"
          "${xcbWmStatic}/lib/libxcb-ewmh.a"
          "${xcbImageStatic}/lib/libxcb-image.a"
          "${xcbKeysymsStatic}/lib/libxcb-keysyms.a"
          "${xcbRenderUtilStatic}/lib/libxcb-render-util.a"
          "${xcbUtilStatic}/lib/libxcb-util.a"
          "${xkbcommonStatic}/lib/libxkbcommon-x11.a"
          "${xkbcommonStatic}/lib/libxkbcommon.a"
          "${waylandStatic}/lib/libwayland-cursor.a"
          "${waylandStatic}/lib/libwayland-egl.a"
          "${waylandStatic}/lib/libwayland-client.a"
          "${libffiStatic}/lib/libffi.a"
        ]
      }")
    '';
  };

  # --- packages.portable ------------------------------------------------------
  # The distributable layout: a patchelf'd, fully stripped binary that boots
  # via the GENERIC distro loader (/lib64/ld-linux-x86-64.so.2 - not a Nix
  # store path), rpath $ORIGIN[/../lib] so libraries could ship alongside
  # later, MicroTeX's runtime resources under share/daemon-app/microtex-res
  # (today the binary still reads the baked store path; the runtime-relative
  # lookup of exactly this location lands with the pkg/size-wins branch),
  # and the third-party notices.
  #
  # remove-references-to scrubs the static Qt prefix strings QLibraryInfo
  # bakes into libQt6Core.a (nothing is resolved through them at runtime -
  # plugins and QML are compiled in), so the store closure of the portable
  # output stays the MicroTeX resources instead of the whole Qt stack.
  portable = pkgs.runCommand "daemon-app-portable-${baseVersion}" {
    nativeBuildInputs = with pkgs; [
      patchelf
      removeReferencesTo
      binutils
    ];
  } ''
    mkdir -p "$out/bin" "$out/share/daemon-app" "$out/share/doc/daemon-app"

    cp ${appStatic}/bin/daemon-app "$out/bin/daemon-app"
    chmod u+w "$out/bin/daemon-app"

    # Full strip first (the stdenv fixup only strips debug info), then
    # rewrite the loader + rpath for the generic-distro floor.
    strip --strip-all "$out/bin/daemon-app"
    patchelf \
      --set-interpreter /lib64/ld-linux-x86-64.so.2 \
      --set-rpath '$ORIGIN:$ORIGIN/../lib' \
      "$out/bin/daemon-app"

    for ref in ${qtbaseStatic} ${qtshadertoolsStatic} ${qtdeclarativeStatic} \
               ${qtsvgStatic} ${qtwebsocketsStatic} ${tinyxml2Static} \
               ${lib.concatStringsSep " " (map toString staticLeaves)}; do
      remove-references-to -t "$ref" "$out/bin/daemon-app"
    done

    cp -r ${depSources.microtex}/res "$out/share/daemon-app/microtex-res"
    cp ${appSrc}/THIRD-PARTY-NOTICES.md "$out/share/doc/daemon-app/THIRD-PARTY-NOTICES.md"
  '';

  # The one-file deliverable next to the layout above.
  tarball = pkgs.runCommand "daemon-app-portable-tarball-${baseVersion}" {
    nativeBuildInputs = [ pkgs.zstd ];
  } ''
    mkdir -p "$out" staging/daemon-app-portable-x86_64
    cp -r ${portable}/. staging/daemon-app-portable-x86_64/
    tar -C staging \
      --sort=name --owner=0 --group=0 --numeric-owner --mtime='@1' \
      -cf - daemon-app-portable-x86_64 \
      | zstd -19 -T0 -o "$out/daemon-app-portable-x86_64.tar.zst"
  '';

  # --- Boot smoke --------------------------------------------------------------
  # Two halves (buildFHSEnv needs user namespaces, which the nix build
  # sandbox does not hand out, so the FHS boot cannot be a check):
  #
  #  * checks.portable-boot-smoke (this derivation): pure, sandbox-safe.
  #    Asserts the ELF contract (generic interpreter, $ORIGIN rpath, the
  #    DT_NEEDED set stays within the documented system floor), prints the
  #    glibc floor (max GLIBC_x.y - the future release-manifest field), and
  #    ACTUALLY BOOTS the binary offscreen by invoking the store glibc's
  #    loader directly with an explicit --library-path (the sandbox has no
  #    /lib64): DAEMON_APP_WAIT_READY must print its sentinel and exit 0.
  #  * apps.portable-smoke: the same boot inside a buildFHSEnv simulating a
  #    generic distro root (real /lib64 loader + X/GL floor), run on a dev
  #    machine (`nix run .#portable-smoke`).
  #
  # DAEMON_APP_WAIT_READY contract (main.cpp): load the full QML scene, run
  # the first-run connect, block until the connection seam reports ready,
  # print "DAEMON_APP_READY ok" and exit 0 (2 on timeout). The smoke runs
  # DAEMON_APP_SERVICE_MODE=mock with DAEMON_APP_SOCKET pointed at an
  # existing stub file: the mock's local-mode plausibility check is "target
  # exists", so the scene must come up AND the state machine must settle for
  # the check to pass - no daemon needed in the sandbox.
  smokeEnv = ''
    export QT_QPA_PLATFORM=offscreen
    export QT_QUICK_BACKEND=software
    export DAEMON_APP_SERVICE_MODE=mock
    export DAEMON_APP_WAIT_READY=20000
  '';

  # The allowlisted DT_NEEDED floor (sonames), aligned with the AppImage
  # community excludelist's system-guaranteed set: glibc + GL vendor stack +
  # libX11/libxcb CORE (libxcb-*.so sub-libs here ship with the libxcb
  # package itself on every distro) + fontconfig/freetype. The xcb-util
  # family, xkbcommon, and the wayland client libs are NOT here on purpose:
  # they are statically linked into the binary (staticLeaves above) exactly
  # because no distro guarantees them (libxcb-cursor.so.0 was a real AppImage
  # loader failure). Anything new must be argued into this list (or linked
  # statically) rather than silently shipped.
  allowedNeeded = [
    "libc.so.6"
    "libm.so.6"
    "libpthread.so.0"
    "libdl.so.2"
    "librt.so.1"
    "libresolv.so.2" # glibc member (Qt Network res_* lookups)
    "ld-linux-x86-64.so.2"
    "libGL.so.1"
    "libEGL.so.1"
    "libGLX.so.0"
    "libOpenGL.so.0"
    "libX11.so.6"
    "libX11-xcb.so.1"
    "libxcb.so.1"
    "libxcb-randr.so.0"
    "libxcb-render.so.0"
    "libxcb-shape.so.0"
    "libxcb-shm.so.0"
    "libxcb-sync.so.1"
    "libxcb-xfixes.so.0"
    "libxcb-xkb.so.1"
    "libxcb-glx.so.0"
    "libfontconfig.so.1"
    "libfreetype.so.6"
  ];

  bootLibPath = lib.makeLibraryPath (
    platformLibs
    ++ (with pkgs; [
      glibc
      stdenv.cc.cc.lib # libstdc++ shouldn't be needed (linked static) but harmless here
    ])
  );

  bootSmoke = pkgs.runCommand "portable-boot-smoke" {
    nativeBuildInputs = with pkgs; [
      binutils
      patchelf
    ];
  } ''
    bin=${portable}/bin/daemon-app
    mkdir -p "$out"
    report="$out/report.txt"

    echo "== portable-boot-smoke: $bin" | tee "$report"

    # 1. Loader + rpath contract.
    interp=$(patchelf --print-interpreter "$bin")
    echo "interpreter: $interp" | tee -a "$report"
    [ "$interp" = "/lib64/ld-linux-x86-64.so.2" ] || { echo "FAIL: unexpected interpreter" >&2; exit 1; }
    rpath=$(patchelf --print-rpath "$bin")
    echo "rpath: $rpath" | tee -a "$report"
    [ "$rpath" = '$ORIGIN:$ORIGIN/../lib' ] || { echo "FAIL: unexpected rpath" >&2; exit 1; }

    # 2. DT_NEEDED stays within the documented system floor.
    echo "-- DT_NEEDED --" | tee -a "$report"
    readelf -d "$bin" | awk '/NEEDED/ { gsub(/[][]/, "", $5); print $5 }' | tee needed.txt >> "$report"
    fail=0
    while read -r so; do
      case " ${lib.concatStringsSep " " allowedNeeded} " in
        *" $so "*) ;;
        *) echo "FAIL: DT_NEEDED '$so' is outside the allowed system floor" >&2; fail=1 ;;
      esac
    done < needed.txt
    [ "$fail" = 0 ] || exit 1
    if grep -q 'libstdc++' needed.txt; then
      echo "FAIL: libstdc++ leaked into DT_NEEDED (-static-libstdc++ regressed)" >&2
      exit 1
    fi

    # 3. glibc floor readout (release-manifest field later).
    floor=$(readelf -V "$bin" | grep -o 'GLIBC_[0-9.]*' | sort -Vu | tail -n1)
    echo "glibc floor: $floor" | tee -a "$report"

    # 4. Offscreen boot through the store loader (no /lib64 in the sandbox).
    export HOME="$TMPDIR/home"; mkdir -p "$HOME"
    export XDG_RUNTIME_DIR="$TMPDIR/xdg"; mkdir -p "$XDG_RUNTIME_DIR"; chmod 700 "$XDG_RUNTIME_DIR"
    touch "$TMPDIR/daemon-sock-stub"
    export DAEMON_APP_SOCKET="$TMPDIR/daemon-sock-stub"
    ${smokeEnv}
    echo "-- boot (offscreen, mock service mode) --" | tee -a "$report"
    if ${pkgs.glibc}/lib/ld-linux-x86-64.so.2 --library-path "${bootLibPath}" \
         "$bin" > boot.log 2>&1; then
      status=0
    else
      status=$?
    fi
    tail -n 20 boot.log >> "$report"
    grep -q 'DAEMON_APP_READY ok' boot.log || { echo "FAIL: no ready sentinel; log:" >&2; cat boot.log >&2; exit 1; }
    [ "$status" = 0 ] || { echo "FAIL: exit $status; log:" >&2; cat boot.log >&2; exit 1; }
    echo "boot: DAEMON_APP_READY ok (exit 0)" | tee -a "$report"
  '';

  # A generic-distro root: glibc's real /lib64 loader + the X/GL floor and
  # nothing Qt- or Nix-specific. `nix run .#portable-smoke` boots the
  # portable binary in it exactly like the in-sandbox check does (offscreen,
  # mock service mode) and additionally proves the /lib64 interpreter path
  # works, which the sandboxed check cannot.
  fhsEnv = pkgs.buildFHSEnv {
    name = "daemon-app-portable-fhs";
    targetPkgs =
      p: with p; [
        libxcb
        libx11
        libxcb-util
        libxcb-image
        libxcb-keysyms
        libxcb-render-util
        libxcb-wm
        libxcb-cursor
        libxkbcommon
        wayland
        libglvnd
        fontconfig
        freetype
        dejavu_fonts
        binutils # readelf for the in-root glibc-floor readout
      ];
    runScript = "bash";
  };

  fhsInner = pkgs.writeText "portable-smoke-inner.sh" ''
    set -eu
    export HOME="$SMOKE_TMP/home"; mkdir -p "$HOME"
    export XDG_RUNTIME_DIR="$SMOKE_TMP/xdg"; mkdir -p "$XDG_RUNTIME_DIR"; chmod 700 "$XDG_RUNTIME_DIR"
    export DAEMON_APP_SOCKET="$SMOKE_TMP/daemon-sock-stub"
    ${smokeEnv}
    echo "-- ldd (inside the FHS root) --"
    ldd ${portable}/bin/daemon-app
    echo "-- glibc floor --"
    readelf -V ${portable}/bin/daemon-app | grep -o 'GLIBC_[0-9.]*' | sort -Vu | tail -n1
    echo "-- boot (offscreen, mock service mode) --"
    ${portable}/bin/daemon-app
  '';

  fhsSmoke = pkgs.writeShellApplication {
    name = "portable-smoke";
    text = ''
      tmp=$(mktemp -d)
      trap 'rm -rf "$tmp"' EXIT
      touch "$tmp/daemon-sock-stub"
      echo "== portable FHS boot smoke (generic distro root) =="
      if SMOKE_TMP="$tmp" ${fhsEnv}/bin/daemon-app-portable-fhs ${fhsInner}; then
        echo "portable-smoke: OK (DAEMON_APP_READY, exit 0)"
      else
        status=$?
        echo "portable-smoke: FAILED (exit $status)" >&2
        exit "$status"
      fi
    '';
  };
in
{
  qtbase = qtbaseStatic;
  qtshadertools = qtshadertoolsStatic;
  qtdeclarative = qtdeclarativeStatic;
  qtsvg = qtsvgStatic;
  qtwebsockets = qtwebsocketsStatic;
  qtStatic = qtStaticJoined;
  tinyxml2 = tinyxml2Static;
  app = appStatic;
  inherit
    portable
    tarball
    bootSmoke
    fhsSmoke
    # The packaging outputs (flake mkLinuxArtifacts + artifact-sanity) reuse
    # the portable contract pieces: the DT_NEEDED allowlist, the sandbox boot
    # library path, and the store prefixes to scrub from shipped binaries.
    allowedNeeded
    bootLibPath
    ;
  scrubPrefixes = [
    (toString qtbaseStatic)
    (toString qtshadertoolsStatic)
    (toString qtdeclarativeStatic)
    (toString qtsvgStatic)
    (toString qtwebsocketsStatic)
    (toString tinyxml2Static)
  ]
  ++ map toString staticLeaves;
}
