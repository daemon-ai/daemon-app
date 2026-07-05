# SPDX-License-Identifier: MPL-2.0
# SPDX-FileCopyrightText: 2026 Jarrad Hope
#
# CPack per-generator configuration (CPACK_PROJECT_CONFIG_FILE): cpack loads
# this once per generator with CPACK_GENERATOR set, before staging the
# install tree. Generator-specific knobs belong here; everything shared sits
# in cmake/Packaging.cmake.
#
# Dependency policy (DEB Depends / RPM Requires): hand-written system floor.
# dpkg-shlibdeps / rpm's AutoReq cannot run on NixOS (no dpkg/rpm database),
# and the packaged payload resolves everything else statically. The floor
# below is exactly the static binary's DT_NEEDED set (the allowlist
# checks.portable-boot-smoke and checks.artifact-sanity pin - nix/portable.nix
# documents each entry's rationale):
#
#   NEEDED evidence                     -> Debian            / Fedora
#   libc/libm/libdl/librt/libresolv     -> libc6             / glibc
#   libgcc_s.so.1                       -> libgcc-s1         / libgcc
#   libGLX/libOpenGL/libEGL (libglvnd)  -> libgl1 libglx0 libopengl0 libegl1
#                                          / libglvnd-glx libglvnd-opengl libglvnd-egl
#   libX11 / libX11-xcb                 -> libx11-6 libx11-xcb1 / libX11 libX11-xcb
#   libxcb.so.1                         -> libxcb1           / libxcb
#   libxcb-{randr,render,shape,shm,     -> libxcb-randr0 libxcb-render0
#            sync,xfixes,xkb,glx}           libxcb-shape0 libxcb-shm0
#                                           libxcb-sync1 libxcb-xfixes0
#                                           libxcb-xkb1 libxcb-glx0
#                                          / libxcb (one package on Fedora)
#   libfontconfig/libfreetype           -> libfontconfig1 libfreetype6
#                                          / fontconfig freetype
#
# The xcb-util family (cursor/icccm/image/keysyms/render-util/util),
# xkbcommon(+x11), and the wayland client libs (client/cursor/egl) are
# STATICALLY LINKED into daemon-app (nix/portable.nix staticLeaves) and are
# deliberately absent: they are exactly the sonames the AppImage community
# excludelist does not guarantee on a base system (libxcb-cursor0 is not
# even packaged everywhere), so depending on them would block installs the
# binary no longer requires. The libxcb-* entries that REMAIN ship with the
# core libxcb package on every distro.
#
# libstdc++/libgcc are linked statically into daemon-app; libstdc++6 stays a
# Depends only as the floor for optional co-packaged node binaries (the
# superproject additionally ships its own libstdc++/libgomp copies in lib/
# for exactly that payload). dbus and openssl are runtime-dlopen'd and
# degrade gracefully, so they are NOT dependencies. Version floors are
# intentionally omitted; scripts/glibc-floor.sh gates the glibc
# symbol-version ceiling instead.

if(CPACK_GENERATOR STREQUAL "DEB")
    # Self-contained product tree under /opt (Filesystem Hierarchy Standard
    # for add-on packages); postinst symlinks the entry points into
    # /usr/local/bin and the desktop assets into /usr/share.
    set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/daemon")
    set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
    # No dpkg database on NixOS: Depends are hand-written above (the static
    # payload's exact DT_NEEDED floor) instead of dpkg-shlibdeps output.
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "libc6, libstdc++6, libgcc-s1, libgl1, libglx0, libopengl0, libegl1, libx11-6, libx11-xcb1, libxcb1, libxcb-randr0, libxcb-render0, libxcb-shape0, libxcb-shm0, libxcb-sync1, libxcb-xfixes0, libxcb-xkb1, libxcb-glx0, libfontconfig1, libfreetype6"
    )
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_COMPRESSION_TYPE "xz")
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
        "${CPACK_DAEMON_APP_LINUX_DIR}/postinst;${CPACK_DAEMON_APP_LINUX_DIR}/prerm"
    )
    # Keep the committed scripts' 0755 mode instead of CPack's 0644 rewrite.
    set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
endif()

