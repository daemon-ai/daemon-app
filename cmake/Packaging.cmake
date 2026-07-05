# SPDX-License-Identifier: MPL-2.0
# SPDX-FileCopyrightText: 2026 Jarrad Hope
#
# Desktop packaging: freedesktop assets, the macOS bundle payload + Qt deploy
# script, bundled sibling binaries, and CPack (Linux: DEB / RPM / AppImage;
# Windows: NSIS over the MinGW cross build; macOS: DragNDrop .dmg).
#
# The Linux packages carry the STATIC-Qt payload: the flake's artifact
# outputs configure the DAEMON_APP_STATIC build with
# DAEMON_APP_PACKAGE_PORTABLE=ON, and the staged tree is rewritten for the
# generic-distro floor before each generator wraps it (generic /lib64
# loader, $ORIGIN rpaths, side-payload prune, store-prefix scrub - see
# cmake/PackagePortablePayload.cmake). checks.artifact-sanity gates the
# result.
#
# The macOS (APPLE) side is CODE-ONLY so far: written and lint-checked on
# Linux, never yet configured or packaged on a mac. It still stages the
# Nix-linked dynamic build; see packaging/macos/README.md before trusting a
# produced .dmg.

include_guard(GLOBAL)

# --- Bundled sibling binaries ----------------------------------------------
# The product ships the Rust daemon + daemon-infer (and optionally daemon-cli)
# side by side with the app: LocalDaemonLauncher discovers bin/daemon next to
# bin/daemon-app, no wrapper scripts. The superproject flake passes prebuilt
# store paths through these cache vars; a standalone app build leaves them
# empty and packages the app alone.
set(DAEMON_APP_BUNDLED_DAEMON
    ""
    CACHE FILEPATH
    "Absolute path to a prebuilt daemon binary to bundle as bin/daemon"
)
set(DAEMON_APP_BUNDLED_DAEMON_INFER
    ""
    CACHE FILEPATH
    "Absolute path to a prebuilt daemon-infer binary to bundle as bin/daemon-infer"
)
set(DAEMON_APP_BUNDLED_DAEMON_CLI
    ""
    CACHE FILEPATH
    "Absolute path to a prebuilt daemon-cli binary to bundle as bin/daemon-cli"
)
# Runtime libraries the bundled binaries need beyond the documented system
# floor (e.g. daemon-infer links libstdc++/libgomp from the nix gcc, whose
# GLIBCXX may be newer than the target distro's) - staged into lib/, resolved
# via the $ORIGIN/../lib rpath the packaging pre-build script writes.
set(DAEMON_APP_BUNDLED_LIBS
    ""
    CACHE STRING
    "Semicolon list of prebuilt shared libraries to bundle into lib/"
)

# Desktop-only: the wasm bundle installs its own artifact set and the mobile
# platforms package through their native toolchains.
if(DAEMON_APP_WASM OR DAEMON_APP_MOBILE)
    return()
endif()

include(GNUInstallDirs)

# --- Platform payload destinations -------------------------------------------
# Linux follows GNUInstallDirs (bin/, share/doc/...). macOS routes the same
# payloads into the app bundle: sibling binaries next to the app executable in
# Contents/MacOS (LocalDaemonLauncher probes
# QCoreApplication::applicationDirPath(), which IS Contents/MacOS inside a
# bundle), docs into Contents/Resources. The literal bundle directory composes
# with the `install(TARGETS daemon-app BUNDLE DESTINATION .)` rule in
# src/DaemonApp/App/CMakeLists.txt: the bundle lands at <prefix>/daemon-app.app
# both under `cmake --install` and in CPack's DragNDrop staging root.
if(APPLE)
    set(_da_bundle_contents "daemon-app.app/Contents")
    set(_da_colocated_bin_dir "${_da_bundle_contents}/MacOS")
    set(_da_doc_dir "${_da_bundle_contents}/Resources")
else()
    set(_da_colocated_bin_dir "${CMAKE_INSTALL_BINDIR}")
    set(_da_doc_dir "${CMAKE_INSTALL_DATADIR}/doc/daemon-app")
endif()

# Windows executables must keep the `.exe` extension: without it the installed tree ships
# `bin\daemon` (a headless PE Windows won't run by name) and LocalDaemonLauncher's co-located
# discovery (which probes `daemon.exe` next to the app) can never find it. Preserve the suffix on
# Windows; other platforms keep the bare name.
if(WIN32)
    set(_da_exe_suffix ".exe")
