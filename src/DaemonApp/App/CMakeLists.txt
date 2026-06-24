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
        CommandPalette.qml
)

target_link_libraries(daemon-app
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        daemon-app::warnings
        # Core services
        daemon-app::persistence
        daemon-app::platform
        daemon-app::command
        daemon-app::export
        daemon-app::settings_store
        daemon-app::connection
        daemon-app::fs
        daemon-app::memory
        daemon-app::nav
        daemon-app::firstrun
        daemon-app::config
        daemon-app::uimodels
        daemon-app::models
        daemon-app::accounts
        daemon-app::profiles
        daemon-app::fleet
        daemon-app::automation
        daemon-app::session
        # Feature QML modules: backing libs (C++ types) + their STATIC plugins.
        # The plugins must be linked explicitly (shared Qt build); main.cpp also
        # references them via Q_IMPORT_QML_PLUGIN so they aren't dead-stripped.
        daemon-app::presentation
        daemon-app::theme_core
        daemon-app::composer_session
        daemon-app::turn
        daemon-app::status_model
        daemon-app::theme
        daemon-app::controls
        daemon-app::blockeditor
        daemon-app::sidebar
        daemon-app::conversationslist
        daemon-app::conversation
        daemon-app::tabs
        daemon-app::files
        daemon-app::editor
        daemon-app::transcript
        daemon-app::composer
        daemon-app::settings
        daemon-app::pages
        daemon-app::memory_views
        daemon-app::statusbar
        da_presentationplugin
        da_theme_coreplugin
        da_composer_sessionplugin
        da_turnplugin
        da_status_modelplugin
        da_themeplugin
        da_controlsplugin
        da_blockeditorplugin
        da_sidebarplugin
        da_conversationslistplugin
        da_conversationplugin
        da_tabsplugin
        da_filesplugin
        da_editorplugin
        da_transcriptplugin
        da_composerplugin
        da_settingsplugin
        da_pagesplugin
        da_memory_viewsplugin
        da_statusbarplugin
        # KSyntaxHighlighting C++ engine. Linking it here builds the framework as a
        # dependency, records its shared lib on the executable rpath, and ensures
        # the QML plugin (kquicksyntaxhighlightingplugin) is built alongside.
        KF6SyntaxHighlighting
        # MicroTeX: application.cpp calls tex::LaTeX::init()/release() at startup.
        LaTeX
)

# The runtime resource dir (fonts + XML) MicroTeX's tex::LaTeX::init() reads.
target_compile_definitions(daemon-app
    PRIVATE MICROTEX_RES_DIR="${MICROTEX_RES_DIR}"
)

# Build dependency on the KSyntaxHighlighting QML plugin so its module is emitted
# under QT_QML_OUTPUT_DIRECTORY before we run; main.cpp adds that dir to the
# engine import path via DAEMON_APP_BUILD_QML_DIR.
add_dependencies(daemon-app kquicksyntaxhighlightingplugin)
target_compile_definitions(daemon-app
    PRIVATE DAEMON_APP_BUILD_QML_DIR="${CMAKE_BINARY_DIR}/qml"
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
