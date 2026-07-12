qt_add_qml_module(da_sidebar
    STATIC
    URI DaemonApp.Sidebar
    VERSION 1.0
    SOURCES
        sidebar_model.h
        sidebar_model.cpp
        integrations_tree_model.h
        integrations_tree_model.cpp
        account_management_controller.h
        account_management_controller.cpp
    QML_FILES
        NewAgentDialog.qml
        Sidebar.qml
        IntegrationsTree.qml
        AccountFormDialog.qml
)
add_library(daemon-app::sidebar ALIAS da_sidebar)

target_link_libraries(da_sidebar
    PRIVATE
        Qt6::Quick
        daemon-app::domain
        daemon-app::persistence
        daemon-app::profiles
        daemon-app::transports
        daemon-app::uimodels
        daemon-app::theme
        daemon-app::controls
        daemon-app::presentation
        # AD (1a.3): the integrations tree projects the mirror (accounts/adapters/convs/persons).
        daemon-app::mirror_ingestor
        daemon-app::warnings
)
