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
# The GUI feature libs (da_sidebar, da_conversationslist, da_conversation) are
# qt_add_qml_module targets that link Qt6::Quick + Theme + Controls (and the
# conversation one drags in Transcript/Composer/BlockEditor). None of that is
# needed by the C++ classes themselves: they only use QAbstractListModel/QObject
# plus the core domain + persistence libs. Recompiling just the .cpp here proves
# the model/controller layer is genuinely toolkit-agnostic - the basis of the
# recommended shared-core extraction - and lets the TUI bind the very same code.
# ---------------------------------------------------------------------------
add_library(da_tui_viewmodels STATIC
    ../DaemonApp/Sidebar/sidebar_model.h
    ../DaemonApp/Sidebar/sidebar_model.cpp
    ../DaemonApp/ConversationsList/conversations_list_model.h
    ../DaemonApp/ConversationsList/conversations_list_model.cpp
    ../DaemonApp/Conversation/conversation_controller.h
    ../DaemonApp/Conversation/conversation_controller.cpp
    ../DaemonApp/Conversation/conversation_orchestrator.h
    ../DaemonApp/Conversation/conversation_orchestrator.cpp
    ../DaemonApp/Conversation/todo_list_model.h
    ../DaemonApp/Conversation/todo_list_model.cpp
)
add_library(daemon-app::tui_viewmodels ALIAS da_tui_viewmodels)

set_target_properties(da_tui_viewmodels PROPERTIES AUTOMOC ON)

# Expose the three feature dirs so both these sources and the TUI executable can
# include the model headers by basename.
target_include_directories(da_tui_viewmodels PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../DaemonApp/Sidebar"
    "${CMAKE_CURRENT_SOURCE_DIR}/../DaemonApp/ConversationsList"
    "${CMAKE_CURRENT_SOURCE_DIR}/../DaemonApp/Conversation"
)

target_link_libraries(da_tui_viewmodels
    PUBLIC
        # Qt6::Qml only for the QML_ELEMENT / qqmlregistration.h attribute macros
        # on the classes; no QML engine is ever instantiated by the TUI.
        Qt6::Qml
        daemon-app::domain
        daemon-app::persistence
        # ConversationOrchestrator owns a TurnController (turn_controller.h).
        daemon-app::turn
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
    display_role_adapter.h
    display_role_adapter.cpp
    tui_palette.h
    tui_palette.cpp
    transcript_render.h
    transcript_render.cpp
    transcript_view.h
    transcript_view.cpp
    conversation_list_view.h
    conversation_list_view.cpp
    queue_strip_view.h
    queue_strip_view.cpp
    attachment_bar_view.h
    attachment_bar_view.cpp
    status_bar_view.h
    status_bar_view.cpp
    composer_chrome.h
    composer_chrome.cpp
    completion_view.h
    completion_view.cpp
    interactive_turn_host.h
    interactive_turn_host.cpp
    line_editor.h
    line_editor.cpp
)

set_target_properties(daemon-tui PROPERTIES AUTOMOC ON)

# Vendored Tui Widgets private headers (ZListView_p.h + its ZWidget_p.h/ListNode_p.h
# deps), pinned to the same revision as the flake's tuiwidgets input. Searched
# before the installed headers so `<Tui/ZListView_p.h>` resolves here; the public
# `<Tui/*.h>` they include still come from the installed package. Used only by
# root_widget.cpp to read ZListView's private scroll offset.
target_include_directories(daemon-tui PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor"
)

target_link_libraries(daemon-tui
    PRIVATE
        PkgConfig::TUIWIDGETS
        PkgConfig::TERMPAINT
        Qt6::Core
        Qt6::Core5Compat
        daemon-app::tui_viewmodels
        daemon-app::presentation
        daemon-app::theme_core
        daemon-app::composer_session
        daemon-app::turn
        daemon-app::status_model
        daemon-app::domain
        daemon-app::persistence
        # The GUI's parse/ingest/view-model engine. It links Qt Gui/Sql + md4qt at
        # link time but needs no QGuiApplication (QColor works under QCoreApplication),
        # so the TUI shares the exact same transcript blocks as the QML front end.
        be_core
        daemon-app::warnings
)

include(GNUInstallDirs)
install(TARGETS daemon-tui
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