else()
    set(_da_exe_suffix "")
endif()
if(DAEMON_APP_BUNDLED_DAEMON)
    install(
        PROGRAMS "${DAEMON_APP_BUNDLED_DAEMON}"
        DESTINATION "${_da_colocated_bin_dir}"
        RENAME "daemon${_da_exe_suffix}"
    )
endif()
if(DAEMON_APP_BUNDLED_DAEMON_INFER)
    install(
        PROGRAMS "${DAEMON_APP_BUNDLED_DAEMON_INFER}"
        DESTINATION "${_da_colocated_bin_dir}"
        RENAME "daemon-infer${_da_exe_suffix}"
    )
endif()
if(DAEMON_APP_BUNDLED_DAEMON_CLI)
    install(
        PROGRAMS "${DAEMON_APP_BUNDLED_DAEMON_CLI}"
        DESTINATION "${_da_colocated_bin_dir}"
        RENAME "daemon-cli${_da_exe_suffix}"
    )
endif()
if(DAEMON_APP_BUNDLED_LIBS)
    if(APPLE)
        set(_da_bundled_lib_dir "${_da_bundle_contents}/Frameworks")
    elseif(WIN32)
        # Windows: the PE loader resolves a DLL from the directory of the
        # importing executable (and PATH), never from a sibling lib/. The
        # bundled worker DLLs (ggml/llama/mtmd + the MinGW runtime) must
        # therefore land in bin/ beside daemon-infer.exe, not lib/.
        set(_da_bundled_lib_dir "${_da_colocated_bin_dir}")
    else()
        set(_da_bundled_lib_dir "${CMAKE_INSTALL_LIBDIR}")
    endif()
    install(
        PROGRAMS ${DAEMON_APP_BUNDLED_LIBS}
        DESTINATION "${_da_bundled_lib_dir}"
    )
endif()

# Co-located node binary relocatability backstop (APPLE, bundled build only).
# The nix-built Rust node binaries carry an absolute /nix/store install name
# for libiconv (the nix libiconv, not the system one), which dangles on any mac
# without a nix store - the daemon would fail to load. libiconv is a macOS
# system library resolved from the dyld shared cache at /usr/lib, so repoint the
# store reference there. Any OTHER residual /nix/store reference has no system
# mapping and is flagged loudly (it would need bundling). install_name_tool
# voids the ad-hoc signature, so re-sign each edited binary. Runs after the
# install(PROGRAMS) copies above.
if(
    APPLE
    AND (
        DAEMON_APP_BUNDLED_DAEMON
        OR DAEMON_APP_BUNDLED_DAEMON_INFER
        OR DAEMON_APP_BUNDLED_DAEMON_CLI
    )
)
    install(
        CODE
            [==[
        set(_da_macos "${CMAKE_INSTALL_PREFIX}/daemon-app.app/Contents/MacOS")
        foreach(_da_nb daemon daemon-infer daemon-cli)
            set(_da_p "${_da_macos}/${_da_nb}")
            if(NOT EXISTS "${_da_p}")
                continue()
            endif()
            execute_process(COMMAND chmod u+w "${_da_p}")
            execute_process(COMMAND otool -L "${_da_p}"
                OUTPUT_VARIABLE _da_o OUTPUT_STRIP_TRAILING_WHITESPACE)
            string(REPLACE "\n" ";" _da_lines "${_da_o}")
            set(_da_changed FALSE)
            foreach(_da_line IN LISTS _da_lines)
                string(REGEX MATCH "/nix/store/[^ ]+[.]dylib" _da_ref "${_da_line}")
                if(_da_ref)
                    get_filename_component(_da_b "${_da_ref}" NAME)
                    if(_da_b MATCHES "^libiconv")
                        message(STATUS "daemon-app: repointing ${_da_nb} ${_da_ref} -> /usr/lib/${_da_b}")
                        execute_process(COMMAND install_name_tool -change
                            "${_da_ref}" "/usr/lib/${_da_b}" "${_da_p}")
                        set(_da_changed TRUE)
                    else()
                        message(WARNING "daemon-app: ${_da_nb} keeps store reference ${_da_ref} with no system-lib mapping (will dangle off-nix)")
                    endif()
                endif()
            endforeach()
            if(_da_changed)
                execute_process(COMMAND codesign --force --sign - "${_da_p}")
            endif()
        endforeach()
    ]==]
    )
