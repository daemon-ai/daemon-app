# SPDX-License-Identifier: MPL-2.0
# SPDX-FileCopyrightText: 2026 Jarrad Hope
#
# Desktop packaging: freedesktop assets, bundled sibling binaries, and CPack
# (DEB / RPM / AppImage; NSIS is a stub for the Windows workstream).
#
# CAVEAT (v1 scaffolding): the payload packaged today is the DYNAMIC-Qt app as
# linked by the Nix toolchain - its interpreter and RUNPATH point into
# /nix/store, so the artifacts only run on hosts with the matching store paths
# (build sandbox, NixOS). The sibling static-qt workstream swaps in a portable
# payload later; what must be correct here is the packaging skeleton: assets,
# install rules, per-generator config (cmake/CPackPerGen.cmake), and the
# artifact sanity checks (flake checks.artifact-sanity).

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

# Desktop-only: the wasm bundle installs its own artifact set and the mobile
# platforms package through their native toolchains.
if(DAEMON_APP_WASM OR DAEMON_APP_MOBILE)
    return()
endif()

include(GNUInstallDirs)

if(DAEMON_APP_BUNDLED_DAEMON)
    install(
        PROGRAMS "${DAEMON_APP_BUNDLED_DAEMON}"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        RENAME daemon
    )
endif()
if(DAEMON_APP_BUNDLED_DAEMON_INFER)
    install(
        PROGRAMS "${DAEMON_APP_BUNDLED_DAEMON_INFER}"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        RENAME daemon-infer
    )
endif()
if(DAEMON_APP_BUNDLED_DAEMON_CLI)
    install(
        PROGRAMS "${DAEMON_APP_BUNDLED_DAEMON_CLI}"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        RENAME daemon-cli
    )
endif()

# License + third-party notices ship with every package. EXISTS-guarded: the
# notices file lands on a sibling branch and must not break a lean checkout.
foreach(_da_doc LICENSE THIRD-PARTY-NOTICES.md)
    if(EXISTS "${CMAKE_SOURCE_DIR}/${_da_doc}")
        install(
            FILES "${CMAKE_SOURCE_DIR}/${_da_doc}"
            DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/daemon-app
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
set(CPACK_PACKAGE_FILE_NAME
    "daemon-${PROJECT_VERSION}-linux-${CMAKE_SYSTEM_PROCESSOR}"
)
set(CPACK_PACKAGE_CHECKSUM SHA256)
# The Nix build tree is unstripped; strip at cpack-install so artifacts do not
# carry debug info.
set(CPACK_STRIP_FILES TRUE)
# Default generators for a bare `cpack`; the flake package outputs pass -G.
# AppImage additionally needs CMake >= 4.2 + appimagetool (nix/cmake-appimage.nix
# + nix/appimagetool.nix provide both).
set(CPACK_GENERATOR "DEB;RPM")

# dpkg is absent at build time (Nix sandbox), so pin the Debian architecture
# name instead of letting the DEB generator shell out for it.
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
else()
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
endif()

# Stash source-tree locations + the nix-injected AppImage runtime for the
# per-generator config file (only CPACK_-prefixed variables survive into the
# serialized CPackConfig cpack reads).
set(CPACK_DAEMON_APP_LINUX_DIR "${_da_linux_dir}")
set(CPACK_DAEMON_APP_WINDOWS_DIR "${CMAKE_SOURCE_DIR}/packaging/windows")
set(DAEMON_APP_APPIMAGE_RUNTIME
    ""
    CACHE FILEPATH
    "Prebuilt AppImage type2 runtime for cpack -G AppImage (injected by the flake)"
)
set(CPACK_DAEMON_APP_APPIMAGE_RUNTIME "${DAEMON_APP_APPIMAGE_RUNTIME}")

set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_SOURCE_DIR}/cmake/CPackPerGen.cmake")

include(CPack)
