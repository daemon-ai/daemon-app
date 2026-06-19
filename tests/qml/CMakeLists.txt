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
    PRIVATE QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_test(NAME tst_theme COMMAND tst_theme)
set_tests_properties(tst_theme PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software"
)
