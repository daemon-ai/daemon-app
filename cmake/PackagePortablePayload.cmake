# SPDX-License-Identifier: MPL-2.0
# SPDX-FileCopyrightText: 2026 Jarrad Hope
#
# CPack pre-build script (CPACK_PRE_BUILD_SCRIPTS): runs after cpack stages
# the install tree, before the generator wraps it. Only wired up by the
# flake's packaging outputs (DAEMON_APP_PACKAGE_PORTABLE=ON over the static-Qt
# configure), never by plain developer configures. Two jobs:
#
# 1. PRUNE the vendored KSyntaxHighlighting side-payload. The static app
#    compiles the engine, the QML plugin, and the syntax definitions in; the
#    vendored subproject's install rules still stage its CLI tool, headers,
#    CMake packages, static archives, QML module scaffolding, translations,
#    and logging categories - dev cruft for a package whose binary needs none
#    of it.
#
# 2. REWRITE the staged ELF payload for a generic distro root, mirroring what
#    nix/portable.nix does to the portable tarball layout:
#      * bin/*: interpreter -> /lib64/ld-linux-x86-64.so.2 (the generic
#        loader, not a Nix store path), rpath -> $ORIGIN:$ORIGIN/../lib (so
#        shipped sibling libraries under lib/ resolve first). This covers the
#        static-Qt daemon-app AND the injected sibling binaries (daemon,
#        daemon-infer, daemon-cli - the DAEMON_APP_BUNDLED_* contract), which
#        are Nix-linked ELFs until this step.
#      * lib/*.so*: rpath -> $ORIGIN (the DAEMON_APP_BUNDLED_LIBS runtime
#        libraries, e.g. the libstdc++/libgomp pair daemon-infer needs).
#      * bin/daemon-app additionally gets its dead store-path string
#        constants scrubbed (remove-references-to): QLibraryInfo bakes the
#        static Qt prefixes into libQt6Core.a; nothing resolves through them
#        at runtime (plugins + QML are compiled in), but scrubbing keeps the
#        shipped bytes free of build-machine paths, matching the portable
#        tarball payload. Prefix list: DAEMON_APP_SCRUB_PREFIXES ->
#        CPACK_DAEMON_APP_SCRUB_PREFIXES.
#
# Tools (patchelf, remove-references-to) come from the artifact derivation's
# nativeBuildInputs.

if(NOT CPACK_TEMPORARY_INSTALL_DIRECTORY)
    set(CPACK_TEMPORARY_INSTALL_DIRECTORY "${CPACK_TEMPORARY_DIRECTORY}")
endif()
message(
    STATUS
    "portable payload rewrite under: ${CPACK_TEMPORARY_INSTALL_DIRECTORY}"
)

# The staging prefix depth differs per generator (DEB/RPM: opt/daemon;
# AppImage: usr), so anchor on the one invariant - the directory holding
# bin/daemon-app is the payload root.
file(
    GLOB_RECURSE _pp_candidates
    LIST_DIRECTORIES false
    "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/*daemon-app"
)
set(_pp_app "")
foreach(_pp_candidate IN LISTS _pp_candidates)
    if(_pp_candidate MATCHES "/bin/daemon-app$")
        list(APPEND _pp_app "${_pp_candidate}")
    endif()
endforeach()
list(LENGTH _pp_app _pp_app_count)
if(NOT _pp_app_count EQUAL 1)
    message(
        FATAL_ERROR
        "expected exactly one staged bin/daemon-app, found ${_pp_app_count}: ${_pp_app}"
    )
endif()
get_filename_component(_pp_bindir "${_pp_app}" DIRECTORY)
get_filename_component(_pp_root "${_pp_bindir}" DIRECTORY)
message(STATUS "portable payload root: ${_pp_root}")

# --- 1. prune the vendored KSyntaxHighlighting side-payload -------------------
foreach(
    _pp_cruft
    bin/ksyntaxhighlighter6
    include
    lib/cmake
    lib/qml
    share/locale
    share/qlogging-categories6
)
    if(EXISTS "${_pp_root}/${_pp_cruft}")
        file(REMOVE_RECURSE "${_pp_root}/${_pp_cruft}")
        message(STATUS "portable payload: pruned ${_pp_cruft}")
    endif()
endforeach()
file(GLOB _pp_archives "${_pp_root}/lib/*.a")
foreach(_pp_hit IN LISTS _pp_archives)
    file(REMOVE "${_pp_hit}")
    message(STATUS "portable payload: pruned ${_pp_hit}")
endforeach()

# --- 2. patchelf + scrub -------------------------------------------------------
file(GLOB _pp_bins "${_pp_root}/bin/*")
foreach(_pp_file IN LISTS _pp_bins)
    execute_process(
        COMMAND
            patchelf --set-interpreter /lib64/ld-linux-x86-64.so.2 --set-rpath
            "$ORIGIN:$ORIGIN/../lib" "${_pp_file}"
        RESULT_VARIABLE _pp_rc
        ERROR_VARIABLE _pp_err
    )
    if(NOT _pp_rc EQUAL 0)
        message(
            FATAL_ERROR
            "patchelf failed on staged executable ${_pp_file}: ${_pp_err}"
        )
    endif()
    message(STATUS "portable payload: patched ${_pp_file}")
endforeach()

file(GLOB _pp_libs "${_pp_root}/lib/*.so*")
foreach(_pp_file IN LISTS _pp_libs)
    if(IS_SYMLINK "${_pp_file}")
        continue()
    endif()
    execute_process(
        COMMAND patchelf --set-rpath "$ORIGIN" "${_pp_file}"
        RESULT_VARIABLE _pp_rc
        ERROR_VARIABLE _pp_err
    )
    if(NOT _pp_rc EQUAL 0)
        message(
            FATAL_ERROR
            "patchelf failed on staged library ${_pp_file}: ${_pp_err}"
        )
    endif()
    message(STATUS "portable payload: patched ${_pp_file}")
endforeach()

if(CPACK_DAEMON_APP_SCRUB_PREFIXES)
    foreach(_pp_prefix IN LISTS CPACK_DAEMON_APP_SCRUB_PREFIXES)
        execute_process(
            COMMAND remove-references-to -t "${_pp_prefix}" "${_pp_app}"
            RESULT_VARIABLE _pp_rc
            ERROR_VARIABLE _pp_err
        )
        if(NOT _pp_rc EQUAL 0)
            message(
                FATAL_ERROR
                "remove-references-to -t ${_pp_prefix} failed on ${_pp_app}: ${_pp_err}"
            )
        endif()
    endforeach()
    message(STATUS "portable payload: scrubbed store prefixes from ${_pp_app}")
endif()
