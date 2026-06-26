qt_add_qml_module(da_terminal
    STATIC
    URI DaemonApp.Terminal
    VERSION 1.0
    QML_FILES
        TerminalPanel.qml
)
add_library(daemon-app::terminal ALIAS da_terminal)

# TerminalPanel.qml imports the external QMLTermWidget plugin (built by the flake
# as qmltermwidget-qt6). Its QML dir comes in either as a CMake cache var
# (-DQMLTERMWIDGET_QML_DIR, set by the Nix package) or, for `nix develop` builds,
# as the QMLTERMWIDGET_QML_DIR env var the devShell exports. Point qmllint at it
# so `import QMLTermWidget` resolves at build time.
if(NOT QMLTERMWIDGET_QML_DIR AND DEFINED ENV{QMLTERMWIDGET_QML_DIR})
    set(QMLTERMWIDGET_QML_DIR "$ENV{QMLTERMWIDGET_QML_DIR}")
endif()
if(QMLTERMWIDGET_QML_DIR)
    set_property(TARGET da_terminal APPEND PROPERTY QT_QML_IMPORT_PATH "${QMLTERMWIDGET_QML_DIR}")
endif()

target_link_libraries(da_terminal
    PRIVATE
        Qt6::Quick
        daemon-app::theme
        daemon-app::warnings
)
