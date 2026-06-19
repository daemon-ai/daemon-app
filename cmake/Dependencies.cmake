# Wires the third-party sources that the Nix flake exposes as <DEP>_SOURCE_DIR
# cache variables into the build via add_subdirectory (built statically).
#
# No git submodules, no FetchContent: the flake pins and provides every source.

include_guard(GLOBAL)

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
# Desktop-only dependencies
# ---------------------------------------------------------------------------
if(NOT DAEMON_APP_MOBILE)
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
