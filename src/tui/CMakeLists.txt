# Experimental terminal frontend built on Tui Widgets (Qt Core based). This is a
# feasibility spike: it reuses the app's C++ view models/controllers unchanged
# and renders them with Tui Widgets instead of QML.
#
# Tui Widgets is a Meson project; we consume its installed pkg-config module
# (TuiWidgetsQt6) rather than driving Meson from CMake. The Nix flake's `tui`
# package provides it (plus termpaint + posixsignalmanager) on PKG_CONFIG_PATH.

find_package(PkgConfig REQUIRED)
pkg_check_modules(TUIWIDGETS REQUIRED IMPORTED_TARGET TuiWidgetsQt6)

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
    PRIVATE
        daemon-app::warnings
)

# ---------------------------------------------------------------------------
# daemon-tui - the terminal application executable.
# ---------------------------------------------------------------------------
qt_add_executable(daemon-tui
    main.cpp
    root_widget.h
    root_widget.cpp
    display_role_adapter.h
    display_role_adapter.cpp
    tui_palette.h
    tui_palette.cpp
)

set_target_properties(daemon-tui PROPERTIES AUTOMOC ON)

target_link_libraries(daemon-tui
    PRIVATE
        PkgConfig::TUIWIDGETS
        Qt6::Core
        Qt6::Core5Compat
        daemon-app::tui_viewmodels
        daemon-app::presentation
        daemon-app::composer_session
        daemon-app::turn
        daemon-app::status_model
        daemon-app::domain
        daemon-app::persistence
        daemon-app::warnings
)

include(GNUInstallDirs)
install(TARGETS daemon-tui
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
