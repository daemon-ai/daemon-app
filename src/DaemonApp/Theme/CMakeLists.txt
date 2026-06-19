# Singleton properties must be set before the module is declared.
set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(FontIcons.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# Bundle the display + icon fonts as module resources. They live under assets/
# so we alias them to "fonts/<name>" relative to the module's resource root,
# letting FontIcons.qml load them with a relative URL.
set(theme_fonts
    ${CMAKE_SOURCE_DIR}/assets/fonts/InterVariable.ttf
    ${CMAKE_SOURCE_DIR}/assets/fonts/fa-solid-900.ttf
    ${CMAKE_SOURCE_DIR}/assets/fonts/fa-brands-400.ttf
    ${CMAKE_SOURCE_DIR}/assets/fonts/material-symbols-outlined.ttf
)
foreach(_font ${theme_fonts})
    get_filename_component(_font_name ${_font} NAME)
    set_source_files_properties(${_font} PROPERTIES QT_RESOURCE_ALIAS fonts/${_font_name})
endforeach()

qt_add_qml_module(da_theme
    STATIC
    URI DaemonApp.Theme
    VERSION 1.0
    QML_FILES
        Theme.qml
        FontIcons.qml
    RESOURCES
        ${theme_fonts}
)
add_library(daemon-app::theme ALIAS da_theme)

target_link_libraries(da_theme PRIVATE Qt6::Quick daemon-app::warnings)
