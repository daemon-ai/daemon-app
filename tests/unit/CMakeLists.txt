find_package(Qt6 6.11 REQUIRED COMPONENTS Test)

# tst_fonts: validates the bundled font assets (glyph coverage, family names) so
# an icon-font regression is caught in CI rather than on screen. It reads the
# files straight from the source tree (the same ones the Theme module embeds).
qt_add_executable(tst_fonts tst_fonts.cpp)
target_link_libraries(tst_fonts PRIVATE Qt6::Gui Qt6::Test)
target_compile_definitions(tst_fonts
    PRIVATE DAEMON_APP_FONTS_DIR="${CMAKE_SOURCE_DIR}/assets/fonts"
)

add_test(NAME tst_fonts COMMAND tst_fonts)
# QRawFont needs a platform integration; run headless in CI.
set_tests_properties(tst_fonts PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_store: exercises the in-memory store's uniformly-recursive agent tree -
# roots, arbitrary depth, subtree-folded counts/scopes, and creation.
qt_add_executable(tst_store tst_store.cpp)
target_link_libraries(tst_store
    PRIVATE
        Qt6::Test
        daemon-app::domain
        daemon-app::persistence
)

add_test(NAME tst_store COMMAND tst_store)
set_tests_properties(tst_store PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_sidebar_model: exercises the flattened agent-tree sidebar model - recursive
# depths, tree roles, expand/collapse, and node-scope selection.
qt_add_executable(tst_sidebar_model tst_sidebar_model.cpp)
target_include_directories(tst_sidebar_model
    PRIVATE "${CMAKE_SOURCE_DIR}/src/DaemonApp/Sidebar"
)
target_link_libraries(tst_sidebar_model
    PRIVATE
        Qt6::Test
        Qt6::Qml
        Qt6::Quick
        daemon-app::domain
        daemon-app::persistence
        daemon-app::sidebar
)

add_test(NAME tst_sidebar_model COMMAND tst_sidebar_model)
set_tests_properties(tst_sidebar_model PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
