qt_add_qml_module(da_transcript
    STATIC
    URI DaemonApp.Transcript
    VERSION 1.0
    QML_FILES
        Transcript.qml
        SessionChrome.qml
)
add_library(daemon-app::transcript ALIAS da_transcript)

target_link_libraries(da_transcript PRIVATE Qt6::Quick daemon-app::theme daemon-app::settings daemon-app::controls daemon-app::blockeditor daemon-app::editor daemon-app::turn daemon-app::warnings)

# The BlockEditor's CodeBlock still imports org.kde.syntaxhighlighting; the plain-
# text mode now uses DaemonApp.Editor. Point qmllint at the build-tree qml dir.
set_property(TARGET da_transcript APPEND PROPERTY QT_QML_IMPORT_PATH "${CMAKE_BINARY_DIR}/qml")
