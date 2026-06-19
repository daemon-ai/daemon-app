qt_add_qml_module(da_conversation
    STATIC
    URI DaemonApp.Conversation
    VERSION 1.0
    SOURCES
        conversation_controller.h
        conversation_controller.cpp
    QML_FILES
        Conversation.qml
)
add_library(daemon-app::conversation ALIAS da_conversation)

target_link_libraries(da_conversation
    PRIVATE
        Qt6::Quick
        daemon-app::domain
        daemon-app::persistence
        daemon-app::theme
        daemon-app::transcript
        daemon-app::composer
        daemon-app::warnings
)
