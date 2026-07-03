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

# tui_parity_tests: the GUI<->TUI parity guardrail. Every TabModel::Kind and
# every CommandRegistry id must have a TUI route (declared in
# src/tui/tui_parity_routes.cpp beside the dispatchers) or an explicit,
# reasoned accepted-divergence entry inside the test. Qt-Core-only sources, so
# it runs headless with the rest of the suite.
qt_add_executable(tui_parity_tests
    tui_parity_tests.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/tui_parity_routes.cpp
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Tabs/tab_model.cpp
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Command/command_registry.cpp
)
set_target_properties(tui_parity_tests PROPERTIES AUTOMOC ON)
target_include_directories(tui_parity_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/tui
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Tabs
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Command
)
target_link_libraries(tui_parity_tests
    PRIVATE
        Qt6::Test
        Qt6::Qml
)
add_test(NAME tui_parity_tests COMMAND tui_parity_tests)
set_tests_properties(tui_parity_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_file_finder_dialog: the Ctrl+G fuzzy file-finder overlay (and the Ctrl+O
# attach picker's kind heuristic). Drives a real FileFinderDialog bound to the
# shared FileFinderModel, indexing a temp workspace through LocalDiskFsService
# in an off-screen Tui terminal: type -> fuzzy results -> Enter previews /
# Shift+Enter pins / Esc dismisses.
qt_add_executable(tui_file_finder_tests
    tst_file_finder_dialog.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/file_finder_dialog.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/palette_dialog.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/root_widget_detail.cpp
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Files/file_finder_model.cpp
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Tabs/tab_model.cpp
)
set_target_properties(tui_file_finder_tests PROPERTIES AUTOMOC ON)
target_include_directories(tui_file_finder_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/tui
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Files
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Tabs
)
target_link_libraries(tui_file_finder_tests
    PRIVATE
        Qt6::Test
        Qt6::Qml
        PkgConfig::TUIWIDGETS
        daemon-app::fs
        daemon-app::domain
        daemon-app::persistence
)
add_test(NAME tui_file_finder_tests COMMAND tui_file_finder_tests)
set_tests_properties(tui_file_finder_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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

# Zen mode round-trip: /distraction (typed + completion accept) hides the
# sidebar / session-list / right columns, a second toggle restores them - so
# the test ends in the original persisted state.
add_test(NAME daemon_tui_offscreen_distraction
    COMMAND ${CMAKE_COMMAND} -E env
        DAEMON_TUI_OFFSCREEN=120x36
        DAEMON_TUI_KEYS=focus-composer,t:/distraction,enter,t:/distraction,enter
        $<TARGET_FILE:daemon-tui>
)

# File-finder + attach-picker overlays through the real shell wiring: Ctrl+G
# opens the "go to file" finder (bubble path), Esc dismisses it; Ctrl+O from
# the focused composer opens the attach picker, Esc hands focus back.
add_test(NAME daemon_tui_offscreen_finder_attach
    COMMAND ${CMAKE_COMMAND} -E env
        DAEMON_TUI_OFFSCREEN=120x36
        DAEMON_TUI_KEYS=ctrl-g,esc,focus-composer,ctrl-o,esc
        $<TARGET_FILE:daemon-tui>
)
