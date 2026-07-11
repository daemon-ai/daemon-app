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

# tst_integrations_tree_view (A2 app/tree-ui): the GUI/TUI parity half - the SAME shared
# IntegrationsTreeModel renders through the DisplayRoleAdapter's Integrations kind (custom roles ->
# Qt::DisplayRole + Tui list decorations). Compiles the adapter + palette directly and links the
# shared TUI view-model lib (which now carries the integrations models) + the transport seams.
qt_add_executable(tui_integrations_tree_tests
    tst_integrations_tree_view.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/display_role_adapter.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/tui_palette.cpp
)
set_target_properties(tui_integrations_tree_tests PROPERTIES AUTOMOC ON)
target_include_directories(tui_integrations_tree_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/tui)
target_link_libraries(tui_integrations_tree_tests
    PRIVATE
        Qt6::Test
        Qt6::Qml
        PkgConfig::TUIWIDGETS
        daemon-app::tui_viewmodels
        daemon-app::transports
        daemon-app::uimodels
        daemon-app::presentation
        daemon-app::theme_core
        daemon-app::mirror_ingestor
)
add_test(NAME tui_integrations_tree_tests COMMAND tui_integrations_tree_tests)
set_tests_properties(tui_integrations_tree_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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

# tst_interactive_turn_host: render honesty (#2) - answering an inline approval
# clears the gate via the shared toolApprovalPatch contract and does NOT fabricate
# a tool result (no synthesized ok/stdout). The completion is rendered from the
# turn engine's event stream instead (node ToolFinished / mock scripted finish).
qt_add_executable(tui_interactive_turn_host_tests
    tst_interactive_turn_host.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/interactive_turn_host.cpp
)
set_target_properties(tui_interactive_turn_host_tests PROPERTIES AUTOMOC ON)
target_include_directories(tui_interactive_turn_host_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/tui)
target_link_libraries(tui_interactive_turn_host_tests
    PRIVATE
        Qt6::Test
        be_core
)
add_test(NAME tui_interactive_turn_host_tests COMMAND tui_interactive_turn_host_tests)
set_tests_properties(tui_interactive_turn_host_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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
    # The interactive hub_settings.cpp renders through the TUI palette.
    ${CMAKE_SOURCE_DIR}/src/tui/tui_palette.cpp
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
        # The interactive hub_settings.cpp binds theme + language seams.
        daemon-app::theme_core
        daemon-app::i18n
        # hub_memory.cpp localizes tier/veracity via DisplayPresenter.
        daemon-app::presentation
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
    ${CMAKE_SOURCE_DIR}/src/tui/agent_type_view.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/agent_setup_view.cpp
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
        # NewAgentDialog now embeds AgentSetupView over the shared setup view-model.
        daemon-app::setup
        daemon-app::models
)
add_test(NAME tui_accounts_wizard_tests COMMAND tui_accounts_wizard_tests)
set_tests_properties(tui_accounts_wizard_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_settings_page: the interactive Settings page - j/k + Space/Enter drive the
# TuiPageHub rows (asserting the ISettingsStore / IDaemonConfig seams reflect the
# edits), and the TuiSettingsEditor dialog flows (PaletteDialog list pick +
# TextPromptDialog) run in an off-screen Tui terminal against fake/mock seams.
# Compiles the hub dispatcher, so every pages/hub_*.cpp TU and its seam libs are
# in the target (the dispatcher references all the builders).
qt_add_executable(tui_settings_page_tests
    tst_settings_page.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/tui_page_hub.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_settings.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_models.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_accounts.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_profiles.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_memory.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_ops.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_users_access.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/pages/hub_channels.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/settings_editor.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/palette_dialog.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/tui_dialogs.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/agent_type_view.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/agent_setup_view.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/tui_palette.cpp
)
set_target_properties(tui_settings_page_tests PROPERTIES AUTOMOC ON)
target_include_directories(tui_settings_page_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/tui)
target_link_libraries(tui_settings_page_tests
    PRIVATE
        Qt6::Test
        Qt6::Qml
        PkgConfig::TUIWIDGETS
        daemon-app::theme_core
        daemon-app::i18n
        daemon-app::config
        daemon-app::settings_store
        daemon-app::connection
        daemon-app::models
        daemon-app::accounts
        daemon-app::profiles
        daemon-app::fleet
        daemon-app::automation
        daemon-app::memory
        daemon-app::uimodels
        daemon-app::daemon_client
        daemon-app::tui_viewmodels
        # NewAgentDialog (tui_dialogs.cpp) embeds AgentSetupView over the shared setup view-model.
        daemon-app::setup
        # hub_users_access.cpp / hub_channels.cpp (compiled with the dispatcher)
        # bind PrincipalModel (daemon_client, above) + the transports seams.
        daemon-app::transports
        # hub_memory.cpp localizes tier/veracity via DisplayPresenter.
        daemon-app::presentation
)
add_test(NAME tui_settings_page_tests COMMAND tui_settings_page_tests)
set_tests_properties(tui_settings_page_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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

# tst_first_run_dialog: the TUI first-run gate's W3 parity - the inference phase's agent-name
# input (prefilled from the chosen provider's label until the user edits it, passed to
# applyInferenceChoice so a NAMED agent is minted) and the hoisted key-validation gate (FIX 4):
# Finish blocked with the shared FirstRunModel's visible reason line until the entered key is
# proven by a non-empty authenticated listing. Drives a real FirstRunDialog off-screen.
qt_add_executable(tui_first_run_dialog_tests
    tst_first_run_dialog.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/dialogs/first_run_dialog.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/agent_type_view.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/agent_setup_view.cpp
)
set_target_properties(tui_first_run_dialog_tests PROPERTIES AUTOMOC ON)
target_include_directories(tui_first_run_dialog_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/tui
    ${CMAKE_SOURCE_DIR}/tests/unit   # cache_test_support.h (the mock-cache reset helper)
)
target_link_libraries(tui_first_run_dialog_tests
    PRIVATE
        Qt6::Test
        PkgConfig::TUIWIDGETS
        daemon-app::firstrun
        daemon-app::connection
        daemon-app::settings_store
        daemon-app::models
        daemon-app::profiles
        daemon-app::accounts
        # AgentSetupView binds the shared setup view-model (Phase D).
        daemon-app::setup
        # AgentTypeView (the agenttype step's shared picker) reads AcpRepository.
        daemon-app::daemon_client
)
add_test(NAME tui_first_run_dialog_tests COMMAND tui_first_run_dialog_tests)
set_tests_properties(tui_first_run_dialog_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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
        # AD: the fixture is the mirror-backed store (the InMemory fixture died).
        daemon-app::daemon_client
        daemon-app::mirror_ingestor
)
add_test(NAME tui_tab_title_refresh_tests COMMAND tui_tab_title_refresh_tests)
set_tests_properties(tui_tab_title_refresh_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_tab_title_mirror (M4 sub-gate 3): the same tab-title wiring over the MIRROR-backed store -
# a node-side retitle (keyed mirror upsert) re-titles the open transcript tab through
# refreshTranscriptTabTitles, the exact RootWidget wiring flipped to storeMirror. Only built
# with the substrate.
    qt_add_executable(tui_tab_title_mirror_tests
        tst_tab_title_mirror.cpp
        ${CMAKE_SOURCE_DIR}/src/tui/root_widget_detail.cpp
        ${CMAKE_SOURCE_DIR}/src/DaemonApp/Tabs/tab_model.cpp
    )
    set_target_properties(tui_tab_title_mirror_tests PROPERTIES AUTOMOC ON)
    target_include_directories(tui_tab_title_mirror_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src/tui
        ${CMAKE_SOURCE_DIR}/src/DaemonApp/Tabs
    )
    target_link_libraries(tui_tab_title_mirror_tests
        PRIVATE
            Qt6::Test
            Qt6::Qml
            daemon-app::domain
            daemon-app::persistence
            daemon-app::mirror_ingestor
            daemon-app::mirror_substrate
            daemon-app::daemon_client
    )
    add_test(NAME tui_tab_title_mirror_tests COMMAND tui_tab_title_mirror_tests)
    set_tests_properties(tui_tab_title_mirror_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_chat_activation (A4 app/chat-tabs; wire v38): the activation-dispatch rewire -
# activating a room/DM opens (or focuses) a native Chat tab for (transport, conv)
# via rwdetail::openConversationTab, instead of the Channels page it fell back to
# before. Drives the shared helper against the real TabModel (the GUI + TUI both
# route conversationActivated through it).
qt_add_executable(tui_chat_activation_tests
    tst_chat_activation.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/root_widget_detail.cpp
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Tabs/tab_model.cpp
)
set_target_properties(tui_chat_activation_tests PROPERTIES AUTOMOC ON)
target_include_directories(tui_chat_activation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/tui
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Tabs
)
target_link_libraries(tui_chat_activation_tests
    PRIVATE
        Qt6::Test
        Qt6::Qml
        daemon-app::domain
        daemon-app::persistence
)
add_test(NAME tui_chat_activation_tests COMMAND tui_chat_activation_tests)
set_tests_properties(tui_chat_activation_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_integrations_activation (AC sidebar composition; wire v38): the shared contract that
# activating an IntegrationsTreeModel conversation row opens a native Chat tab for
# (transport, conversation) via rwdetail::openConversationTab -> TabModel (the seam both shells
# install when mounting the integrations tree). Links the shared TUI view-model lib (which carries
# the integrations models) + the transport seams.
# TabModel + IntegrationsTreeModel live in the shared TUI view-model lib; only the free-function
# helper (rwdetail::openConversationTab) is compiled directly (it is not part of that lib).
qt_add_executable(tui_integrations_activation_tests
    tst_integrations_activation.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/root_widget_detail.cpp
)
set_target_properties(tui_integrations_activation_tests PROPERTIES AUTOMOC ON)
target_include_directories(tui_integrations_activation_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/tui
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Tabs
)
target_link_libraries(tui_integrations_activation_tests
    PRIVATE
        Qt6::Test
        Qt6::Qml
        daemon-app::tui_viewmodels
        daemon-app::transports
        daemon-app::domain
        daemon-app::persistence
        daemon-app::mirror_ingestor
)
add_test(NAME tui_integrations_activation_tests COMMAND tui_integrations_activation_tests)
set_tests_properties(tui_integrations_activation_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_profile_editor: the W5 TUI profile editor - drives ProfileEditorDialog
# (and its text-prompt / multi-line / checklist sub-flows, with real key
# events) against the mock profile store + mock provider catalog in an
# off-screen Tui terminal, asserting the store receives EXACTLY the field map
# the GUI editor's save() writes. tui_dialogs.cpp (TextPromptDialog) drags the
# AcpRepository declaration, hence daemon_client.
qt_add_executable(tui_profile_editor_tests
    tst_profile_editor.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/dialogs/profile_editor_dialog.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/palette_dialog.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/tui_dialogs.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/agent_type_view.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/agent_setup_view.cpp
)
set_target_properties(tui_profile_editor_tests PROPERTIES AUTOMOC ON)
target_include_directories(tui_profile_editor_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/tui
    ${CMAKE_SOURCE_DIR}/tests/unit
)
target_link_libraries(tui_profile_editor_tests
    PRIVATE
        Qt6::Test
        PkgConfig::TUIWIDGETS
        daemon-app::profiles
        daemon-app::models
        daemon-app::uimodels
        daemon-app::daemon_client
        # tui_dialogs.cpp's NewAgentDialog embeds AgentSetupView over the shared setup view-model.
        daemon-app::setup
)
add_test(NAME tui_profile_editor_tests COMMAND tui_profile_editor_tests)
set_tests_properties(tui_profile_editor_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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

# Profiles editor round-trip: open the Profiles hub (/profiles), press 'e' to
# open the profile editor on the selected row, Esc back out - smoke-proves the
# real RootWidget wiring (page action key -> editor dialog -> focus restore),
# on top of the unit-level tui_profile_editor_tests. Mock service mode: the
# editor only opens on a non-empty profile list (daemon mode offline is empty).
add_test(NAME daemon_tui_offscreen_profile_editor
    COMMAND ${CMAKE_COMMAND} -E env
        DAEMON_APP_SERVICE_MODE=mock
        DAEMON_TUI_OFFSCREEN=120x36
        DAEMON_TUI_KEYS=focus-composer,t:/profiles,enter,focus-transcript,t:e,esc
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
    # The interactive hub_settings.cpp renders through the TUI palette.
    ${CMAKE_SOURCE_DIR}/src/tui/tui_palette.cpp
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
        # The interactive hub_settings.cpp binds theme + language seams.
        daemon-app::theme_core
        daemon-app::i18n
        # hub_memory.cpp localizes tier/veracity via DisplayPresenter.
        daemon-app::presentation
)
add_test(NAME tui_hub_channels_tests COMMAND tui_hub_channels_tests)
set_tests_properties(tui_hub_channels_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Settings-page editing smoke test through the full shell: open via /settings,
# walk down to a notification toggle (row 3), Space-flip it twice (so the run
# ends in the original persisted state, like the distraction test), then walk
# back up and open + cancel the theme picker.
add_test(NAME daemon_tui_offscreen_settings_edit
    COMMAND ${CMAKE_COMMAND} -E env
        DAEMON_TUI_OFFSCREEN=120x36
        DAEMON_TUI_KEYS=focus-composer,t:/settings,enter,t:j,t:j,t:j,space,space,t:k,t:k,t:k,enter,esc
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
