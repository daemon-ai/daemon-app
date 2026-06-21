qt_add_qml_module(da_transcript
    STATIC
    URI DaemonApp.Transcript
    VERSION 1.0
    QML_FILES
        Transcript.qml
        ConversationChrome.qml
)
add_library(daemon-app::transcript ALIAS da_transcript)

target_link_libraries(da_transcript PRIVATE Qt6::Quick daemon-app::theme daemon-app::settings daemon-app::blockeditor daemon-app::turn daemon-app::warnings)

# Transcript.qml imports the external org.kde.syntaxhighlighting module (plain-text
# highlighting); point qmllint at the build-tree dir where its plugin is emitted.
set_property(TARGET da_transcript APPEND PROPERTY QT_QML_IMPORT_PATH "${CMAKE_BINARY_DIR}/qml")
