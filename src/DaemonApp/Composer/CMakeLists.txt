qt_add_qml_module(da_composer
    STATIC
    URI DaemonApp.Composer
    VERSION 1.0
    SOURCES
        # Browser (wasm) attachment upload: local file -> node workspace over IFsService, then a
        # chip pointing at the uploaded workspace path (W7). Registered as a QML type in this
        # module; used only on the wasm path (see Composer.qml's Qt.platform.os gate).
        composer_upload_controller.h
        composer_upload_controller.cpp
    QML_FILES
        Composer.qml
        ComposerControls.qml
        ComposerMenu.qml
        ModelPill.qml
        ModelPickerOverlay.qml
        CompletionPopover.qml
        AttachmentChip.qml
        StatusStack.qml
        QueuePanel.qml
        TodoList.qml
        SubagentList.qml
        SessionSettingsPopover.qml
        CheckpointsPopover.qml
)
add_library(daemon-app::composer ALIAS da_composer)

target_link_libraries(da_composer PRIVATE Qt6::Quick daemon-app::theme daemon-app::controls daemon-app::composer_session daemon-app::files daemon-app::fs daemon-app::platform daemon-app::warnings)
