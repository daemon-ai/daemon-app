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
        ForeignBackendPicker.qml
        AgentSetupForm.qml
        # [wave2:app-engines] foreign-agent register/remove surface (C1)
        AgentsSettingsSection.qml
        RegisterAgentDialog.qml
        ConnectionSection.qml
        GatewaySection.qml  # [wave2:app-gateway] Phase F: node OpenAI-gateway settings + health
        NotificationsSection.qml
        ModelSettingsSection.qml
        ChatSettingsSection.qml
        ToolsSettingsSection.qml
        SafetySettingsSection.qml
        DelegationLimitsSection.qml  # [wave2:app-delegation] F7/DEL-7
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
        CustomProvidersSection.qml
        AccountsPage.qml
        AddAccountWizard.qml
        AuthFlowSheet.qml
        ProfilesPage.qml
        ProfileEditor.qml
        CuratorRow.qml
        DashboardPage.qml
        FleetPage.qml
        ChannelsPage.qml
        # [acct-mgmt] room lifecycle dialogs (Phase B)
        JoinRoomDialog.qml
        NewRoomDialog.qml
        # [acct-mgmt] contacts / roster dialogs (Phase D)
        ContactProfileDialog.qml
        FindPeopleDialog.qml
        SessionsPage.qml
        ApprovalsPage.qml
        RoutingPage.qml
        RouteDialog.qml
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
        # RoutingManagerController reads the mirror store projections + the IRoutingActions seam.
        daemon-app::daemonnet
        daemon-app::domain
        daemon-app::uimodels
        daemon-app::warnings
)

# mirror A6 (spec 09 wave M3): the routing manager reads the mirror store's route_pins/rooms/
# sessions projections where the substrate is built; substrate-less stacks render an empty manager.
if(DAEMON_APP_HAVE_MIRROR_SUBSTRATE)
    target_link_libraries(da_pages PRIVATE
        daemon-app::mirror_substrate
        daemon-app::mirror_ingestor
    )
    target_compile_definitions(da_pages PRIVATE DAEMON_APP_HAVE_MIRROR_SUBSTRATE=1)
endif()