endif()

# License + third-party notices ship with every package. EXISTS-guarded: the
# notices file lands on a sibling branch and must not break a lean checkout.
foreach(_da_doc LICENSE THIRD-PARTY-NOTICES.md)
    if(EXISTS "${CMAKE_SOURCE_DIR}/${_da_doc}")
        install(
            FILES "${CMAKE_SOURCE_DIR}/${_da_doc}"
            DESTINATION "${_da_doc_dir}"
        )
    endif()
endforeach()

# --- Linux desktop assets ---------------------------------------------------
if(UNIX AND NOT APPLE)
    set(_da_linux_dir "${CMAKE_SOURCE_DIR}/packaging/linux")

    install(
        FILES "${_da_linux_dir}/daemon-app.desktop"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )

    # AppStream metainfo: the <release> stub reads the VERSION-derived project
    # version at configure time. The date honors SOURCE_DATE_EPOCH, so Nix
    # builds stay reproducible (epoch date) while dev builds show today.
    string(TIMESTAMP DAEMON_APP_METAINFO_DATE "%Y-%m-%d" UTC)
    configure_file(
        "${_da_linux_dir}/io.daemon.app.metainfo.xml.in"
        "${CMAKE_BINARY_DIR}/packaging/io.daemon.app.metainfo.xml"
        @ONLY
    )
    install(
        FILES "${CMAKE_BINARY_DIR}/packaging/io.daemon.app.metainfo.xml"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
    )

    # hicolor icons: committed rasters (packages must not rasterize at build
    # time; regen via packaging/linux/icons/regen.sh) + the scalable SVG.
    foreach(
        _da_icon_size
        16
        32
        48
        64
        128
        256
        512
    )
        install(
            FILES "${_da_linux_dir}/icons/daemon-app-${_da_icon_size}.png"
            DESTINATION
                ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${_da_icon_size}x${_da_icon_size}/apps
            RENAME daemon-app.png
        )
    endforeach()
    install(
        FILES "${_da_linux_dir}/icons/daemon-app.svg"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
    )
endif()

