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

# tui_access_gating_tests: the auth6 capability gates in the TUI (Wave 1) - the
# palette's "access" entry is hidden fail-closed until the wired principal
# grants access_admin (re-hiding on logout), and TuiPageHub::openManagerPage
# consumes but never mounts the UsersAccess page without the capability. Links
# the real page-hub dispatcher + builders so it exercises the shipped gate.
qt_add_executable(tui_access_gating_tests
    tst_tui_access_gating.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/tui_page_hub.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_settings.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_models.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_accounts.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_profiles.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_memory.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_ops.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_users_access.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_channels.cpp
)
set_target_properties(tui_access_gating_tests PROPERTIES AUTOMOC ON)
target_include_directories(tui_access_gating_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/tui)
target_link_libraries(tui_access_gating_tests
    PRIVATE
        Qt6::Test
        PkgConfig::TUIWIDGETS
        # TabModel + CommandRegistry + the memory view-models, shared with the GUI.
        daemon-app::tui_viewmodels
        # PrincipalModel (WhoAmI) + the seam interface libs the hub builders bind.
        daemon-app::daemon_client
        daemon-app::uimodels
        # hub_channels.cpp (compiled with the dispatcher) binds the transports seams.
        daemon-app::transports
)
add_test(NAME tui_access_gating_tests COMMAND tui_access_gating_tests)
set_tests_properties(tui_access_gating_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_accounts_wizard: the TUI add-account wizard against the mock accounts
# service in an off-screen terminal - provider pick -> (auth-method pick) ->
# masked key entry (-> base URL) -> the connected account row appears; the
# OAuth branch kicks the simulated handshake (pending -> connected); Esc aborts
# without mutation. Compiles the flow + the palette/dialog widgets it drives;
# tui_dialogs.cpp drags the NewAgentDialog deps (profiles + daemon_client).
qt_add_executable(tui_accounts_wizard_tests
    tst_accounts_wizard.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/dialogs/add_account_flow.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/palette_dialog.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/tui_dialogs.cpp
)
set_target_properties(tui_accounts_wizard_tests PROPERTIES AUTOMOC ON)
target_include_directories(tui_accounts_wizard_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/tui
    ${CMAKE_SOURCE_DIR}/tests/unit
)
target_link_libraries(tui_accounts_wizard_tests
    PRIVATE
        Qt6::Test
        PkgConfig::TUIWIDGETS
        daemon-app::accounts
        daemon-app::uimodels
        daemon-app::profiles
        daemon-app::daemon_client
)
add_test(NAME tui_accounts_wizard_tests COMMAND tui_accounts_wizard_tests)
set_tests_properties(tui_accounts_wizard_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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

# tst_hub_channels: the Channels hub page (Wave 1) - seeded transport-registry /
# presence seams land in the page markdown (accounts + states + rooms + adapter
# picker), absent seams degrade to an explicit note, and the "channels" manager
# route opens the TabModel::Channels tab. Compiles the TuiPageHub dispatcher, so
# it pulls in every per-page builder TU plus their seam libraries.
qt_add_executable(tui_hub_channels_tests
    tst_hub_channels.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/tui_page_hub.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_settings.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_models.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_accounts.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_profiles.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_memory.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_ops.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_channels.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_users_access.cpp
)
set_target_properties(tui_hub_channels_tests PROPERTIES AUTOMOC ON)
target_include_directories(tui_hub_channels_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/tui)
target_link_libraries(tui_hub_channels_tests
    PRIVATE
        Qt6::Test
        PkgConfig::TUIWIDGETS
        # TabModel + the memory view-models the hub builders bind (also exposes
        # the Tabs/Memory include dirs).
        daemon-app::tui_viewmodels
        daemon-app::config
        daemon-app::connection
        daemon-app::settings_store
        daemon-app::models
        daemon-app::accounts
        daemon-app::profiles
        daemon-app::fleet
        daemon-app::automation
        daemon-app::memory
        daemon-app::transports
        daemon-app::uimodels
        # hub_users_access.cpp (compiled with the dispatcher) binds PrincipalModel.
        daemon-app::daemon_client
)
add_test(NAME tui_hub_channels_tests COMMAND tui_hub_channels_tests)
set_tests_properties(tui_hub_channels_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
