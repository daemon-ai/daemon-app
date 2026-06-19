qt_add_qml_module(da_conversationslist
    STATIC
    URI DaemonApp.ConversationsList
    VERSION 1.0
    SOURCES
        conversations_list_model.h
        conversations_list_model.cpp
    QML_FILES
        ConversationsList.qml
)
add_library(daemon-app::conversationslist ALIAS da_conversationslist)

target_link_libraries(da_conversationslist
    PRIVATE
        Qt6::Quick
        daemon-app::domain
        daemon-app::persistence
        daemon-app::theme
        daemon-app::controls
        daemon-app::warnings
)
