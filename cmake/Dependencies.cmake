# Wires the third-party sources that the Nix flake exposes as <DEP>_SOURCE_DIR
# cache variables into the build via add_subdirectory (built statically).
#
# No git submodules, no FetchContent: the flake pins and provides every source.

include_guard(GLOBAL)

# We vendor two KDE/ECM projects via add_subdirectory (md4qt and, below,
# KSyntaxHighlighting). With ECM on the build env, each one runs
# include(ECMGenerateQDoc), which unconditionally creates a fixed set of *global*
# documentation aggregate targets (prepare_docs, generate_docs, install_html_docs,
# generate_qch, install_qch_docs) at include time. The second include then aborts
# configure with "target ... already exists". KDECMakeSettings can likewise emit a
# shared "uninstall" target from two subprojects.
#
# Make duplicate-named custom targets a no-op so a second vendored ECM subproject
# reuses the first one's aggregate targets instead of colliding. Our own
# (first-party) build never creates two same-named custom targets, so this only
# ever absorbs the harmless cross-subproject duplicates. The original builtin is
# reachable via the underscore-prefixed name.
function(add_custom_target _name)
    if(TARGET ${_name})
        message(STATUS "Dependencies: reusing existing custom target '${_name}' (vendored ECM subproject duplicate).")
        return()
    endif()
    # Forward to the builtin WITHOUT collapsing the arguments through `${ARGV}`:
    # a bare `${ARGV}` re-splits every argument on ';', which corrupts any argument
    # that legitimately contains semicolons. Qt's qmllint wiring
    # (_qt_internal_target_enable_qmllint) passes the whole command as one quoted
    # generator expression, `COMMAND "$<IF:${have_files},${cmd},${cmd_dummy}>"`,
    # whose branches are ';'-joined lists relied upon by COMMAND_EXPAND_LISTS.
    # Re-splitting tears the "$<IF:...>" apart so CMake emits it literally, and
    # `all_qmllint` then fails with a shell syntax error. Escape embedded
    # semicolons per-argument so each argument survives the forward unchanged.
    set(_fwd_args "")
    math(EXPR _last_arg "${ARGC} - 1")
    foreach(_i RANGE 0 ${_last_arg})
        set(_a "${ARGV${_i}}")
        string(REPLACE ";" "\\;" _a "${_a}")
        list(APPEND _fwd_args "${_a}")
    endforeach()
    _add_custom_target(${_fwd_args})
endfunction()

# When clang-format is on the build env, ECM's KDEClangFormat (pulled in by the
# vendored frameworks' KDEFrameworkCompilerSettings) writes a generated
# .clang-format back into the *subproject's* source dir at include time. Those
# sources are read-only /nix/store paths, so the write aborts (re)configure
# (CMake regenerates on any CMakeLists change). Force that one lookup to
# NOTFOUND so the write is skipped - first-party formatting is handled outside
# CMake. Same class of read-only-source issue the .qmlls.ini toggle below guards.
function(find_program _var)
    if(_var STREQUAL "KDE_CLANG_FORMAT_EXECUTABLE")
        set(${_var} "${_var}-NOTFOUND"
            CACHE FILEPATH "Disabled for read-only vendored ECM subprojects" FORCE)
        return()
    endif()
    _find_program(${ARGV})
endfunction()

# Resolve a <DEP>_SOURCE_DIR from cache var or environment; FATAL if required & missing.
function(_daemon_app_resolve_dir out_var name)
    set(value "${${name}}")
    if(NOT value AND DEFINED ENV{${name}})
        set(value "$ENV{${name}}")
    endif()
    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

# ---------------------------------------------------------------------------
# md4qt - Markdown parser (header/library), all platforms
# ---------------------------------------------------------------------------
_daemon_app_resolve_dir(_md4qt_dir MD4QT_SOURCE_DIR)
if(NOT EXISTS "${_md4qt_dir}/CMakeLists.txt")
    message(FATAL_ERROR "MD4QT_SOURCE_DIR must point to an md4qt source tree (got '${_md4qt_dir}')")
endif()
set(BUILD_MD4QT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_MD4QT_BENCHMARK OFF CACHE BOOL "" FORCE)
set(INSTALL_MD4QT OFF CACHE BOOL "" FORCE)
add_subdirectory("${_md4qt_dir}" "${CMAKE_BINARY_DIR}/_deps/md4qt" EXCLUDE_FROM_ALL)