# --- macOS bundle payload + Qt deploy ----------------------------------------
# The bundle itself (MACOSX_BUNDLE, Info.plist, .icns) is declared on the
# target in src/DaemonApp/App/CMakeLists.txt; this block stages the rest of
# the bundle payload and bolts the Qt deployment onto the install.
if(APPLE)
    # MicroTeX's runtime resources (fonts + XML), staged where bundle
    # resources belong. application.cpp's microtexResDir() probes
    # applicationDirPath()/../Resources/microtex-res (among the other
    # layouts), so this bundle copy wins over the baked build path.
    install(
        DIRECTORY "${MICROTEX_RES_DIR}/"
        DESTINATION "${_da_bundle_contents}/Resources/microtex-res"
    )

    # macdeployqt equivalent: bundles the Qt frameworks + plugins and the QML
    # imports of every module the app uses into the .app (Frameworks/,
    # PlugIns/, Resources/qml/ + bundle qt.conf), rewriting install names.
    # Generated at configure time, executed at install time - so both
    # `cmake --install` and CPack's DragNDrop staging pass through it.
    # APPLE-only on purpose: the Linux artifacts need no deploy step (the
    # static payload is self-contained) and the wasm install set likewise.
    qt_generate_deploy_qml_app_script(
        TARGET daemon-app
        OUTPUT_SCRIPT _da_macos_deploy_script
        # Signing hook (Qt >= 6.7 forwards DEPLOY_TOOL_OPTIONS to
        # macdeployqt): uncomment once a Developer ID identity exists on the
        # build mac. This does NOT cover the bundled daemon/daemon-infer/
        # daemon-cli - hardened-runtime notarization needs those signed
        # individually BEFORE the bundle seal; see packaging/macos/README.md
        # ("Codesigning") for the full inside-out order.
        #
        # DEPLOY_TOOL_OPTIONS -hardened-runtime "-codesign=Developer ID Application: <name> (<team id>)"
    )
    install(SCRIPT "${_da_macos_deploy_script}")

    # macdeployqt relocatability backstop. macdeployqt copies the vendored
    # KF6SyntaxHighlighting dylib into Contents/Frameworks (reached via the
    # org.kde.syntaxhighlighting QML plugin's dependency chain) and rewrites the
    # plugin's reference to it, but it leaves the app executable's OWN load
    # command pointing at the dylib's absolute install path
    # (CMAKE_INSTALL_NAME_DIR = the nix-store $out/lib on this build). That path
    # is not resolvable at deploy time, so it is never rewritten and dangles the
    # moment the .app is copied out of the store into the DMG - the app then
    # aborts at launch with a dyld "Library not loaded" for the store path.
    # Repoint any residual absolute /nix/store reference left in the executable
    # at the bundled copy through the @rpath entry macdeployqt already added
    # (@executable_path/../Frameworks), but only when that copy is actually in
    # Frameworks, then re-seal the ad-hoc signature the edit voids. Runs after
    # the deploy script, so it sees the fully deployed bundle.
    install(
        CODE
            [==[
        set(_da_app "${CMAKE_INSTALL_PREFIX}/daemon-app.app")
        set(_da_exe "${_da_app}/Contents/MacOS/daemon-app")
        if(EXISTS "${_da_exe}")
            execute_process(COMMAND otool -L "${_da_exe}"
                OUTPUT_VARIABLE _da_otool OUTPUT_STRIP_TRAILING_WHITESPACE)
            string(REPLACE "\n" ";" _da_lines "${_da_otool}")
            set(_da_relocated FALSE)
            foreach(_da_line IN LISTS _da_lines)
                string(REGEX MATCH "/nix/store/[^ ]+[.]dylib" _da_ref "${_da_line}")
                if(_da_ref)
                    get_filename_component(_da_base "${_da_ref}" NAME)
                    if(EXISTS "${_da_app}/Contents/Frameworks/${_da_base}")
                        message(STATUS "daemon-app: relocating ${_da_ref} -> @rpath/${_da_base}")
                        execute_process(COMMAND install_name_tool -change
                            "${_da_ref}" "@rpath/${_da_base}" "${_da_exe}")
                        set(_da_relocated TRUE)
                    else()
                        message(WARNING "daemon-app: leftover store reference ${_da_ref} has no bundled copy in Frameworks")
                    endif()
                endif()
            endforeach()
            if(_da_relocated)
                execute_process(COMMAND codesign --force --sign - "${_da_app}")
            endif()
        endif()
    ]==]
    )

    # Offscreen QPA plugin deployment. macdeployqt deploys ONLY the plugin for
    # the build's current QPA - libqcocoa - so a bundle launched without a
    # window server (headless CI/SSH, and the DAEMON_APP_WAIT_READY boot
    # contract the other platforms gate on) has no platform plugin: cocoa needs
    # a display and aborts. Pointing QT_QPA_PLATFORM_PLUGIN_PATH at the Qt store
    # copy loads a SECOND Qt binary set (two QtCore) and crashes, so the
    # offscreen plugin must live INSIDE the bundle next to cocoa, with its Qt
    # references rewritten to resolve the bundle's own frameworks. Same
    # relocatability problem as the exe backstop above; here we rewrite every
    # residual /nix/store reference to an @loader_path path into the bundle's
    # Frameworks (Contents/PlugIns/platforms -> ../../Frameworks), drop the
    # plugin's store LC_RPATHs so no @rpath can fall back to the store Qt, then
    # ad-hoc sign the plugin and re-seal the bundle. The offscreen plugin's
    # framework deps (QtGui/QtCore/...) are a subset of cocoa's, so macdeployqt
    # already deployed everything it needs.
    if(TARGET Qt6::QOffscreenIntegrationPlugin)
        install(
            CODE
                [==[
            set(_da_off_src "$<TARGET_FILE:Qt6::QOffscreenIntegrationPlugin>")
            set(_da_app "${CMAKE_INSTALL_PREFIX}/daemon-app.app")
            set(_da_plugdir "${_da_app}/Contents/PlugIns/platforms")
            if(EXISTS "${_da_off_src}" AND IS_DIRECTORY "${_da_plugdir}")
                get_filename_component(_da_off_name "${_da_off_src}" NAME)
                set(_da_off "${_da_plugdir}/${_da_off_name}")
                message(STATUS "daemon-app: deploying offscreen QPA plugin ${_da_off_name}")
                file(COPY "${_da_off_src}" DESTINATION "${_da_plugdir}")
                execute_process(COMMAND chmod u+w "${_da_off}")
                # Rewrite each absolute store dependency to a bundle-relative
                # @loader_path reference (framework tail preserved, plain dylibs
                # by basename), so dyld never reaches the store Qt.
                execute_process(COMMAND otool -L "${_da_off}"
                    OUTPUT_VARIABLE _da_otool OUTPUT_STRIP_TRAILING_WHITESPACE)
                string(REPLACE "\n" ";" _da_lines "${_da_otool}")
                foreach(_da_line IN LISTS _da_lines)
                    string(REGEX MATCH "/nix/store/[^ ]+" _da_ref "${_da_line}")
                    if(_da_ref)
                        if(_da_ref MATCHES "[.]framework/")
                            string(REGEX REPLACE "^.*/([^/]+[.]framework/.*)$"
                                "@loader_path/../../Frameworks/\\1" _da_new "${_da_ref}")
                        else()
                            get_filename_component(_da_rb "${_da_ref}" NAME)
                            set(_da_new "@loader_path/../../Frameworks/${_da_rb}")
                        endif()
                        execute_process(COMMAND install_name_tool -change
                            "${_da_ref}" "${_da_new}" "${_da_off}")
                    endif()
                endforeach()
                # Drop store LC_RPATHs so a stray @rpath dep can't resolve to the
                # store Qt binary set.
                execute_process(COMMAND otool -l "${_da_off}"
                    OUTPUT_VARIABLE _da_load OUTPUT_STRIP_TRAILING_WHITESPACE)
                string(REGEX MATCHALL "path /nix/store/[^ ]+" _da_rpaths "${_da_load}")
                foreach(_da_rp IN LISTS _da_rpaths)
                    string(REPLACE "path " "" _da_rp "${_da_rp}")
                    execute_process(COMMAND install_name_tool -delete_rpath
                        "${_da_rp}" "${_da_off}" ERROR_QUIET)
                endforeach()
                execute_process(COMMAND codesign --force --sign - "${_da_off}")
                # Re-seal the bundle now that PlugIns changed.
                execute_process(COMMAND codesign --force --sign - "${_da_app}")
            else()
                message(WARNING "daemon-app: offscreen plugin or platforms dir missing; headless boot from the bundle will fail")
            endif()
        ]==]
        )
    else()
        message(
            WARNING
            "daemon-app: Qt6::QOffscreenIntegrationPlugin target not found; offscreen QPA plugin will not be bundled"
        )
    endif()

    # QMLTermWidget (embedded terminal) module deployment. It is a runtime QML
    # import supplied via the import path on Linux (the nix wrapQtAppsHook adds
    # qmltermwidget-qt6's lib/qml). macdeployqt does not discover it, so a bundle
    # omits it - and because Main.qml imports it UNCONDITIONALLY (Session ->
    # TerminalPanel -> `import QMLTermWidget`), QQmlApplicationEngine fails to
    # load the ROOT component (rootObjects() empty -> main returns early), so the
    # .app does not launch AT ALL, GUI or headless - not merely a dead terminal
    # panel. Deploy the module under Resources/qml (qt.conf's QML import root)
    # and rewrite its plugin dylib's store Qt references to @loader_path paths
    # into the bundle Frameworks (Resources/qml/QMLTermWidget -> ../../../
    # Frameworks), dropping store LC_RPATHs, then ad-hoc sign and re-seal. Same
    # relocatability backstop as the exe and the offscreen plugin. QMLTERMWIDGET_QML_DIR
    # is the flake's -D flag (nix/flake.nix depFlags) pointing at the plugin's
    # qml prefix; a bare dev configure without it skips deployment.
    if(
        DEFINED QMLTERMWIDGET_QML_DIR
        AND EXISTS "${QMLTERMWIDGET_QML_DIR}/QMLTermWidget"
    )
        install(
            DIRECTORY "${QMLTERMWIDGET_QML_DIR}/QMLTermWidget"
            DESTINATION "${_da_bundle_contents}/Resources/qml"
        )
        install(
            CODE
                [==[
            set(_da_app "${CMAKE_INSTALL_PREFIX}/daemon-app.app")
            set(_da_qtw "${_da_app}/Contents/Resources/qml/QMLTermWidget")
            # The module is copied from the read-only nix store; install_name_tool
            # writes a sibling temp file, so the DIRECTORY (not just the dylib) must
            # be writable.
            execute_process(COMMAND chmod -R u+w "${_da_qtw}")
            file(GLOB _da_qtw_dylibs "${_da_qtw}/*.dylib")
            foreach(_da_lib IN LISTS _da_qtw_dylibs)
                execute_process(COMMAND otool -L "${_da_lib}"
                    OUTPUT_VARIABLE _da_o OUTPUT_STRIP_TRAILING_WHITESPACE)
                string(REPLACE "\n" ";" _da_lines "${_da_o}")
                foreach(_da_line IN LISTS _da_lines)
                    string(REGEX MATCH "/nix/store/[^ ]+" _da_ref "${_da_line}")
                    if(_da_ref)
                        if(_da_ref MATCHES "[.]framework/")
                            string(REGEX REPLACE "^.*/([^/]+[.]framework/.*)$"
                                "@loader_path/../../../Frameworks/\\1" _da_new "${_da_ref}")
                        else()
                            get_filename_component(_da_rb "${_da_ref}" NAME)
                            set(_da_new "@loader_path/../../../Frameworks/${_da_rb}")
                        endif()
                        execute_process(COMMAND install_name_tool -change
                            "${_da_ref}" "${_da_new}" "${_da_lib}")
                    endif()
                endforeach()
                execute_process(COMMAND otool -l "${_da_lib}"
                    OUTPUT_VARIABLE _da_load OUTPUT_STRIP_TRAILING_WHITESPACE)
                string(REGEX MATCHALL "path /nix/store/[^ ]+" _da_rpaths "${_da_load}")
                foreach(_da_rp IN LISTS _da_rpaths)
                    string(REPLACE "path " "" _da_rp "${_da_rp}")
                    execute_process(COMMAND install_name_tool -delete_rpath
                        "${_da_rp}" "${_da_lib}" ERROR_QUIET)
                endforeach()
                execute_process(COMMAND codesign --force --sign - "${_da_lib}")
            endforeach()
            execute_process(COMMAND codesign --force --sign - "${_da_app}")
        ]==]
        )
    else()
        message(
            WARNING
            "daemon-app: QMLTERMWIDGET_QML_DIR unset or module missing; the embedded terminal will be absent and the bundle will fail to launch (Main.qml imports QMLTermWidget unconditionally)"
        )
    endif()
