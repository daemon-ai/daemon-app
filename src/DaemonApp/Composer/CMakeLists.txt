qt_add_qml_module(da_composer
    STATIC
    URI DaemonApp.Composer
    VERSION 1.0
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
)
add_library(daemon-app::composer ALIAS da_composer)

target_link_libraries(da_composer PRIVATE Qt6::Quick daemon-app::theme daemon-app::controls daemon-app::composer_session daemon-app::warnings)