# ---------------------------------------------------------------------------
# earcut.hpp - header-only polygon triangulation, all platforms
# ---------------------------------------------------------------------------
_daemon_app_resolve_dir(_earcut_dir EARCUT_SOURCE_DIR)
if(NOT EXISTS "${_earcut_dir}/include/mapbox/earcut.hpp")
    message(FATAL_ERROR "EARCUT_SOURCE_DIR must point to a mapbox/earcut.hpp source tree (got '${_earcut_dir}')")
endif()
if(NOT TARGET earcut::earcut)
    add_library(earcut INTERFACE)
    add_library(earcut::earcut ALIAS earcut)
    target_include_directories(earcut INTERFACE "${_earcut_dir}/include")
endif()

# ---------------------------------------------------------------------------
# KSyntaxHighlighting - KDE syntax highlighting engine + its QML module
# (org.kde.syntaxhighlighting). Built from the pinned source tree; produces the
# C++ target KF6SyntaxHighlighting and the QML plugin kquicksyntaxhighlightingplugin.
# ---------------------------------------------------------------------------
_daemon_app_resolve_dir(_ksyntax_dir KSYNTAXHIGHLIGHTING_SOURCE_DIR)
if(NOT EXISTS "${_ksyntax_dir}/CMakeLists.txt")
    message(FATAL_ERROR "KSYNTAXHIGHLIGHTING_SOURCE_DIR must point to a KSyntaxHighlighting source tree (got '${_ksyntax_dir}')")
endif()

# GUI components carry the formats/themes the highlighter applies. Bundle the
# syntax + theme definitions into the library as Qt resources (QRC_SYNTAX) and
# skip QStandardPaths lookups (NO_STANDARD_PATHS) so the build needs no external
# data dirs and stays reproducible. Quick is already found by the root project,
# so the framework's src/quick (the QML module) builds.
set(KSYNTAXHIGHLIGHTING_USE_GUI ON CACHE BOOL "" FORCE)
set(QRC_SYNTAX ON CACHE BOOL "" FORCE)
set(NO_STANDARD_PATHS ON CACHE BOOL "" FORCE)

# Land the generated QML plugin in one predictable build-tree dir so it can be
# placed on the QML import path (devShell run + qmllint). Our own QML modules are
# STATIC (linked into the executable), so relocating their metadata here is benign.
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml" CACHE PATH "" FORCE)

# KSyntaxHighlighting pulls in KDECMakeSettings, which defaults BUILD_TESTING ON
# and would build the framework's own autotests with our tree. Force it off for
# its configure, then unset so our later include(CTest) still defaults ON.
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