endif()

# --- CPack -------------------------------------------------------------------
# Per-generator specifics (packaging prefix, dependency lists, maintainer
# scripts, AppImage runtime/update-info) live in cmake/CPackPerGen.cmake,
# loaded by cpack once per generator.
#
# Package name: "daemon" (the product), not "daemon-app" - the DEB/RPM/
# AppImage ship the daemon + daemon-infer (+ daemon-cli) next to the app once
# the superproject injects them, so the artifact is named after the product
# users install, mirroring the superproject bundle label.
set(CPACK_PACKAGE_NAME "daemon")
set(CPACK_PACKAGE_VENDOR "daemon project")
# Placeholder contact/homepage until the public site lands.
set(CPACK_PACKAGE_CONTACT "Jarrad Hope <maintainers@daemon.io>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://daemon.io")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_DESCRIPTION
    "Daemon is an AI agent chat application. The desktop app is a thin client of the daemon node: it renders sessions, transcripts, models, and fleet state served over the daemon wire protocol, and ships alongside the daemon and daemon-infer binaries."
)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_CHECKSUM SHA256)

if(APPLE)
    # CMAKE_SYSTEM_PROCESSOR is arm64 / x86_64 on macOS.
    set(CPACK_PACKAGE_FILE_NAME
        "daemon-${PROJECT_VERSION}-macos-${CMAKE_SYSTEM_PROCESSOR}"
    )
    # Never re-strip Mach-O payloads at package time: every deployed binary
    # is at least ad-hoc signed (the linker on arm64, macdeployqt after its
    # install-name rewrites, codesign in the release flow), and stripping
    # invalidates the seal.
    set(CPACK_STRIP_FILES FALSE)
    set(CPACK_GENERATOR "DragNDrop")
