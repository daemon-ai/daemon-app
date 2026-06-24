qt_add_qml_module(da_session_view
    STATIC
    URI DaemonApp.Session
    VERSION 1.0
    SOURCES
        session_controller.h
        session_controller.cpp
        session_orchestrator.h
        session_orchestrator.cpp
        todo_list_model.h
        todo_list_model.cpp
        subagent_model.h
        subagent_model.cpp
    QML_FILES
        Session.qml
        TranscriptPage.qml
        ToolGroup.qml
)
add_library(daemon-app::session_view ALIAS da_session_view)

target_link_libraries(da_session_view
    PRIVATE
        Qt6::Quick
        daemon-app::domain
        daemon-app::persistence
        daemon-app::theme
        daemon-app::controls
        daemon-app::settings
        daemon-app::tabs
        daemon-app::transcript
        daemon-app::composer
        daemon-app::turn
        daemon-app::pages
        daemon-app::memory_views
        daemon-app::warnings
)
