cmake_minimum_required(VERSION 3.21)

# Single source of truth for the SemVer base: the top-level VERSION file (no leading "v"). The
# git-enriched build string is derived in cmake/Version.cmake (Nix passes DAEMON_APP_VERSION_STR;
# dev builds use `git describe`).
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" DAEMON_APP_BASE_VERSION LIMIT_COUNT 1)
string(STRIP "${DAEMON_APP_BASE_VERSION}" DAEMON_APP_BASE_VERSION)

project(daemon-app
    VERSION ${DAEMON_APP_BASE_VERSION}
    DESCRIPTION "AI agent chat application (Qt 6 QML/C++)"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Emit compile_commands.json so clang-tidy / cppcheck / run-clang-tidy can analyze the tree
# (consumed by the justfile `lint-cpp` and `audit-cleanup` recipes).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Generate a .qmlls.ini so qmlls/qmllint can resolve the project's QML modules.
set(QT_QML_GENERATE_QMLLS_INI ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Qt for WebAssembly (browser) build. EMSCRIPTEN is set by emscripten's own
# toolchain file, which Qt's wasm qt.toolchain.cmake chain-loads (see the
# wasm-release preset). Like DAEMON_APP_MOBILE this is derived, not an option:
# a desktop configure must never flip it. Source-level gating uses Q_OS_WASM;
# DAEMON_APP_WASM is the CMake-side mirror for excluding whole files/deps.
if(EMSCRIPTEN)
    set(DAEMON_APP_WASM ON)
else()
    set(DAEMON_APP_WASM OFF)
endif()

# Platform flags used to gate native-only code/deps further down the tree.
if(ANDROID OR IOS OR DAEMON_APP_WASM)
    set(DAEMON_APP_MOBILE ON)
else()
    set(DAEMON_APP_MOBILE OFF)
endif()

# Portable static-Qt desktop build (the flake's `portable` package): the GUI is
# linked against a statically built Qt, so everything that assumes a shared-Qt
# runtime drops out - the qmake-built QMLTermWidget shared plugin is replaced by
# the same TerminalPanelStub the browser uses, and the optional qtkeychain
# integration is skipped (the token store falls back to QSettings). Desktop
# only: the wasm build has its own (derived, not optional) gating above.
option(DAEMON_APP_STATIC "Portable static-Qt desktop build (terminal stub, QSettings token store)" OFF)
if(DAEMON_APP_WASM AND DAEMON_APP_STATIC)
    message(STATUS "daemon-app: DAEMON_APP_STATIC forced OFF for the wasm build (wasm gating applies)")
    set(DAEMON_APP_STATIC OFF)
endif()

# Qt Quick + Controls for the UI; QtWidgets on desktop for the tray.
# QuickControls2 is needed for QQuickStyle (pinning the app style in main).
# LinguistTools provides qt_add_translations (lupdate/lrelease + .qm embedding).
# WebSockets carries the daemon wire mux to browser (wasm) clients and is the
# natively-testable third transport backend on desktop.
find_package(Qt6 6.11 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    QmlModels
    Quick
    QuickControls2
    QuickDialogs2
    QuickDialogs2QuickImpl
    QuickLayouts
    Network
    Sql
    WebSockets
    LinguistTools
)
# QtWidgets exists only for the desktop tray (QSystemTrayIcon): mobile and the
# browser build run pure Qt Quick on QGuiApplication.
if(NOT DAEMON_APP_MOBILE AND NOT DAEMON_APP_WASM)
    find_package(Qt6 6.11 REQUIRED COMPONENTS Widgets)
endif()

qt_standard_project_setup(REQUIRES 6.11)

# Experimental Tui Widgets terminal frontend (daemon-tui). Off by default so the
# GUI build and CI are unaffected; the Nix flake's `tui` package flips it on and
# provides the Meson-built Tui Widgets stack via pkg-config. Core5Compat is a
# transitive requirement of the Qt6 build of Tui Widgets.
option(DAEMON_APP_TUI "Build the experimental Tui Widgets TUI frontend (daemon-tui)" OFF)
# Hard-off in the browser: the TUI drives a real terminal (PTY/termios), which
# cannot exist in the wasm sandbox. The normal variable shadows any cache value
# for the rest of the configure.
if(DAEMON_APP_WASM AND DAEMON_APP_TUI)
    message(STATUS "daemon-app: DAEMON_APP_TUI forced OFF for the wasm build")
    set(DAEMON_APP_TUI OFF)
endif()
# The static gate covers the GUI only: daemon-tui stays a dynamic build (its
# Meson-built Tui Widgets stack is shared-Qt), so the combination is refused
# rather than half-supported.
if(DAEMON_APP_STATIC AND DAEMON_APP_TUI)
    message(STATUS "daemon-app: DAEMON_APP_TUI forced OFF for the static build (GUI only)")
    set(DAEMON_APP_TUI OFF)
endif()
if(DAEMON_APP_TUI)
    find_package(Qt6 6.11 REQUIRED COMPONENTS Core5Compat)
endif()

include(CompilerWarnings)

# Third-party dependencies are vendored by the flake and consumed here
# (md4qt + earcut for the BlockEditor renderer; QWindowKit etc. on desktop).
include(Dependencies)

# Generate the git-enriched version header (daemon-app::version) consumed by the frontends' main().
include(Version)

add_subdirectory(src)

# --- Internationalization -------------------------------------------------
# A single translation catalog shared by both frontends: lupdate scans the
# whole src tree (QML qsTr + C++ tr), lrelease compiles each i18n/*.ts to a
# .qm, and qt_add_translations embeds them into the GUI (and TUI when built)
# under the ":/i18n" resource prefix. main() loads them via QTranslator.
#
# The .ts files are committed; refresh them with `cmake --build . --target
# update_translations` after changing strings. Add a locale by listing it in
# DAEMON_APP_LOCALES and re-running that target.
include(Translations)

# Tests. Building/running them is opt-in (BUILD_TESTING, default ON) so it can
# be turned off for lean release builds. Never built for the browser or mobile:
# the QTest binaries exercise desktop-only seams (QProcess launchers, local
# sockets), there is no ctest runtime in the wasm sandbox or on-device, and on
# Android Qt's target finalization would try to package every test executable
# as its own APK (the wasm-release preset also sets BUILD_TESTING=OFF; this
# guard keeps a preset-less configure consistent).
include(CTest)
if(BUILD_TESTING AND NOT DAEMON_APP_MOBILE)
    add_subdirectory(tests)
endif()

# Standalone command-line tools (no Qt). daemon-updater is the self-update apply
# helper (src/tools/updater), packaged into every desktop artifact. Added here
# (after include(CTest), like tests/) so its ctest registration is reachable; a
# headless browser/mobile artifact never self-applies, so it is desktop-only.
if(NOT DAEMON_APP_MOBILE)
    add_subdirectory(src/tools/updater)
endif()

# Desktop packaging: freedesktop assets (Linux) / bundle payload + Qt deploy
# (macOS), bundled sibling binaries, CPack (DEB / RPM / AppImage / DragNDrop).
# No-op for wasm/mobile configures.
include(Packaging)
