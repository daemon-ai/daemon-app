qt_add_qml_module(da_statusbar
    STATIC
    URI DaemonApp.StatusBar
    VERSION 1.0
    QML_FILES
        StatusBar.qml
        StatusBarItem.qml
        GatewayMenu.qml
)
add_library(daemon-app::statusbar ALIAS da_statusbar)

target_link_libraries(da_statusbar
    PRIVATE
        Qt6::Quick
        daemon-app::theme
        daemon-app::warnings
)
