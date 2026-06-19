qt_add_qml_module(da_composer
    STATIC
    URI DaemonApp.Composer
    VERSION 1.0
    QML_FILES
        Composer.qml
)
add_library(daemon-app::composer ALIAS da_composer)

target_link_libraries(da_composer PRIVATE Qt6::Quick daemon-app::theme daemon-app::controls daemon-app::warnings)