# The root project enables QT_QML_GENERATE_QMLLS_INI, but that makes the
# framework's QML module (kquicksyntaxhighlightingplugin) try to write a
# .qmlls.ini back into its own source dir - which is a read-only /nix/store path,
# so the build fails. The .ini only aids the language server for first-party QML,
# so turn it off for this vendored subtree and restore it afterwards.
set(_da_prev_qmlls_ini "${QT_QML_GENERATE_QMLLS_INI}")
set(QT_QML_GENERATE_QMLLS_INI OFF)
# Cross static builds (wasm, mingw, android, iOS) share the rcc/zstd mismatch:
# rcc is a HOST tool (QT_HOST_PATH, zstd ON in nixpkgs' qtbase) while the TARGET
# Qt6Core is built with FEATURE_zstd=OFF (the iOS Qt sets it OFF explicitly, see
# nix/ios.nix). The native linux-static build runs its own zstd-less rcc and
# never hits this.
if(DAEMON_APP_WASM OR ANDROID OR IOS OR (DAEMON_APP_STATIC AND CMAKE_CROSSCOMPILING))
    # The host rcc compresses QRC payloads with zstd by default, but the wasm,
    # mingw, android and iOS Qt6Core are built without the zstd feature, so the
    # registrar's qResourceFeatureZstd() is undefined at link. Qt's own
    # resource pipeline passes --no-zstd when QT_FEATURE_zstd is off; the
    # framework's data/CMakeLists.txt invokes Qt6::rcc manually and does not.
    # Point the imported rcc at a wrapper forcing --no-zstd for every caller
    # (Qt's own calls then pass it twice, which rcc accepts).
    get_target_property(_da_rcc_real Qt6::rcc IMPORTED_LOCATION)
    if(NOT _da_rcc_real)
        get_target_property(_da_rcc_real Qt6::rcc IMPORTED_LOCATION_RELEASE)
    endif()
    set(_da_rcc_wrapper "${CMAKE_BINARY_DIR}/rcc-no-zstd.sh")
    file(WRITE "${_da_rcc_wrapper}" "#!/bin/sh\nexec \"${_da_rcc_real}\" --no-zstd \"$@\"\n")
    file(CHMOD "${_da_rcc_wrapper}" PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    # Every per-config location must point at the wrapper: Qt's
    # AdditionalTargetInfo file defines RELWITHDEBINFO/MINSIZEREL aliases, and
    # the generator resolves Qt6::rcc against the active CMAKE_BUILD_TYPE's
    # config before falling back to the plain IMPORTED_LOCATION.
    set_target_properties(Qt6::rcc PROPERTIES
        IMPORTED_LOCATION "${_da_rcc_wrapper}"
        IMPORTED_LOCATION_RELEASE "${_da_rcc_wrapper}"
        IMPORTED_LOCATION_RELWITHDEBINFO "${_da_rcc_wrapper}"
        IMPORTED_LOCATION_MINSIZEREL "${_da_rcc_wrapper}"
        IMPORTED_LOCATION_DEBUG "${_da_rcc_wrapper}")

    # The framework's CLI tool + example executables are useless payload on
    # these single-binary configs (and would pollute the NSIS install tree on
    # the mingw build); on wasm ECM's KDECompilerSettings additionally gives
    # every executable -Wl,--enable-new-dtags (an ELF-only flag wasm-ld
    # rejects); on android Qt would even package each one as its own APK at
    # finalization. EXCLUDE_FROM_ALL keeps only what the app links - the static
    # lib + QML plugin - in the build; both are static archives.
    add_subdirectory("${_ksyntax_dir}" "${CMAKE_BINARY_DIR}/_deps/ksyntaxhighlighting" EXCLUDE_FROM_ALL)
elseif(APPLE)
    # macOS ships the app as a self-contained .app bundle: macdeployqt copies
    # the KF6SyntaxHighlighting library and the org.kde.syntaxhighlighting QML
    # plugin (both of which the app links) into Contents/Frameworks + qml. The
    # framework's CLI tool, dev headers, CMake package files, and i18n catalogs
    # are useless payload that would otherwise leak into the DragNDrop DMG root
    # (the darwin analog of the KSyntaxHighlighting side-payload prune
    # PackagePortablePayload.cmake performs for the Linux packages). Same
    # EXCLUDE_FROM_ALL treatment as the single-binary configs above keeps them
    # out of the install/package tree while the linked lib + plugin still build.
    add_subdirectory("${_ksyntax_dir}" "${CMAKE_BINARY_DIR}/_deps/ksyntaxhighlighting" EXCLUDE_FROM_ALL)
else()
    add_subdirectory("${_ksyntax_dir}" "${CMAKE_BINARY_DIR}/_deps/ksyntaxhighlighting")
endif()
set(QT_QML_GENERATE_QMLLS_INI "${_da_prev_qmlls_ini}")
unset(BUILD_TESTING CACHE)

# ---------------------------------------------------------------------------
# MicroTeX - LaTeX math renderer with a Qt/QPainter backend. Built from the
# pinned source tree as the plain-CMake target `LaTeX` (project name LaTeX);
# the -DQT=ON path compiles src/platform/qt/graphic_qt.cpp, giving the
# `tex::Graphics2D_qt` class the MathImageProvider paints through.
# ---------------------------------------------------------------------------
_daemon_app_resolve_dir(_microtex_dir MICROTEX_SOURCE_DIR)
if(NOT EXISTS "${_microtex_dir}/CMakeLists.txt")
    message(FATAL_ERROR "MICROTEX_SOURCE_DIR must point to a MicroTeX source tree (got '${_microtex_dir}')")
endif()

# Build the Qt backend (graphic_qt.cpp -> tex::Graphics2D_qt). These options are
# read with if() BEFORE option() runs in MicroTeX's CMakeLists, so a FORCEd cache
# value is what its configure sees. Silence the library's logging and disable the
# debug box overlays so rendered formulas are clean.
set(QT ON CACHE BOOL "" FORCE)
set(HAVE_LOG OFF CACHE BOOL "" FORCE)
set(GRAPHICS_DEBUG OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)

# EXCLUDE_FROM_ALL keeps the bundled LaTeXQtSample demo target out of the default
# build: nothing links it, so it is configured but never compiled. Only the
# `LaTeX` library is linked by the BlockEditor module.
# MicroTeX's `add_library(LaTeX "")` follows BUILD_SHARED_LIBS, which is ON in
# this configure and produced a libLaTeX.so the packaged app cannot find at
# runtime. Force a static LaTeX lib (like be_core/be_diagram/ksyntax) so it links
# into the executable; shadow the variable only for this subtree, then restore.
set(_da_prev_shared "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF)
add_subdirectory("${_microtex_dir}" "${CMAKE_BINARY_DIR}/_deps/microtex" EXCLUDE_FROM_ALL)
set(BUILD_SHARED_LIBS "${_da_prev_shared}")

# Treat MicroTeX's public headers as system includes so its (vendored, upstream)
# header warnings do not surface in our translation units that include latex.h.
if(TARGET LaTeX)
    set_target_properties(LaTeX PROPERTIES
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_microtex_dir}/src")
endif()

# Resources (fonts + XML) MicroTeX's tex::LaTeX::init() reads at runtime. The
# read-only Nix store path is fine; init only reads from it.
set(MICROTEX_RES_DIR "${_microtex_dir}/res" CACHE INTERNAL "MicroTeX runtime resource dir")

# ---------------------------------------------------------------------------
# Desktop-only dependencies
#
# Opt-in: nothing links these yet (tray/updater/autostart land with the platform
# layer), and some are legacy qmake-era trees that need their CMake integration
# finished. Off by default so the renderer build (md4qt + earcut above) does not
# pull them in. Flip DAEMON_APP_DESKTOP_DEPS=ON when wiring src/platform.
# ---------------------------------------------------------------------------
option(DAEMON_APP_DESKTOP_DEPS "Build the desktop-only third-party deps (QWindowKit, updater, autostart, shortcut)" OFF)
# Hard-off on mobile AND in the browser: native window chrome, updater,
# autostart and global shortcuts are all desktop-OS integrations.
if(NOT DAEMON_APP_MOBILE AND NOT DAEMON_APP_WASM AND DAEMON_APP_DESKTOP_DEPS)
    # QWindowKit - frameless window / native chrome (Quick module only)
    _daemon_app_resolve_dir(_qwk_dir QWINDOWKIT_SOURCE_DIR)
    if(NOT EXISTS "${_qwk_dir}/CMakeLists.txt")
        message(FATAL_ERROR "QWINDOWKIT_SOURCE_DIR must point to a QWindowKit source tree (got '${_qwk_dir}')")
    endif()
    set(QWINDOWKIT_BUILD_STATIC ON CACHE BOOL "" FORCE)
    set(QWINDOWKIT_BUILD_WIDGETS OFF CACHE BOOL "" FORCE)
    set(QWINDOWKIT_BUILD_QUICK ON CACHE BOOL "" FORCE)
    set(QWINDOWKIT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(QWINDOWKIT_BUILD_DOCUMENTATIONS OFF CACHE BOOL "" FORCE)
    set(QWINDOWKIT_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory("${_qwk_dir}" "${CMAKE_BINARY_DIR}/_deps/qwindowkit" EXCLUDE_FROM_ALL)

    # Best-effort wiring for the qmake-era libraries. Each provides its own
    # target when a CMake build is present; otherwise we emit a placeholder
    # INTERFACE target so platform/desktop still configures, and the concrete
    # integration is finished in the platform layer.
    function(_daemon_app_optional_dep name dir_var target)
        _daemon_app_resolve_dir(_dir ${dir_var})
        if(EXISTS "${_dir}/CMakeLists.txt")
            add_subdirectory("${_dir}" "${CMAKE_BINARY_DIR}/_deps/${name}" EXCLUDE_FROM_ALL)
        else()
            message(WARNING
                "${dir_var} has no CMakeLists.txt ('${_dir}'); creating a placeholder "
                "INTERFACE target '${target}'. Finish the CMake integration in src/platform.")
            if(NOT TARGET ${target})
                add_library(${target} INTERFACE)
            endif()
        endif()
    endfunction()

    _daemon_app_optional_dep(qsimpleupdater    QSIMPLEUPDATER_SOURCE_DIR    QSimpleUpdater)
    _daemon_app_optional_dep(qautostart        QAUTOSTART_SOURCE_DIR        QAutostart)
    _daemon_app_optional_dep(qxtglobalshortcut QXTGLOBALSHORTCUT_SOURCE_DIR QxtGlobalShortcut)
endif()
