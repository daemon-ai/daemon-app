find_package(Qt6 6.11 REQUIRED COMPONENTS QuickTest Qml Quick)

# tst_theme: QtQuickTest over the Theme singleton. Asserts setTheme switches and
# that each of Light/Dark/Sepia yields the expected token hexes (guards the
# id/property collision regression). QUICK_TEST_SOURCE_DIR points the runner at
# the tst_*.qml in this dir.
qt_add_executable(tst_theme tst_theme_main.cpp)
target_link_libraries(tst_theme
    PRIVATE
        Qt6::QuickTest
        Qt6::Qml
        Qt6::Quick
        da_theme
        da_themeplugin
)
target_compile_definitions(tst_theme
    PRIVATE QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/theme"
)

add_test(NAME tst_theme COMMAND tst_theme)
set_tests_properties(tst_theme PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software"
)

# tst_composer: QtQuickTest over the ported message composer. Drives send/queue/
# history/draft behaviors through the composer's public surface. Composer pulls
# in Controls (QtQuick.Controls), so a Controls style must be available offscreen
# (Basic is always present).
find_package(Qt6 6.11 REQUIRED COMPONENTS QuickControls2)
qt_add_executable(tst_composer tst_composer_main.cpp)
target_link_libraries(tst_composer
    PRIVATE
        Qt6::QuickTest
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        da_theme
        da_themeplugin
        da_controls
        da_controlsplugin
        da_composer_session
        da_composer_sessionplugin
        da_composer
        da_composerplugin
)
target_compile_definitions(tst_composer
    PRIVATE
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/composer"
        TEST_QML_IMPORT_PATH="$ENV{QML2_IMPORT_PATH}"
)

add_test(NAME tst_composer COMMAND tst_composer)
set_tests_properties(tst_composer PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QUICK_CONTROLS_STYLE=Basic"
)

# tst_chip: QtQuickTest over the Kit Chip control. Asserts tone -> text/border
# tint, icon/close affordance visibility, clicked()/closeRequested() emission,
# and the non-interactive default. Chip pulls in QtQuick.Controls (Label/
# ToolTip), so the Basic style must be available offscreen.
qt_add_executable(tst_chip tst_chip_main.cpp)
target_link_libraries(tst_chip
    PRIVATE
        Qt6::QuickTest
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        da_theme
        da_themeplugin
        da_controls
        da_controlsplugin
)
target_compile_definitions(tst_chip
    PRIVATE
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/controls"
        TEST_QML_IMPORT_PATH="$ENV{QML2_IMPORT_PATH}"
)

add_test(NAME tst_chip COMMAND tst_chip)
set_tests_properties(tst_chip PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QUICK_CONTROLS_STYLE=Basic"
)

# tst_tabs: QtQuickTest over the pane tab strip. Drives a real TabBar + StackLayout
# off the shared TabModel - add/remove/switch, find-or-create re-activation, the
# "+"/settings affordances, and click-to-activate. Pulls in Controls/Layouts, so a
# Controls style must be available offscreen (Basic is always present).
qt_add_executable(tst_tabs tst_tabs_main.cpp)
target_link_libraries(tst_tabs
    PRIVATE
        Qt6::QuickTest
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        da_theme
        da_themeplugin
        da_controls
        da_controlsplugin
        da_tabs
        da_tabsplugin
)
target_compile_definitions(tst_tabs
    PRIVATE
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tabs"
        TEST_QML_IMPORT_PATH="$ENV{QML2_IMPORT_PATH}"
)

add_test(NAME tst_tabs COMMAND tst_tabs)
set_tests_properties(tst_tabs PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QUICK_CONTROLS_STYLE=Basic"
)

# tst_downloads_panel: QtQuickTest over the REAL DownloadsPanel (DaemonApp.Pages), driven by a
# MockModelCatalog injected as the app-global ModelCatalog context property. Guards the
# rows-render / self-hide / activeOnly-terminal behaviors, the multi-file "file x/y" label, and
# the "Use this model" installedIdFor resolution. The Pages module is static, so its plugin (and
# its transitive backing libs) link explicitly, like the GUI executable does.
qt_add_executable(tst_downloads_panel tst_downloads_panel_main.cpp)
target_link_libraries(tst_downloads_panel
    PRIVATE
        Qt6::QuickTest
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        daemon-app::models
        da_theme
        da_themeplugin
        da_controls
        da_controlsplugin
        da_pages
        da_pagesplugin
)
target_compile_definitions(tst_downloads_panel
    PRIVATE
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/downloads_panel"
        TEST_QML_IMPORT_PATH="$ENV{QML2_IMPORT_PATH}"
)

add_test(NAME tst_downloads_panel COMMAND tst_downloads_panel)
set_tests_properties(tst_downloads_panel PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QUICK_CONTROLS_STYLE=Basic"
)
