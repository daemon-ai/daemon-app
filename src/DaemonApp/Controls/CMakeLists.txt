qt_add_qml_module(da_controls
    STATIC
    URI DaemonApp.Controls
    VERSION 1.0
    QML_FILES
        Glyph.qml
        IconButton.qml
        TextField.qml
        CheckBox.qml
        TextButton.qml
        Tooltip.qml
        ScrollBar.qml
        ThemeSwatch.qml
)
add_library(daemon-app::controls ALIAS da_controls)

target_link_libraries(da_controls
    PRIVATE
        Qt6::Quick
        daemon-app::theme
        daemon-app::warnings
)
