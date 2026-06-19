cmake_minimum_required(VERSION 3.21)

project(daemon-app
    VERSION 0.1.0
    DESCRIPTION "AI agent chat application (Qt 6 QML/C++)"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
find_package(Qt6 6.11 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Network Sql)
if(NOT DAEMON_APP_MOBILE)
    find_package(Qt6 6.11 REQUIRED COMPONENTS Widgets)
endif()

qt_standard_project_setup(REQUIRES 6.11)

include(CompilerWarnings)

# Third-party dependencies are vendored by the flake and consumed here
# (md4qt + earcut for the BlockEditor renderer; QWindowKit etc. on desktop).
include(Dependencies)

add_subdirectory(src)

# Tests. Building/running them is opt-in (BUILD_TESTING, default ON) so it can
# be turned off for lean release builds.
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