elseif(WIN32)
    set(CPACK_PACKAGE_FILE_NAME "daemon-${PROJECT_VERSION}-win64")
    # The cross build tree is unstripped; strip at cpack-install.
    set(CPACK_STRIP_FILES TRUE)
    # NSIS (the Windows cross build) needs makensis on PATH
    # (nix/windows.nix provides it).
    set(CPACK_GENERATOR "NSIS")

    # The NSIS MUI license page renders exactly one file; ship LICENSE and
    # the third-party notices on it together (both files also install into
    # share/doc/daemon-app above). Generated at configure so cpack only
    # reads it.
    file(READ "${CMAKE_SOURCE_DIR}/LICENSE" _da_nsis_license)
    if(EXISTS "${CMAKE_SOURCE_DIR}/THIRD-PARTY-NOTICES.md")
        file(READ "${CMAKE_SOURCE_DIR}/THIRD-PARTY-NOTICES.md" _da_nsis_notices)
        string(
            APPEND _da_nsis_license
            "\n\n================================================================\nTHIRD-PARTY NOTICES\n================================================================\n\n${_da_nsis_notices}"
        )
    endif()
    file(
        WRITE "${CMAKE_BINARY_DIR}/packaging/nsis-license.txt"
        "${_da_nsis_license}"
    )
    set(CPACK_DAEMON_APP_NSIS_LICENSE
        "${CMAKE_BINARY_DIR}/packaging/nsis-license.txt"
    )

    # --- Per-user NSIS installer (decision: todo u3-windows) -----------------
    # We install per-user under $LOCALAPPDATA\Programs\Daemon (set in
    # cmake/CPackPerGen.cmake) so the auto-updater's SelfApply tier can run the
    # installer silently (/S) from the detached daemon-updater helper without a
    # UAC prompt (VS Code / Chrome user-setup model). CMake's stock
    # NSIS.template.in hardcodes `RequestExecutionLevel admin` and
    # `SetShellVarContext all` (per-machine, HKLM) with NO CPack variable to
    # flip either, so we emit a MINIMALLY patched copy of CMake's own template
    # onto CPACK_MODULE_PATH (cpack's FindTemplate searches it for
    # "NSIS.template.in" before falling back to CMAKE_ROOT). Patching CMake's
    # live template (rather than vendoring the whole ~1000-line file) keeps this
    # resilient to unrelated template changes; each substitution is guarded so a
    # CMake bump that renames a directive fails LOUDLY instead of silently
    # shipping a per-machine installer.
    set(_da_nsis_tmpl_src
        "${CMAKE_ROOT}/Modules/Internal/CPack/NSIS.template.in"
    )
    if(NOT EXISTS "${_da_nsis_tmpl_src}")
        message(FATAL_ERROR
            "daemon-app: CMake NSIS template not found at '${_da_nsis_tmpl_src}'; "
            "cannot produce the per-user installer (see cmake/CPackPerGen.cmake)."
        )
    endif()
    file(READ "${_da_nsis_tmpl_src}" _da_nsis_tmpl)

    # Each entry is "search|||replace"; the search MUST be present (else FATAL).
    # T1  no elevation (promptless self-apply);
    # T2  current-user shell context -> HKCU uninstall hive + per-user Start Menu
    #     (4 occurrences across .onInit and un.onInit);
    # T3  the JustMe default dir uses the per-user root instead of $DOCUMENTS, so
    #     admin and non-admin users both land in $LOCALAPPDATA\Programs\Daemon;
    # T4  uninstall-before-install reads the prior install from HKCU (where a
    #     per-user install records it), matching the current-user context.
    set(_da_nsis_patches
        "RequestExecutionLevel admin|||RequestExecutionLevel user"
        "SetShellVarContext all|||SetShellVarContext current"
        "StrCpy $INSTDIR \"$DOCUMENTS\\@CPACK_PACKAGE_INSTALL_DIRECTORY@\"|||StrCpy $INSTDIR \"@CPACK_NSIS_INSTALL_ROOT@\\@CPACK_PACKAGE_INSTALL_DIRECTORY@\""
        "ReadRegStr $0 HKLM \"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\@CPACK_PACKAGE_INSTALL_REGISTRY_KEY@\" \"UninstallString\"|||ReadRegStr $0 HKCU \"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\@CPACK_PACKAGE_INSTALL_REGISTRY_KEY@\" \"UninstallString\""
        "ReadRegStr $1 HKLM \"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\@CPACK_PACKAGE_INSTALL_REGISTRY_KEY@\" \"DisplayName\"|||ReadRegStr $1 HKCU \"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\@CPACK_PACKAGE_INSTALL_REGISTRY_KEY@\" \"DisplayName\""
    )
    foreach(_da_patch IN LISTS _da_nsis_patches)
        string(REPLACE "|||" ";" _da_pair "${_da_patch}")
        list(GET _da_pair 0 _da_search)
        list(GET _da_pair 1 _da_repl)
        string(FIND "${_da_nsis_tmpl}" "${_da_search}" _da_found)
        if(_da_found EQUAL -1)
            message(FATAL_ERROR
                "daemon-app: per-user NSIS patch anchor not found in CMake's "
                "NSIS.template.in: '${_da_search}'. The stock template changed; "
                "review the per-user install patch in cmake/Packaging.cmake."
            )
        endif()
        string(REPLACE "${_da_search}" "${_da_repl}" _da_nsis_tmpl "${_da_nsis_tmpl}")
    endforeach()

    set(_da_nsis_tmpl_dir "${CMAKE_BINARY_DIR}/cpack-modules")
    file(WRITE "${_da_nsis_tmpl_dir}/NSIS.template.in" "${_da_nsis_tmpl}")
    # cpack searches CPACK_MODULE_PATH for NSIS.template.in ahead of CMAKE_ROOT.
    set(CPACK_MODULE_PATH "${_da_nsis_tmpl_dir}" ${CPACK_MODULE_PATH})
