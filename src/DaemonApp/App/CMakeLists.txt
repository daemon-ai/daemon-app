# The App module is the application executable and the DaemonApp.App QML module.
# (Target name 'daemon-app' intentionally differs from the URI 'DaemonApp.App'.)

qt_add_executable(daemon-app
    main.cpp
    application.h
    application.cpp
)

qt_add_qml_module(daemon-app
    URI DaemonApp.App
    VERSION 1.0
    QML_FILES
        Main.qml
)

target_link_libraries(daemon-app
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        daemon-app::warnings
        # Core services
        daemon-app::persistence
        daemon-app::platform
        # Feature QML modules: backing libs (C++ types) + their STATIC plugins.
        # The plugins must be linked explicitly (shared Qt build); main.cpp also
        # references them via Q_IMPORT_QML_PLUGIN so they aren't dead-stripped.
        daemon-app::theme
        daemon-app::controls
        daemon-app::blockeditor
        daemon-app::sidebar
        daemon-app::conversationslist
        daemon-app::conversation
        daemon-app::transcript
        daemon-app::composer
        daemon-app::settings
        daemon-app::statusbar
        da_themeplugin
        da_controlsplugin
        da_blockeditorplugin
        da_sidebarplugin
        da_conversationslistplugin
        da_conversationplugin
        da_transcriptplugin
        da_composerplugin
        da_settingsplugin
        da_statusbarplugin
)

# On desktop the QApplication base lives in QtWidgets (for the tray).
if(NOT DAEMON_APP_MOBILE)
    target_link_libraries(daemon-app PRIVATE Qt6::Widgets)
endif()

# Install the single self-contained binary. wrapQtAppsHook (flake) wraps it with
# an isolated Qt environment, so it runs independently of the host's Qt.
include(GNUInstallDirs)
install(TARGETS daemon-app
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
