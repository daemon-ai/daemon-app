qt_add_qml_module(da_transcript
    STATIC
    URI DaemonApp.Transcript
    VERSION 1.0
    QML_FILES
        Transcript.qml
)
add_library(daemon-app::transcript ALIAS da_transcript)

target_link_libraries(da_transcript PRIVATE Qt6::Quick daemon-app::theme daemon-app::warnings)