else()
    set(CPACK_PACKAGE_FILE_NAME
        "daemon-${PROJECT_VERSION}-linux-${CMAKE_SYSTEM_PROCESSOR}"
    )
    # The Nix build tree is unstripped; strip at cpack-install so artifacts
    # do not carry debug info.
    set(CPACK_STRIP_FILES TRUE)
    # Default generators for a bare `cpack`; the flake package outputs pass
    # -G. AppImage additionally needs CMake >= 4.2 + appimagetool
    # (nix/cmake-appimage.nix + nix/appimagetool.nix provide both).
    set(CPACK_GENERATOR "DEB;RPM")

    # dpkg is absent at build time (Nix sandbox), so pin the Debian
    # architecture name instead of letting the DEB generator shell out for it.
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
    else()
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
    endif()
endif()

# Stash source-tree locations + the nix-injected AppImage runtime for the
# per-generator config file (only CPACK_-prefixed variables survive into the
# serialized CPackConfig cpack reads).
set(CPACK_DAEMON_APP_LINUX_DIR "${_da_linux_dir}")
set(CPACK_DAEMON_APP_MACOS_DIR "${CMAKE_SOURCE_DIR}/packaging/macos")
set(CPACK_DAEMON_APP_WINDOWS_DIR "${CMAKE_SOURCE_DIR}/packaging/windows")
set(DAEMON_APP_APPIMAGE_RUNTIME
    ""
    CACHE FILEPATH
    "Prebuilt AppImage type2 runtime for cpack -G AppImage (injected by the flake)"
)
set(CPACK_DAEMON_APP_APPIMAGE_RUNTIME "${DAEMON_APP_APPIMAGE_RUNTIME}")

