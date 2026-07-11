find_package(Qt6 6.11 REQUIRED COMPONENTS QuickTest Qml Quick)

# tst_theme: QtQuickTest over the Theme singleton. Asserts setTheme switches and
# that each of Light/Dark/Sepia yields the expected token hexes (guards the
# id/property collision regression). QUICK_TEST_SOURCE_DIR points the runner at
# the tst_*.qml in this dir.
qt_add_executable(tst_theme tst_theme_main.cpp)
target_link_libraries(tst_theme
    PRIVATE
        Qt6::QuickTest
        Qt6::Qml
        Qt6::Quick
        da_theme
        da_themeplugin
)
target_compile_definitions(tst_theme
    PRIVATE QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/theme"
)

add_test(NAME tst_theme COMMAND tst_theme)
set_tests_properties(tst_theme PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software"
)

# tst_composer: QtQuickTest over the ported message composer. Drives send/queue/
# history/draft behaviors through the composer's public surface. Composer pulls
# in Controls (QtQuick.Controls), so a Controls style must be available offscreen
# (Basic is always present).
find_package(Qt6 6.11 REQUIRED COMPONENTS QuickControls2)
qt_add_executable(tst_composer tst_composer_main.cpp)
target_link_libraries(tst_composer
    PRIVATE
        Qt6::QuickTest
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        da_theme
        da_themeplugin
        da_controls
        da_controlsplugin
        da_composer_session
        da_composer_sessionplugin
        da_composer
        da_composerplugin
)
target_compile_definitions(tst_composer
    PRIVATE
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/composer"
        TEST_QML_IMPORT_PATH="$ENV{QML2_IMPORT_PATH}"
)

add_test(NAME tst_composer COMMAND tst_composer)
set_tests_properties(tst_composer PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QUICK_CONTROLS_STYLE=Basic"
)

# tst_chip: QtQuickTest over the Kit Chip control. Asserts tone -> text/border
# tint, icon/close affordance visibility, clicked()/closeRequested() emission,
# and the non-interactive default. Chip pulls in QtQuick.Controls (Label/
# ToolTip), so the Basic style must be available offscreen.
qt_add_executable(tst_chip tst_chip_main.cpp)
target_link_libraries(tst_chip
    PRIVATE
        Qt6::QuickTest
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        da_theme
        da_themeplugin
        da_controls
        da_controlsplugin
)
target_compile_definitions(tst_chip
    PRIVATE
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/controls"
        TEST_QML_IMPORT_PATH="$ENV{QML2_IMPORT_PATH}"
)

add_test(NAME tst_chip COMMAND tst_chip)
set_tests_properties(tst_chip PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QUICK_CONTROLS_STYLE=Basic"
)

# [wave2:backlog-app] tst_badge (TOOL-8): the Kit.Badge count pill — hidden at 0, visible when
# positive, tone -> fill tint, and the >max clamp. Pulls in QtQuick.Controls (Label), so the Basic
# style must be available offscreen.
qt_add_executable(tst_badge tst_badge_main.cpp)
target_link_libraries(tst_badge
    PRIVATE
        Qt6::QuickTest
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        da_theme
        da_themeplugin
        da_controls
        da_controlsplugin
)
target_compile_definitions(tst_badge
    PRIVATE
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/badge"
        TEST_QML_IMPORT_PATH="$ENV{QML2_IMPORT_PATH}"
)

add_test(NAME tst_badge COMMAND tst_badge)
set_tests_properties(tst_badge PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QUICK_CONTROLS_STYLE=Basic"
)

# [wave2:app-engines] tst_engine_origin_chip (C5): the approval origin chip's no-noise contract —
# with no EngineIdentity facade in context it renders nothing regardless of sessionId (also guards
# the component imports/loads cleanly).
qt_add_executable(tst_engine_origin_chip tst_engine_origin_chip_main.cpp)
target_link_libraries(tst_engine_origin_chip
    PRIVATE
        Qt6::QuickTest
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        da_theme
        da_themeplugin
        da_controls
        da_controlsplugin
)
target_compile_definitions(tst_engine_origin_chip
    PRIVATE
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/engine_chip"
        TEST_QML_IMPORT_PATH="$ENV{QML2_IMPORT_PATH}"
)

add_test(NAME tst_engine_origin_chip COMMAND tst_engine_origin_chip)
set_tests_properties(tst_engine_origin_chip PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QUICK_CONTROLS_STYLE=Basic"
)

