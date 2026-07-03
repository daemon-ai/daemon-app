# Experimental terminal frontend built on Tui Widgets (Qt Core based). This is a
# feasibility spike: it reuses the app's C++ view models/controllers unchanged
# and renders them with Tui Widgets instead of QML.
#
# Tui Widgets is a Meson project; we consume its installed pkg-config module
# (TuiWidgetsQt6) rather than driving Meson from CMake. The Nix flake's `tui`
# package provides it (plus termpaint + posixsignalmanager) on PKG_CONFIG_PATH.

find_package(PkgConfig REQUIRED)
pkg_check_modules(TUIWIDGETS REQUIRED IMPORTED_TARGET TuiWidgetsQt6)
# termpaint is the C layer beneath Tui Widgets. It is only a Requires.private of
# TuiWidgetsQt6, so its headers are not on the public include path; pull it in
# explicitly so the mouse layer can read termpaint_event.h (TERMPAINT_EV_MOUSE).
pkg_check_modules(TERMPAINT REQUIRED IMPORTED_TARGET termpaint)

# ---------------------------------------------------------------------------
# da_tui_viewmodels - the existing view models/controllers, compiled WITHOUT the
# Qt Quick UI stack.
#
# The GUI feature libs (da_sidebar, da_sessionslist, da_session_view) are
# qt_add_qml_module targets that link Qt6::Quick + Theme + Controls (and the
# session one drags in Transcript/Composer/BlockEditor). None of that is
# needed by the C++ classes themselves: they only use QAbstractListModel/QObject
# plus the core domain + persistence libs. Recompiling just the .cpp here proves
# the model/controller layer is genuinely toolkit-agnostic - the basis of the
# recommended shared-core extraction - and lets the TUI bind the very same code.
# ---------------------------------------------------------------------------
add_library(da_tui_viewmodels STATIC
    ../DaemonApp/Sidebar/sidebar_model.h
    ../DaemonApp/Sidebar/sidebar_model.cpp
    ../DaemonApp/SessionsList/sessions_list_model.h
    ../DaemonApp/SessionsList/sessions_list_model.cpp
    ../DaemonApp/Session/session_controller.h
    ../DaemonApp/Session/session_controller.cpp
    ../DaemonApp/Session/session_orchestrator.h
    ../DaemonApp/Session/session_orchestrator.cpp
    ../DaemonApp/Session/todo_list_model.h
    ../DaemonApp/Session/todo_list_model.cpp
    ../DaemonApp/Session/subagent_model.h
    ../DaemonApp/Session/subagent_model.cpp
    ../DaemonApp/Tabs/tab_model.h
    ../DaemonApp/Tabs/tab_model.cpp
    ../DaemonApp/Command/command_registry.h
    ../DaemonApp/Command/command_registry.cpp
    # File tree + editor view-models, shared verbatim with the GUI.
    ../DaemonApp/Files/fs_explorer_model.h
    ../DaemonApp/Files/fs_explorer_model.cpp
    ../DaemonApp/Files/file_finder_model.h
    ../DaemonApp/Files/file_finder_model.cpp
    # Participants section model, shared verbatim with the GUI.
    ../DaemonApp/Participants/participants_model.h
    ../DaemonApp/Participants/participants_model.cpp
    ../DaemonApp/Editor/app/code_editor_controller.h
    ../DaemonApp/Editor/app/code_editor_controller.cpp
    ../DaemonApp/Editor/app/line_model.h
    ../DaemonApp/Editor/app/line_model.cpp
    # Memory view-models, shared verbatim with the GUI (graph render is GUI-only;
    # the TUI projects the same models as markdown / adjacency).
    ../DaemonApp/Memory/memory_list_model.h
    ../DaemonApp/Memory/memory_list_model.cpp
    ../DaemonApp/Memory/memory_stats_model.h
    ../DaemonApp/Memory/memory_stats_model.cpp
    ../DaemonApp/Memory/memory_timeline_model.h
    ../DaemonApp/Memory/memory_timeline_model.cpp
    ../DaemonApp/Memory/memory_graph_model.h
    ../DaemonApp/Memory/memory_graph_model.cpp
)
add_library(daemon-app::tui_viewmodels ALIAS da_tui_viewmodels)

set_target_properties(da_tui_viewmodels PROPERTIES AUTOMOC ON)

# Expose the three feature dirs so both these sources and the TUI executable can
# include the model headers by basename.
target_include_directories(da_tui_viewmodels PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../DaemonApp/Sidebar"
    "${CMAKE_CURRENT_SOURCE_DIR}/../DaemonApp/SessionsList"
    "${CMAKE_CURRENT_SOURCE_DIR}/../DaemonApp/Session"
    "${CMAKE_CURRENT_SOURCE_DIR}/../DaemonApp/Tabs"
    "${CMAKE_CURRENT_SOURCE_DIR}/../DaemonApp/Command"
    "${CMAKE_CURRENT_SOURCE_DIR}/../DaemonApp/Files"
    "${CMAKE_CURRENT_SOURCE_DIR}/../DaemonApp/Participants"
    "${CMAKE_CURRENT_SOURCE_DIR}/../DaemonApp/Editor"
    "${CMAKE_CURRENT_SOURCE_DIR}/../DaemonApp/Editor/app"
    "${CMAKE_CURRENT_SOURCE_DIR}/../DaemonApp/Memory"
)