# --- Portable payload rewrite (Linux packages over the static-Qt build) ------
# The flake's packaging outputs configure the static-Qt app with
# DAEMON_APP_PACKAGE_PORTABLE=ON: after cpack stages the install tree, the
# pre-build script prunes the vendored KSyntaxHighlighting side-payload and
# rewrites every staged ELF for the generic-distro floor (generic /lib64
# loader, $ORIGIN rpaths, store-prefix scrub) - the packaged twin of what
# nix/portable.nix does to the portable tarball layout. OFF for developer
# configures: `cpack` from a dev tree keeps packaging the build as-is.
option(
    DAEMON_APP_PACKAGE_PORTABLE
    "Rewrite the staged CPack payload for a generic distro root (patchelf + prune; flake packaging outputs only)"
    OFF
)
set(DAEMON_APP_SCRUB_PREFIXES
    ""
    CACHE STRING
    "Semicolon list of store prefixes remove-references-to scrubs from the staged daemon-app binary"
)
if(DAEMON_APP_PACKAGE_PORTABLE)
    set(CPACK_PRE_BUILD_SCRIPTS
        "${CMAKE_SOURCE_DIR}/cmake/PackagePortablePayload.cmake"
    )
    set(CPACK_DAEMON_APP_SCRUB_PREFIXES "${DAEMON_APP_SCRUB_PREFIXES}")
endif()

set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_SOURCE_DIR}/cmake/CPackPerGen.cmake")

include(CPack)
