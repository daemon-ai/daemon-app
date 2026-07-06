# App-level manager/settings pages, mounted as singleton tab content by
# Session.qml (the GUI's tab strip). One QML module holds the page shell,
# reusable chrome (PageHeader / NavRail / SettingRow / SectionLabel), and each
# page/section; new pages are added here as they are implemented.
qt_add_qml_module(da_pages
    STATIC
    URI DaemonApp.Pages
    VERSION 1.0
    SOURCES
        routing_manager_controller.h
        routing_manager_controller.cpp
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
        AgentInferencePicker.qml
        AgentTypePicker.qml
        ConnectionSection.qml
        NotificationsSection.qml
        ModelSettingsSection.qml
        ChatSettingsSection.qml
        ToolsSettingsSection.qml
        SafetySettingsSection.qml
        MemorySettingsSection.qml
        WorkspaceSection.qml
        VoiceSection.qml
        AdvancedSection.qml
        ArchivedSection.qml
        ModelsPage.qml
        ModelsDiscover.qml
        DiscoverDialog.qml
        QuantPickerPopup.qml
        DownloadsPanel.qml
        ModelsDownloads.qml
        ModelsInstalled.qml
        ModelsProviders.qml
        AccountsPage.qml
        AddAccountWizard.qml
        AuthFlowSheet.qml
        ProfilesPage.qml
        ProfileEditor.qml
        CuratorRow.qml
        DashboardPage.qml
        FleetPage.qml
        ChannelsPage.qml
        SessionsPage.qml
        ApprovalsPage.qml
        RoutingPage.qml
        RouteDialog.qml
        RoutingTopology.qml
        CronPage.qml
        CronJobDialog.qml
        FirstRunGate.qml
        FilePage.qml
        UsersAccessPage.qml
)
add_library(daemon-app::pages ALIAS da_pages)

target_link_libraries(da_pages
    PRIVATE
        Qt6::Quick
        # FilePage hosts the code editor (DaemonApp.Editor) and the rich markdown
        # block editor (DaemonApp.BlockEditor).
        daemon-app::editor
        daemon-app::blockeditor
        # RoutingManagerController bridges the DaemonNet routing projections/mutations.
        daemon-app::daemonnet
        daemon-app::domain
        daemon-app::uimodels
        # RoutingTopology imports the reusable DaemonApp.Graph kit module.
        daemon-app::graph
        daemon-app::warnings
)
