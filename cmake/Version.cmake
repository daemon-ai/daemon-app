# SPDX-License-Identifier: MPL-2.0
# SPDX-FileCopyrightText: 2026 Jarrad Hope
#
# Embed a git-enriched SemVer build string into a generated header (cmake/daemon_app_version.h.in).
# Source of truth: PROJECT_VERSION (from the top-level VERSION file). The Nix flake injects the
# reproducible value via -DDAEMON_APP_VERSION_STR=...; otherwise the build-metadata suffix is
# derived from `git describe` at BUILD time so the dirty / off-tag marker stays current.
#
# Consumers link the INTERFACE target `da_version` (for the generated include dir) and depend on the
# `daemon_app_version_header` target (for build ordering / refresh).

set(_dav_dir "${CMAKE_BINARY_DIR}/generated")
set(DAEMON_APP_VERSION_HEADER "${_dav_dir}/daemon_app_version.h")
set(_dav_template "${CMAKE_CURRENT_SOURCE_DIR}/cmake/daemon_app_version.h.in")
set(_dav_script "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateVersionHeader.cmake")
file(MAKE_DIRECTORY "${_dav_dir}")

if(DEFINED DAEMON_APP_VERSION_STR)
    set(_dav_injected "${DAEMON_APP_VERSION_STR}")
else()
    set(_dav_injected "")
endif()

set(_dav_args
    -DPROJECT_VERSION=${PROJECT_VERSION}
    -DINJECTED_VERSION_STR=${_dav_injected}
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DTEMPLATE=${_dav_template}
    -DOUTPUT=${DAEMON_APP_VERSION_HEADER}
    -P ${_dav_script}
)

# Generate once at configure time so the header exists for the first compile / IDE indexing.
execute_process(COMMAND ${CMAKE_COMMAND} ${_dav_args})

# Always-out-of-date target that refreshes the header at build time (no-op write when unchanged).
add_custom_target(daemon_app_version_header
    BYPRODUCTS "${DAEMON_APP_VERSION_HEADER}"
    COMMAND ${CMAKE_COMMAND} ${_dav_args}
    COMMENT "Updating daemon_app_version.h"
    VERBATIM
)

add_library(da_version INTERFACE)
add_library(daemon-app::version ALIAS da_version)
target_include_directories(da_version INTERFACE "${_dav_dir}")
