cmake_minimum_required(VERSION 3.21)

project(daemon-app
    VERSION 0.1.0
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

# Platform flag used to gate desktop-only code/deps further down the tree.
if(ANDROID OR IOS)
    set(DAEMON_APP_MOBILE ON)
else()
    set(DAEMON_APP_MOBILE OFF)
endif()

# Qt Quick + Controls for the UI; QtWidgets on desktop for the tray.
# QuickControls2 is needed for QQuickStyle (pinning the app style in main).
# LinguistTools provides qt_add_translations (lupdate/lrelease + .qm embedding).
find_package(Qt6 6.11 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Network Sql LinguistTools)
if(NOT DAEMON_APP_MOBILE)
    find_package(Qt6 6.11 REQUIRED COMPONENTS Widgets)
endif()

qt_standard_project_setup(REQUIRES 6.11)

# Experimental Tui Widgets terminal frontend (daemon-tui). Off by default so the
# GUI build and CI are unaffected; the Nix flake's `tui` package flips it on and
# provides the Meson-built Tui Widgets stack via pkg-config. Core5Compat is a
# transitive requirement of the Qt6 build of Tui Widgets.
option(DAEMON_APP_TUI "Build the experimental Tui Widgets TUI frontend (daemon-tui)" OFF)
if(DAEMON_APP_TUI)
    find_package(Qt6 6.11 REQUIRED COMPONENTS Core5Compat)
endif()

include(CompilerWarnings)

# Third-party dependencies are vendored by the flake and consumed here
# (md4qt + earcut for the BlockEditor renderer; QWindowKit etc. on desktop).
include(Dependencies)

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
# be turned off for lean release builds.
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
