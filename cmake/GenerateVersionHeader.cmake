# SPDX-License-Identifier: MPL-2.0
# SPDX-FileCopyrightText: 2026 Jarrad Hope
#
# Run in script mode (`cmake -P`) to (re)generate the version header. Invoked at BUILD time by the
# `daemon_app_version_header` target so the off-tag distance / short hash / dirty marker stay
# current (phosphor's build-time `git describe`). Inputs passed via -D:
#   PROJECT_VERSION       the SemVer base (from the VERSION file)
#   INJECTED_VERSION_STR  a ready value from the build system (Nix); when non-empty it wins
#   SOURCE_DIR            the repo root, for `git describe`
#   TEMPLATE              path to daemon_app_version.h.in
#   OUTPUT                path to the generated header

if(NOT "${INJECTED_VERSION_STR}" STREQUAL "")
    set(DAEMON_APP_VERSION_STR "${INJECTED_VERSION_STR}")
else()
    set(DAEMON_APP_VERSION_STR "${PROJECT_VERSION}")
    find_package(Git QUIET)
    if(Git_FOUND)
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" describe --tags --match "v[0-9]*" --dirty --always
            WORKING_DIRECTORY "${SOURCE_DIR}"
            OUTPUT_VARIABLE _raw
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            RESULT_VARIABLE _rc
        )
        if(_rc EQUAL 0 AND NOT _raw STREQUAL "")
            # Map `git describe` to a SemVer build-metadata suffix (phosphor grammar).
            set(_dirty "")
            if(_raw MATCHES "-dirty$")
                set(_dirty ".dirty")
                string(REGEX REPLACE "-dirty$" "" _raw "${_raw}")
            endif()
            string(REGEX REPLACE "^v" "" _raw "${_raw}")
            if(_raw MATCHES "^(.+)-([0-9]+)-g([0-9a-f]+)$")
                # N commits past the most recent tag.
                set(DAEMON_APP_VERSION_STR "${PROJECT_VERSION}+${CMAKE_MATCH_2}.g${CMAKE_MATCH_3}${_dirty}")
            elseif(_raw MATCHES "^[0-9a-f]+$")
                # `--always` with no reachable tag: a bare abbreviated hash.
                set(DAEMON_APP_VERSION_STR "${PROJECT_VERSION}+g${_raw}${_dirty}")
            elseif(NOT _dirty STREQUAL "")
                # Exact tag, dirty tree.
                set(DAEMON_APP_VERSION_STR "${PROJECT_VERSION}+dirty")
            endif()
            # Exact clean tag: keep the bare PROJECT_VERSION set above.
        endif()
    endif()
endif()

# Write via temp + copy-if-different so an unchanged version doesn't force a recompile of consumers.
configure_file("${TEMPLATE}" "${OUTPUT}.tmp" @ONLY)
file(COPY_FILE "${OUTPUT}.tmp" "${OUTPUT}" ONLY_IF_DIFFERENT)
file(REMOVE "${OUTPUT}.tmp")
