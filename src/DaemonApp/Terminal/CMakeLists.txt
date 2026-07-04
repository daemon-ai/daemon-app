# The desktop panel embeds the external QMLTermWidget plugin (a native PTY -
# impossible in the browser sandbox or on mobile, and a shared qmake-built
# plugin that cannot ride a static-Qt binary; keeping GPL-2.0 qmltermwidget
# out of the statically linked artifact is also deliberate, see
# THIRD-PARTY-NOTICES.md). Those builds compile the placeholder
# TerminalPanelStub.qml into the SAME module URI instead, aliased to the real
# file name so the QML type stays `TerminalPanel` and consumers (Session.qml,
# the status-bar toggle) are platform-blind. Exactly one variant is part of
# the module per configure; the other is never compiled or linted.
if(DAEMON_APP_WASM OR DAEMON_APP_STATIC OR DAEMON_APP_MOBILE)
    set(_da_terminal_panel TerminalPanelStub.qml)
    # The alias renames the file inside the module; the typename property makes
    # the registered QML type `TerminalPanel` too (without it the qmldir would
    # read `TerminalPanelStub 1.0 TerminalPanel.qml` and consumers would break).
    set_source_files_properties(TerminalPanelStub.qml PROPERTIES
        QT_RESOURCE_ALIAS TerminalPanel.qml
        QT_QML_SOURCE_TYPENAME TerminalPanel
    )
else()
    set(_da_terminal_panel TerminalPanel.qml)
endif()

qt_add_qml_module(da_terminal
    STATIC
    URI DaemonApp.Terminal
    VERSION 1.0
    QML_FILES
        ${_da_terminal_panel}
)
add_library(daemon-app::terminal ALIAS da_terminal)

# TerminalPanel.qml imports the external QMLTermWidget plugin (built by the flake
# as qmltermwidget-qt6). Its QML dir comes in either as a CMake cache var
# (-DQMLTERMWIDGET_QML_DIR, set by the Nix package) or, for `nix develop` builds,
# as the QMLTERMWIDGET_QML_DIR env var the devShell exports. Point qmllint at it
# so `import QMLTermWidget` resolves at build time. The stub never imports it,
# so the lookup belongs to the dynamic desktop build only.
if(NOT DAEMON_APP_WASM AND NOT DAEMON_APP_STATIC AND NOT DAEMON_APP_MOBILE)
    if(NOT QMLTERMWIDGET_QML_DIR AND DEFINED ENV{QMLTERMWIDGET_QML_DIR})
        set(QMLTERMWIDGET_QML_DIR "$ENV{QMLTERMWIDGET_QML_DIR}")
    endif()
    if(QMLTERMWIDGET_QML_DIR)
        set_property(TARGET da_terminal APPEND PROPERTY QT_QML_IMPORT_PATH "${QMLTERMWIDGET_QML_DIR}")
    endif()
endif()

target_link_libraries(da_terminal
    PRIVATE
        Qt6::Quick
        daemon-app::theme
        daemon-app::warnings
)
