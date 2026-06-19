# Singleton property must be set before the module is declared.
set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(da_theme
    STATIC
    URI DaemonApp.Theme
    VERSION 1.0
    QML_FILES Theme.qml
)
add_library(daemon-app::theme ALIAS da_theme)

target_link_libraries(da_theme PRIVATE Qt6::Quick daemon-app::warnings)