if(CPACK_GENERATOR STREQUAL "RPM")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/daemon")
    set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")
    # Mirror of SHLIBDEPS OFF: rpmbuild's find-requires would list the Nix
    # store sonames; Requires are hand-written above.
    set(CPACK_RPM_PACKAGE_AUTOREQPROV OFF)
    set(CPACK_RPM_PACKAGE_REQUIRES
        "glibc, libstdc++, libgcc, libglvnd-glx, libglvnd-opengl, libglvnd-egl, libX11, libX11-xcb, libxcb, fontconfig, freetype"
    )
    set(CPACK_RPM_PACKAGE_LICENSE "MPL-2.0")
    set(CPACK_RPM_PACKAGE_GROUP "Development/Tools")
    set(CPACK_RPM_PACKAGE_URL "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE
        "${CPACK_DAEMON_APP_LINUX_DIR}/postinst"
    )
    set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE
        "${CPACK_DAEMON_APP_LINUX_DIR}/prerm"
    )
    # The staged binaries carry $ORIGIN rpaths (the portable payload
    # rewrite); rpmbuild's check-rpaths policy script rejects any rpath it
    # does not recognize, so silence it. _tmppath/_dbpath default to /var/tmp
    # and /var/lib/rpm, which do not exist in the Nix build sandbox - point
    # both into rpmbuild's own working tree.
    set(CPACK_RPM_SPEC_MORE_DEFINE
        "%define __brp_check_rpaths %{nil}
%define _tmppath %{_topdir}/tmp
%define _dbpath %{_topdir}/rpmdb"
    )
endif()

if(CPACK_GENERATOR STREQUAL "AppImage")
    # Canonical AppDir layout: usr/bin, usr/share/... under the mount point.
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
    # The generator requires CPACK_PACKAGE_ICON to name an installed icon file
    # matching the desktop entry's Icon key (basename lookup).
    set(CPACK_PACKAGE_ICON "daemon-app.png")
    set(CPACK_APPIMAGE_DESKTOP_FILE "daemon-app.desktop")
    # Pinned type2 runtime from nix/appimagetool.nix - no network fetch at
    # package time. Empty means the appimagetool wrapper injects its own
    # pinned runtime.
    if(CPACK_DAEMON_APP_APPIMAGE_RUNTIME)
        set(CPACK_APPIMAGE_RUNTIME_FILE "${CPACK_DAEMON_APP_APPIMAGE_RUNTIME}")
    endif()
    # AppImage delta-update transport: the gh-releases-zsync form points
    # AppImageUpdate at the latest GitHub release's .zsync control file (the
    # release pipeline publishes daemon-*x86_64.AppImage.zsync next to the
    # artifact). Orthogonal to the in-app minisign-verified updater.
    set(CPACK_APPIMAGE_UPDATE_INFORMATION
        "gh-releases-zsync|daemon-ai|daemon|latest|daemon-*x86_64.AppImage.zsync"
    )
    # Metainfo is validated by checks.artifact-sanity (appstreamcli); the
    # sandbox running appimagetool has no appstream, so skip its own check.
    set(CPACK_APPIMAGE_NO_APPSTREAM ON)
endif()

if(CPACK_GENERATOR STREQUAL "DragNDrop")
    # macOS drag-and-drop .dmg: the staging root carries daemon-app.app (the
    # BUNDLE DESTINATION "." install) and the generator adds the /Applications
    # symlink itself. CODE-ONLY so far - this branch has never run on a mac;
    # see packaging/macos/README.md for the validation debt.
    set(CPACK_DMG_VOLUME_NAME "Daemon")
    # UDZO (zlib-compressed, read-only) mounts everywhere and is the
    # compatibility default; switch to UDBZ (bzip2) only if size ever matters
    # more than mount speed on older macOS.
    set(CPACK_DMG_FORMAT "UDZO")
    # Custom volume icon for the mounted image (hdiutil attaches it via the
    # generator's SetFile/Rez plumbing).
    set(CPACK_PACKAGE_ICON "${CPACK_DAEMON_APP_MACOS_DIR}/daemon-app.icns")
    # Apple deprecated DMG software license agreements, and an SLA-wrapped
    # image breaks `xcrun stapler staple` on the .dmg - keep the license
    # inside the bundle (Contents/Resources, staged by Packaging.cmake)
    # instead of attaching CPACK_RESOURCE_FILE_LICENSE to the image.
    set(CPACK_DMG_SLA_USE_RESOURCE_FILE_LICENSE OFF)
    # Finder layout (icon grid positions, window size, backdrop) needs a
    # committed .DS_Store setup script + background raster, both of which can
    # only be authored on a mac (Finder/AppleScript). Neither asset exists
    # yet - packaging/macos/dmg-background.png is intentionally NOT committed;
    # uncomment when they land:
    # set(CPACK_DMG_BACKGROUND_IMAGE
    #     "${CPACK_DAEMON_APP_MACOS_DIR}/dmg-background.png"
    # )
    # set(CPACK_DMG_DS_STORE_SETUP_SCRIPT
    #     "${CPACK_DAEMON_APP_MACOS_DIR}/dmg-ds-store-setup.scpt"
    # )
