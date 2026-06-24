qt_add_qml_module(da_sessionslist
    STATIC
    URI DaemonApp.SessionsList
    VERSION 1.0
    SOURCES
        sessions_list_model.h
        sessions_list_model.cpp
    QML_FILES
        SessionsList.qml
)
add_library(daemon-app::sessionslist ALIAS da_sessionslist)

target_link_libraries(da_sessionslist
    PRIVATE
        Qt6::Quick
        daemon-app::domain
        daemon-app::persistence
        daemon-app::theme
        daemon-app::controls
        daemon-app::presentation
        daemon-app::warnings
)
