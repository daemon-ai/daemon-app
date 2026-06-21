# Shared composer session core: the draft/queue/history/attachment FSM extracted
# from Composer.qml so the QML GUI and the TUI drive identical behavior. Consumed
# by the GUI as the DaemonApp.ComposerSession QML module and by the TUI as a plain
# C++ lib. Quick-free: only Qt6::Qml (for QML_ELEMENT) plus the domain enums.
qt_add_qml_module(da_composer_session
    STATIC
    URI DaemonApp.ComposerSession
    VERSION 1.0
    SOURCES
        composer_queue_model.h
        composer_queue_model.cpp
        composer_attachment_model.h
        composer_attachment_model.cpp
        completion_model.h
        completion_model.cpp
        composer_session_controller.h
        composer_session_controller.cpp
)
add_library(daemon-app::composer_session ALIAS da_composer_session)

target_include_directories(da_composer_session PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(da_composer_session
    PUBLIC
        Qt6::Qml
    PRIVATE
        daemon-app::warnings
)
