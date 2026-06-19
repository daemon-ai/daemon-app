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
