# Shared turn-lifecycle core: the scripted assistant-turn FSM extracted from
# TurnSimulator.qml so the QML GUI and the TUI drive identical turns. Consumed by
# the GUI as the DaemonApp.Turn QML module (feeding EditorController::ingestEvents)
# and by the TUI as a plain C++ lib. Quick-free: Qt6::Qml (for QML_ELEMENT) plus
# Qt6::Core for QTimer/QVariant.
qt_add_qml_module(da_turn
    STATIC
    URI DaemonApp.Turn
    VERSION 1.0
    SOURCES
        i_turn_engine.h
        turn_controller.h
        turn_controller.cpp
        daemon_turn_engine.h
        daemon_turn_engine.cpp
        turn_engine_factory.h
)
add_library(daemon-app::turn ALIAS da_turn)

target_include_directories(da_turn PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(da_turn
    PUBLIC
        Qt6::Qml
        Qt6::Core
        # The daemon engine wraps the shared NodeApiClient + codec facade.
        daemon-app::daemon_client
    PRIVATE
        daemon-app::warnings
)
