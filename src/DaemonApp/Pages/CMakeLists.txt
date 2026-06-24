# App-level manager/settings pages, mounted as singleton tab content by
# Conversation.qml (the GUI's tab strip). One QML module holds the page shell,
# reusable chrome (PageHeader / NavRail / SettingRow / SectionLabel), and each
# page/section; new pages are added here as they are implemented.
qt_add_qml_module(da_pages
    STATIC
    URI DaemonApp.Pages
    VERSION 1.0
    QML_FILES
        SettingsPage.qml
        PageHeader.qml
        NavRail.qml
        SectionLabel.qml
        SettingRow.qml
        Placeholder.qml
        ComboRow.qml
        AppearanceSection.qml
        AboutSection.qml
        ConnectionPicker.qml
        ConnectionSection.qml
        NotificationsSection.qml
        ModelSettingsSection.qml
        ChatSettingsSection.qml
        SafetySettingsSection.qml
        MemorySettingsSection.qml
        WorkspaceSection.qml
        VoiceSection.qml
        AdvancedSection.qml
        ArchivedSection.qml
        ModelsPage.qml
        ModelsDiscover.qml
        ModelsDownloads.qml
        ModelsInstalled.qml
        ModelsProviders.qml
        AccountsPage.qml
        AddAccountWizard.qml
        ProfilesPage.qml
        ProfileEditor.qml
        CuratorRow.qml
        DashboardPage.qml
        FleetPage.qml
        SessionsPage.qml
        ApprovalsPage.qml
        RoutingPage.qml
        CronPage.qml
        CronJobDialog.qml
        FirstRunGate.qml
        FilePage.qml
)
add_library(daemon-app::pages ALIAS da_pages)

target_link_libraries(da_pages
    PRIVATE
        Qt6::Quick
        # FilePage hosts the code editor (DaemonApp.Editor) and the rich markdown
        # block editor (DaemonApp.BlockEditor).
        daemon-app::editor
        daemon-app::blockeditor
        daemon-app::warnings
)
