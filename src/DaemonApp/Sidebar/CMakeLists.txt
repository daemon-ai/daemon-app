qt_add_qml_module(da_sidebar
    STATIC
    URI DaemonApp.Sidebar
    VERSION 1.0
    SOURCES
        sidebar_model.h
        sidebar_model.cpp
    QML_FILES
        Sidebar.qml
)
add_library(daemon-app::sidebar ALIAS da_sidebar)

target_link_libraries(da_sidebar
    PRIVATE
        Qt6::Quick
        daemon-app::domain
        daemon-app::persistence
        daemon-app::theme
        daemon-app::controls
        daemon-app::presentation
        daemon-app::warnings
)
