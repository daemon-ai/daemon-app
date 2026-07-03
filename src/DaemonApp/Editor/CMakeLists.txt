# DaemonApp.Editor - the QML-native code editor. A plain-C++ engine (de_core:
# block-based line buffer, document + undo, incremental KSyntaxHighlighting) plus
# the QML module exposing CodeEditorController + the line model and the editor
# view. KTextEditor/Lite XL are study references only; nothing is linked from them.

# --- de_core: the editor engine (Quick-free) --------------------------------
add_library(de_core STATIC
    engine/line_buffer.cpp
    engine/line_buffer.h
    engine/text_document.cpp
    engine/text_document.h
    engine/code_highlighter.cpp
    engine/code_highlighter.h
)
add_library(daemon-app::editor_core ALIAS de_core)

set_target_properties(de_core PROPERTIES AUTOMOC ON)
target_include_directories(de_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(de_core
    PUBLIC
        Qt6::Core
        Qt6::Gui
    PRIVATE
        daemon-app::warnings
        daemon-app::numeric
)
if(NOT DAEMON_APP_WASM)
    target_link_libraries(de_core PUBLIC KF6SyntaxHighlighting)
endif()

# --- da_editor: QML module (controller + line model + the view) -------------
qt_add_qml_module(da_editor
    STATIC
    URI DaemonApp.Editor
    VERSION 1.0
    SOURCES
        app/code_editor_controller.cpp
        app/code_editor_controller.h
        app/line_model.cpp
        app/line_model.h
    QML_FILES
        qml/CodeEditor.qml
)
add_library(daemon-app::editor ALIAS da_editor)

target_include_directories(da_editor PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
# The generated QML type registration includes the QML_ELEMENT headers by
# basename (<code_editor_controller.h>, <line_model.h>), so the app/ dir must be
# on the include path (same pattern as DaemonApp.BlockEditor).
target_include_directories(da_editor PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/app"
    "${CMAKE_CURRENT_SOURCE_DIR}/engine")

target_link_libraries(da_editor
    PRIVATE
        Qt6::Quick
        de_core
        daemon-app::theme
        daemon-app::controls
        daemon-app::warnings
        daemon-app::numeric
)
