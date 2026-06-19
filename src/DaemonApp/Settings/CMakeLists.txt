qt_add_qml_module(da_settings
    STATIC
    URI DaemonApp.Settings
    VERSION 1.0
    SOURCES
        uisettings.h
        uisettings.cpp
    QML_FILES
        SettingsMenu.qml
        FontStyleButton.qml
        OptionRow.qml
)
add_library(daemon-app::settings ALIAS da_settings)

target_link_libraries(da_settings
    PRIVATE
        Qt6::Quick
        daemon-app::theme
        daemon-app::controls
        daemon-app::warnings
)
