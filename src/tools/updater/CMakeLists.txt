# daemon-updater: a tiny standalone (no-Qt) self-update apply helper. The app
# stages + verifies a new artifact, spawns this binary with a spec, and quits;
# the helper waits for the app PID to exit, applies the update atomically, and
# relaunches. Design: packaging/UPDATES.md. Frozen CLI contract: see updater.cpp.
#
# Builds for every DESKTOP target (native Linux dev + static portable, MinGW
# cross, macOS). Excluded from wasm/Android/iOS by the guard on the parent
# add_subdirectory (a headless mobile/browser artifact never self-applies).
#
# The install rule below stages daemon-updater into ${CMAKE_INSTALL_BINDIR}, so
# every CPack payload (AppImage usr/bin, NSIS, macOS .app Contents/MacOS via the
# packaging install tree) picks it up automatically alongside the app binary.

include(GNUInstallDirs)

# --- vendored SHA-256 (public domain; Brad Conte) --------------------------
# Built as plain C and kept free of the project -Werror set, exactly like the
# vendored zcbor codec. Exposed to the C++ engine as a SYSTEM include so the
# clang-tidy gate does not analyze third-party headers. Lint/format/typos
# exclusions live in .clang-format-ignore, typos.toml and REUSE.toml.
enable_language(C)
add_library(da_updater_sha256 STATIC vendor/sha256.c)
set_target_properties(
    da_updater_sha256
    PROPERTIES
        C_STANDARD 11
        POSITION_INDEPENDENT_CODE ON
        AUTOMOC OFF
        AUTOUIC OFF
        AUTORCC OFF
)
target_include_directories(
    da_updater_sha256
    SYSTEM
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor"
)

# --- the helper binary ------------------------------------------------------
add_executable(
    daemon-updater
    main.cpp
    updater.cpp
    updater.h
    digest.cpp
    digest.h
)
# No Qt: keep the Qt autogen machinery off this plain-C++ tool.
set_target_properties(
    daemon-updater
    PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF
)
# Plain C++17 by contract (no Qt, no exceptions across the boundary). The tree's
# global standard is C++20; requesting cxx_std_17 only sets the floor.
target_compile_features(daemon-updater PRIVATE cxx_std_17)
target_link_libraries(
    daemon-updater
    PRIVATE da_updater_sha256 daemon-app::warnings
)

# macOS: the shippable product is the .app, so the helper must live INSIDE it
# next to daemon-app (Contents/MacOS) — a sibling in <dmg>/bin would be dropped
# when the user drags only the bundle to /Applications. This mirrors the
# co-located node binaries in cmake/Packaging.cmake. Elsewhere, GNUInstallDirs.
if(APPLE)
    install(TARGETS daemon-updater RUNTIME DESTINATION "daemon-app.app/Contents/MacOS")
else()
    install(TARGETS daemon-updater RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# --- native integration tests (Linux) --------------------------------------
# Driven by a shell script against the built binary: rename-swap (happy + cross
# filesystem), two-move (happy + induced-failure rollback), sha256 mismatch,
# wait-pid (wait + timeout), and detached relaunch. Registered only for a native
# desktop build with testing on; the cross/mobile builds set BUILD_TESTING=OFF.
if(BUILD_TESTING AND NOT WIN32 AND NOT DAEMON_APP_MOBILE)
    add_test(
        NAME tst_updater
        COMMAND
            bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.sh"
            "$<TARGET_FILE:daemon-updater>"
    )
endif()