endif()

if(CPACK_GENERATOR STREQUAL "NSIS")
    # PER-USER product tree: $LOCALAPPDATA\Programs\Daemon (the VS Code / Chrome
    # "user setup" model). bin\ holds daemon-app.exe (+ the bundled daemon
    # binaries once the superproject injects them, same DAEMON_APP_BUNDLED_*
    # contract as the Linux artifacts).
    #
    # WHY per-user (decision for todo u3-windows, packaging/UPDATES.md SelfApply):
    # the auto-updater's SelfApply tier runs this installer silently (/S) from
    # the detached daemon-updater helper AFTER the app exits. A per-machine
    # ($PROGRAMFILES64) install requires elevation, so every unattended update
    # would raise a UAC prompt on a process the user never launched - which
    # defeats promptless self-apply and can fail outright for standard users.
    # Per-user installs to a user-writable root with RequestExecutionLevel user
    # (no elevation ever). No install base has shipped, so there is no
    # per-machine -> per-user migration to perform.
    #
    # The elevation level (RequestExecutionLevel) and the shell context
    # (SetShellVarContext, which routes the uninstall registry hive to HKCU and
    # the Start Menu to the per-user profile) are hardcoded to per-machine in
    # CMake's stock NSIS.template.in with NO CPack variable to override them, so
    # cmake/Packaging.cmake emits a minimally-patched template onto
    # CPACK_MODULE_PATH. $LOCALAPPDATA is a user-scoped NSIS constant and is not
    # architecture-split, so the 32-bit stub / $PROGRAMFILES-vs-64 concern that
    # forced $PROGRAMFILES64 for a per-machine install does not apply here.
    set(CPACK_NSIS_INSTALL_ROOT "$LOCALAPPDATA\\Programs")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Daemon")
    set(CPACK_NSIS_PACKAGE_NAME "Daemon")
    set(CPACK_NSIS_DISPLAY_NAME "Daemon")

    # Branding: installer/uninstaller icons + the Add/Remove Programs entry
    # (icon path is relative to the installed tree, backslash-separated).
    set(CPACK_NSIS_MUI_ICON "${CPACK_DAEMON_APP_WINDOWS_DIR}/daemon-app.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CPACK_DAEMON_APP_WINDOWS_DIR}/daemon-app.ico")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\daemon-app.exe")
    set(CPACK_NSIS_URL_INFO_ABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_NSIS_CONTACT "${CPACK_PACKAGE_CONTACT}")
    set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
    set(CPACK_NSIS_COMPRESSOR "/SOLID lzma")

    # License page: LICENSE + THIRD-PARTY-NOTICES concatenated at configure
    # time (cmake/Packaging.cmake); fall back to the bare LICENSE for a
    # stray `cpack -G NSIS` against an older configure.
    if(CPACK_DAEMON_APP_NSIS_LICENSE)
        set(CPACK_RESOURCE_FILE_LICENSE "${CPACK_DAEMON_APP_NSIS_LICENSE}")
    endif()

    # Shortcuts: Start Menu entry always; the Desktop icon rides the
    # installer's "Create Daemon Desktop Icon" checkbox (Install Options
    # page, off by default). The generated uninstaller removes both, plus
    # the install tree and the registry entries.
    set(CPACK_PACKAGE_EXECUTABLES "daemon-app" "Daemon")
    set(CPACK_CREATE_DESKTOP_LINKS "daemon-app")
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY "bin")

    # Upgrades: silently run the previous uninstaller first, so stale
    # payload never survives a version bump (NSIS has no component-level
    # upgrade tracking of its own).
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

    # Finish page offers to launch the app (path resolves under
    # CPACK_NSIS_EXECUTABLES_DIRECTORY).
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "daemon-app.exe")
endif()
