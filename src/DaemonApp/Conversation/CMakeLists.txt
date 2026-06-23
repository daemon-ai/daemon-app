qt_add_qml_module(da_conversation
    STATIC
    URI DaemonApp.Conversation
    VERSION 1.0
    SOURCES
        conversation_controller.h
        conversation_controller.cpp
        conversation_orchestrator.h
        conversation_orchestrator.cpp
        todo_list_model.h
        todo_list_model.cpp
        subagent_model.h
        subagent_model.cpp
    QML_FILES
        Conversation.qml
        TranscriptPage.qml
        ToolGroup.qml
)
add_library(daemon-app::conversation ALIAS da_conversation)

target_link_libraries(da_conversation
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
        daemon-app::warnings
)