# tst_tabs: QtQuickTest over the pane tab strip. Drives a real TabBar + StackLayout
# off the shared TabModel - add/remove/switch, find-or-create re-activation, the
# "+"/settings affordances, and click-to-activate. Pulls in Controls/Layouts, so a
# Controls style must be available offscreen (Basic is always present).
qt_add_executable(tst_tabs tst_tabs_main.cpp)
target_link_libraries(tst_tabs
    PRIVATE
        Qt6::QuickTest
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        da_theme
        da_themeplugin
        da_controls
        da_controlsplugin
        da_tabs
        da_tabsplugin
)
target_compile_definitions(tst_tabs
    PRIVATE
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tabs"
        TEST_QML_IMPORT_PATH="$ENV{QML2_IMPORT_PATH}"
)

add_test(NAME tst_tabs COMMAND tst_tabs)
set_tests_properties(tst_tabs PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QUICK_CONTROLS_STYLE=Basic"
)

# tst_downloads_panel: QtQuickTest over the REAL DownloadsPanel (DaemonApp.Pages), driven by a
# MockModelCatalog injected as the app-global ModelCatalog context property. Guards the
# rows-render / self-hide / activeOnly-terminal behaviors, the multi-file "file x/y" label, and
# the "Use this model" installedIdFor resolution. The Pages module is static, so its plugin (and
# its transitive backing libs) link explicitly, like the GUI executable does.
qt_add_executable(tst_downloads_panel tst_downloads_panel_main.cpp)
target_link_libraries(tst_downloads_panel
    PRIVATE
        Qt6::QuickTest
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        daemon-app::models
        da_theme
        da_themeplugin
        da_controls
        da_controlsplugin
        da_pages
        da_pagesplugin
)
target_compile_definitions(tst_downloads_panel
    PRIVATE
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/downloads_panel"
        TEST_QML_IMPORT_PATH="$ENV{QML2_IMPORT_PATH}"
)

add_test(NAME tst_downloads_panel COMMAND tst_downloads_panel)
set_tests_properties(tst_downloads_panel PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QUICK_CONTROLS_STYLE=Basic"
)

# [integrations wire v38] tst_integrations (work package AC): the REAL IntegrationsTree
# (DaemonApp.Sidebar), driven by the in-repo mock Transports / Persons seams injected as the
# app-global context properties + a serviceless AuthFlow (so the mounted AuthFlowSheet never binds
# a redirect sink offscreen). Guards populate-from-seams, conversation activation reaching the
# component signal, and the add-account -> AuthFlowSheet handoff. The Sidebar + Pages modules are
# static, so their plugins (and transitive backing libs) link explicitly, like the GUI executable.
qt_add_executable(tst_integrations tst_integrations_main.cpp)
target_link_libraries(tst_integrations
    PRIVATE
        Qt6::QuickTest
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        da_theme
        da_themeplugin
        da_controls
        da_controlsplugin
        da_pages
        da_pagesplugin
        da_sidebar
        da_sidebarplugin
        daemon-app::transports
        daemon-app::auth
)
target_compile_definitions(tst_integrations
    PRIVATE
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/integrations"
        TEST_QML_IMPORT_PATH="$ENV{QML2_IMPORT_PATH}"
)

add_test(NAME tst_integrations COMMAND tst_integrations)
set_tests_properties(tst_integrations PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QUICK_CONTROLS_STYLE=Basic"
)

# tst_sidebar: the REAL Sidebar.qml composition. Injects the mock seams (the mock fleet seed backs
# the session store) to prove the fleet SidebarModel renders no transport section (the legacy
# transports-tree sidebar path was deleted in M3), while the IntegrationsTree is mounted + populated
# and its conversation activation propagates to the Sidebar's conversationActivated signal.
qt_add_executable(tst_sidebar tst_sidebar_main.cpp)
target_link_libraries(tst_sidebar
    PRIVATE
        Qt6::QuickTest
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        da_theme
        da_themeplugin
        da_controls
        da_controlsplugin
        da_presentation
        da_presentationplugin
        da_pages
        da_pagesplugin
        da_sidebar
        da_sidebarplugin
        daemon-app::transports
        daemon-app::auth
        daemon-app::persistence
        daemon-app::profiles
        daemon-app::daemonnet
        daemon-app::fleet
        daemon-app::uimodels
        # AD: the scenario-seeded mirror store replaces the deleted InMemory fixture.
        daemon-app::daemon_client
        daemon-app::mirror_ingestor
)
target_compile_definitions(tst_sidebar
    PRIVATE
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/sidebar"
        TEST_QML_IMPORT_PATH="$ENV{QML2_IMPORT_PATH}"
)

add_test(NAME tst_sidebar COMMAND tst_sidebar)
set_tests_properties(tst_sidebar PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QUICK_CONTROLS_STYLE=Basic"
)
