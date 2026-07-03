# TUI transcript-render tests. They exercise TranscriptLayout (src/tui) against
# the shared be_core parse engine, so they only build when the TUI frontend is
# enabled (DAEMON_APP_TUI), which is also when the TuiWidgets pkg-config module
# is on PKG_CONFIG_PATH. Headless: only ZColor/ZTextAttributes are used (no real
# terminal), so they run under ctest like the other engine tests.

find_package(Qt6 6.11 REQUIRED COMPONENTS Test)
find_package(PkgConfig REQUIRED)
pkg_check_modules(TUIWIDGETS REQUIRED IMPORTED_TARGET TuiWidgetsQt6)

qt_add_executable(tui_transcript_tests
    tst_transcript_render.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/transcript_render.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/tui_palette.cpp
)
set_target_properties(tui_transcript_tests PROPERTIES AUTOMOC ON)
target_include_directories(tui_transcript_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/tui)
target_link_libraries(tui_transcript_tests
    PRIVATE
        Qt6::Test
        PkgConfig::TUIWIDGETS
        be_core
        daemon-app::theme_core
)
add_test(NAME tui_transcript_tests COMMAND tui_transcript_tests)
set_tests_properties(tui_transcript_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_transcript_view: drives the TranscriptView widget in an off-screen Tui
# terminal with a focus-cycling root - asserts that an interactive block lets Tab
# / Shift+Tab leave to the next pane while arrow keys walk the block's controls.
qt_add_executable(tui_transcript_view_tests
    tst_transcript_view.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/transcript_view.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/transcript_render.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/tui_palette.cpp
)
set_target_properties(tui_transcript_view_tests PROPERTIES AUTOMOC ON)
target_include_directories(tui_transcript_view_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/tui)
target_link_libraries(tui_transcript_view_tests
    PRIVATE
        Qt6::Test
        PkgConfig::TUIWIDGETS
        be_core
        daemon-app::theme_core
)
add_test(NAME tui_transcript_view_tests COMMAND tui_transcript_view_tests)
set_tests_properties(tui_transcript_view_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_tui_palette: proves the TUI palette is theme-aware - each accessor resolves
# the active theme's shared token to a ZColor, and switching the active theme
# changes the resolved colors (Light vs Dark vs Sepia vs Midnight).
qt_add_executable(tui_palette_tests
    tst_tui_palette.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/tui_palette.cpp
)
set_target_properties(tui_palette_tests PROPERTIES AUTOMOC ON)
target_include_directories(tui_palette_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/tui)
target_link_libraries(tui_palette_tests
    PRIVATE
        Qt6::Test
        PkgConfig::TUIWIDGETS
        be_core
        daemon-app::theme_core
)
add_test(NAME tui_palette_tests COMMAND tui_palette_tests)
set_tests_properties(tui_palette_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_tab_strip: drives the TabStripView (bound to the shared TabModel) in an
# off-screen Tui terminal - asserts the painted chips/"+" and the click hit-test
# (activate / close / new-tab). Needs Qt6::Qml for the TabModel's QML_ELEMENT macro.
qt_add_executable(tui_tab_strip_tests
    tst_tab_strip.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/tab_strip_view.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/tui_palette.cpp
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Tabs/tab_model.cpp
)
set_target_properties(tui_tab_strip_tests PROPERTIES AUTOMOC ON)
target_include_directories(tui_tab_strip_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/tui
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Tabs
)
target_link_libraries(tui_tab_strip_tests
    PRIVATE
        Qt6::Test
        Qt6::Qml
        PkgConfig::TUIWIDGETS
        be_core
        daemon-app::theme_core
)
add_test(NAME tui_tab_strip_tests COMMAND tui_tab_strip_tests)
set_tests_properties(tui_tab_strip_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_tab_title_refresh: the stale-tab-title fix - a session-store changed()
# (e.g. the node's auto-title landing via the roster refetch, with no content
# change) re-resolves every open transcript tab's title through
# rwdetail::refreshTranscriptTabTitles, leaving page tabs untouched.
qt_add_executable(tui_tab_title_refresh_tests
    tst_tab_title_refresh.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/root_widget_detail.cpp
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Tabs/tab_model.cpp
)
set_target_properties(tui_tab_title_refresh_tests PROPERTIES AUTOMOC ON)
target_include_directories(tui_tab_title_refresh_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/tui
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Tabs
)
target_link_libraries(tui_tab_title_refresh_tests
    PRIVATE
        Qt6::Test
        Qt6::Qml
        daemon-app::domain
        daemon-app::persistence
)
add_test(NAME tui_tab_title_refresh_tests COMMAND tui_tab_title_refresh_tests)
set_tests_properties(tui_tab_title_refresh_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_test(NAME daemon_tui_offscreen_startup
    COMMAND ${CMAKE_COMMAND} -E env
        DAEMON_TUI_OFFSCREEN=100x30
        $<TARGET_FILE:daemon-tui>
)

add_test(NAME daemon_tui_offscreen_tabs_and_find
    COMMAND ${CMAKE_COMMAND} -E env
        DAEMON_TUI_OFFSCREEN=120x36
        DAEMON_TUI_KEYS=ctrl-t,ctrl-tab,ctrl-f,esc
        $<TARGET_FILE:daemon-tui>
)

add_test(NAME daemon_tui_offscreen_explorer_theme
    COMMAND ${CMAKE_COMMAND} -E env
        DAEMON_TUI_OFFSCREEN=120x36
        DAEMON_TUI_KEYS=ctrl-e,cycle-theme
        $<TARGET_FILE:daemon-tui>
)
