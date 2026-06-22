qt_add_qml_module(da_tabs
    STATIC
    URI DaemonApp.Tabs
    VERSION 1.0
    SOURCES
        tab_model.h
        tab_model.cpp
    QML_FILES
        TabBar.qml
)
add_library(daemon-app::tabs ALIAS da_tabs)

target_link_libraries(da_tabs
    PRIVATE
        Qt6::Quick
        daemon-app::theme
        daemon-app::controls
        daemon-app::warnings
)
