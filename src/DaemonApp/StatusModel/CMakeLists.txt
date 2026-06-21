# Shared status-bar core: the daemon-bound footer state plus the derived/formatted
# strings (gateway tone, agent detail, context bar/label, live turn/session
# timers) extracted from StatusBar.qml so the GUI footer and the TUI footer share
# one formatting path and one 1s tick. Consumed by the GUI as the
# DaemonApp.StatusModel QML module and by the TUI as a plain C++ lib. Quick-free:
# Qt6::Qml (for QML_ELEMENT) plus Qt6::Core for QTimer.
qt_add_qml_module(da_status_model
    STATIC
    URI DaemonApp.StatusModel
    VERSION 1.0
    SOURCES
        status_bar_model.h
        status_bar_model.cpp
)
add_library(daemon-app::status_model ALIAS da_status_model)

target_include_directories(da_status_model PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(da_status_model
    PUBLIC
        Qt6::Qml
        Qt6::Core
    PRIVATE
        daemon-app::warnings
)
