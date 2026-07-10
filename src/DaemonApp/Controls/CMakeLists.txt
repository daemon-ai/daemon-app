qt_add_qml_module(da_controls
    STATIC
    URI DaemonApp.Controls
    VERSION 1.0
    SOURCES
        # [integrations wire v38] A3 generic auth component: Kit.QrCode (QQuickPaintedItem over the
        # shared qr::QrMatrix). Lands in the Kit, not in feature QML, per the Daemon Kit rule.
        qr_code_item.h
        qr_code_item.cpp
    QML_FILES
        Glyph.qml
        IconButton.qml
        TextField.qml
        TextArea.qml
        SpinBox.qml
        CheckBox.qml
        TextButton.qml
        Tooltip.qml
        ScrollBar.qml
        ThemeSwatch.qml
        Switch.qml
        Dropdown.qml
        Menu.qml
        MenuItem.qml
        MenuSeparator.qml
        EditMenu.qml
        Dialog.qml
        TreeRow.qml
        ProgressBar.qml
        Toast.qml
        Chip.qml
        # [wave2:app-engines] approval origin engine chip (C5)
        EngineOriginChip.qml
        # [wave2:backlog-app] TOOL-8 pending-count pill badge
        Badge.qml
        # [acct-mgmt] node-described AccountSettingsSchema form (join/create room extras)
        SettingsSchemaForm.qml
)
add_library(daemon-app::controls ALIAS da_controls)

# The Kit.QrCode registration file includes "qr_code_item.h" by basename.
target_include_directories(da_controls PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(da_controls
    PRIVATE
        Qt6::Quick
        Qt6::Gui
        daemon-app::theme
        daemon-app::qr
        daemon-app::warnings
)
