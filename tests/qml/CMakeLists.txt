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