target_link_libraries(da_tui_viewmodels
    PUBLIC
        # Qt6::Qml only for the QML_ELEMENT / qqmlregistration.h attribute macros
        # on the classes; no QML engine is ever instantiated by the TUI.
        Qt6::Qml
        daemon-app::domain
        daemon-app::persistence
        # SessionOrchestrator owns a TurnController (turn_controller.h).
        daemon-app::turn
        # FsExplorerModel talks to the fs seam; the editor controller owns the
        # de_core engine (block buffer + incremental KSyntaxHighlighting).
        daemon-app::fs
        daemon-app::editor_core
        # The memory view-models talk to the memory-inspection seam.
        daemon-app::memory
    PRIVATE
        daemon-app::warnings
)

# ---------------------------------------------------------------------------
# daemon-tui - the terminal application executable.
# ---------------------------------------------------------------------------
qt_add_executable(daemon-tui
    main.cpp
    mouse_terminal.h
    mouse_terminal.cpp
    root_widget.h
    root_widget.cpp
    root_widget_detail.h
    root_widget_detail.cpp
    root_widget_wiring.cpp
    root_widget_input.cpp
    root_widget_tabs.cpp
    root_widget_pages.cpp
    root_widget_strips.cpp
    root_widget_headless.cpp
    submit_input_box.h
    submit_input_box.cpp
    search_input_box.h
    search_input_box.cpp
    tree_list_view.h
    tree_list_view.cpp
    tui_dialogs.h
    tui_dialogs.cpp
    # First-run gate dialog (split out of tui_dialogs.cpp so the first-run
    # workstream owns one focused TU).
    dialogs/first_run_dialog.h
    dialogs/first_run_dialog.cpp
    tui_page_hub.h
    tui_page_hub.cpp
    # Hand-maintained parity coverage declaration (enforced by
    # tests/tui/tui_parity_tests.cpp): the TabModel kinds + command ids the TUI
    # routes. Keep in sync with the dispatchers when adding routes.
    tui_parity_routes.h
    tui_parity_routes.cpp
    # Per-page hub projections (one TU per Wave-1 ownership area; the dispatch
    # switch stays in tui_page_hub.cpp).
    pages/hub_detail.h
    pages/hub_settings.cpp
    pages/hub_models.cpp
    pages/hub_accounts.cpp
    pages/hub_profiles.cpp
    pages/hub_memory.cpp
    pages/hub_ops.cpp
    tui_file_tab_controller.h
    tui_file_tab_controller.cpp
    tui_shell_layout.h
    tui_shell_layout.cpp
    tui_overlay_host.h
    tui_overlay_host.cpp
    tab_session_manager.h
    tab_session_manager.cpp
    display_role_adapter.h
    display_role_adapter.cpp
    tui_palette.h
    tui_palette.cpp
    transcript_render.h
    transcript_render.cpp
    transcript_view.h
    transcript_view.cpp
    session_list_view.h
    session_list_view.cpp
    queue_strip_view.h
    queue_strip_view.cpp
    attachment_bar_view.h
    attachment_bar_view.cpp
    status_bar_view.h
    status_bar_view.cpp
    tab_strip_view.h
    tab_strip_view.cpp
    file_tree_view.h
    file_tree_view.cpp
    participants_view.h
    participants_view.cpp
    code_editor_view.h
    code_editor_view.cpp
    composer_chrome.h
    composer_chrome.cpp
    completion_view.h
    completion_view.cpp
    palette_dialog.h
    palette_dialog.cpp
    file_finder_dialog.h
    file_finder_dialog.cpp
    interactive_turn_host.h
    interactive_turn_host.cpp
    line_editor.h
    line_editor.cpp
)

# main.cpp includes the generated daemon_app_version.h; depend on the target that refreshes it.
add_dependencies(daemon-tui daemon_app_version_header)

set_target_properties(daemon-tui PROPERTIES AUTOMOC ON)

# Vendored Tui Widgets private headers (ZListView_p.h + its ZWidget_p.h/ListNode_p.h
# deps), pinned to the same revision as the flake's tuiwidgets input. Searched
# before the installed headers so `<Tui/ZListView_p.h>` resolves here; the public
# `<Tui/*.h>` they include still come from the installed package. Used only by
# root_widget.cpp to read ZListView's private scroll offset.
# The TUI root itself is also on the include path so the nested TUs (pages/,
# dialogs/) include the shell headers by basename, matching the flat files.
target_include_directories(daemon-tui PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor"
)

target_link_libraries(daemon-tui
    PRIVATE
        PkgConfig::TUIWIDGETS
        PkgConfig::TERMPAINT
        Qt6::Core
        Qt6::Core5Compat
        daemon-app::i18n
        daemon-app::tui_viewmodels
        daemon-app::presentation
        daemon-app::theme_core
        daemon-app::composer_session
        daemon-app::turn
        daemon-app::status_model
        daemon-app::export
        daemon-app::domain
        daemon-app::persistence
        daemon-app::settings_store
        daemon-app::connection
        daemon-app::nav
        daemon-app::firstrun
        daemon-app::config
        daemon-app::uimodels
        daemon-app::models
        daemon-app::accounts
        daemon-app::profiles
        daemon-app::fleet
        daemon-app::automation
        daemon-app::session
        daemon-app::daemon_client
        # The GUI's parse/ingest/view-model engine. It links Qt Gui/Sql + md4qt at
        # link time but needs no QGuiApplication (QColor works under QCoreApplication),
        # so the TUI shares the exact same transcript blocks as the QML front end.
        be_core
        # The SessionLogEntry <-> document bridge (roadmap P4), shared with the GUI.
        be_transcript
        # File tree + editor: the fs seam and the de_core engine, shared with GUI.
        daemon-app::fs
        daemon-app::editor_core
        # Memory-inspection seam (seeded mock) + shared view-models.
        daemon-app::memory
        daemon-app::warnings
        daemon-app::version
)

include(GNUInstallDirs)
install(TARGETS daemon-tui
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
