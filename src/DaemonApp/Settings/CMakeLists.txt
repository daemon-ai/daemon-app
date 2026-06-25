# Singleton properties must be set before the module is declared.
set_source_files_properties(LayoutState.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

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
        LayoutState.qml
)
add_library(daemon-app::settings ALIAS da_settings)

target_link_libraries(da_settings
    PRIVATE
        Qt6::Quick
        daemon-app::theme
        daemon-app::theme_core
        daemon-app::controls
        daemon-app::i18n
        daemon-app::warnings
)
