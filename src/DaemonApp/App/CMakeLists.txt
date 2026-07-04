# The App module is the application executable and the DaemonApp.App QML module.
# (Target name 'daemon-app' intentionally differs from the URI 'DaemonApp.App'.)

qt_add_executable(daemon-app
    main.cpp
    application.h
    application.cpp
)

# main.cpp includes the generated daemon_app_version.h; depend on the target that refreshes it.
add_dependencies(daemon-app daemon_app_version_header)

qt_add_qml_module(daemon-app
    URI DaemonApp.App
    VERSION 1.0
    QML_FILES
        Main.qml
        CommandPalette.qml
        RightPanel.qml
)

target_link_libraries(daemon-app
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        daemon-app::warnings
        daemon-app::version
        # Core services
        daemon-app::i18n
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
        daemon-app::daemon_client
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
        daemon-app::graph
        daemon-app::blockeditor
        daemon-app::sidebar
        daemon-app::sessionslist
        daemon-app::session_view
        daemon-app::tabs
        daemon-app::terminal
        daemon-app::files
        daemon-app::participants
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
        da_graphplugin
        da_blockeditorplugin
        da_sidebarplugin
        da_sessionslistplugin
        da_session_viewplugin
        da_tabsplugin
        da_terminalplugin
        da_filesplugin
        da_participantsplugin
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
# Desktop/mobile read it straight from the (Nix store) source tree. The browser
# cannot read host paths, so the wasm link packs the directory into the
# emscripten .data preload bundle and init() reads it back from the same
# fixed MEMFS mount point.
if(DAEMON_APP_WASM)
    # Size flags already evaluated and NOT carried, for the record:
    #  * --closure 1 halves the JS glue but breaks qtloader.js at boot
    #    (TypeError on 'asyncifyEnabled': the loader pokes Module properties
    #    that closure renames away).
    #  * -sEVAL_CTORS=1 bails at the first environment-reading import
    #    (environ_sizes_get) and saved <1 KB.
    target_link_options(daemon-app
        PRIVATE "SHELL:--preload-file ${MICROTEX_RES_DIR}@/microtex-res"
        # IDBFS backend (Browser Persistence Polish / W2): the shell's preRun mounts the AppData dir
        # (daemon_cache*.db) as IDBFS and FS.syncfs(true)s it from IndexedDB before main() runs, so
        # DaemonCacheStore opens the persisted db with no C++ ordering changes; scheduleIdbfsSync()
        # writes back. Linking -lidbfs.js pulls the backend into the runtime.
        PRIVATE "SHELL:-lidbfs.js"
    )
    target_compile_definitions(daemon-app
        PRIVATE MICROTEX_RES_DIR="/microtex-res"
    )
else()
    target_compile_definitions(daemon-app
        PRIVATE MICROTEX_RES_DIR="${MICROTEX_RES_DIR}"
    )
endif()

# qtloader's `qt.environment` copies entries into emscripten's ENV at preRun, but only when
# the ENV runtime object is exported from the module (qtloader throws otherwise, naming this
# exact property). Export it so DAEMON_APP_* env-gated behavior (service-mode override, the
# headless probes like DAEMON_APP_WAIT_READY) is reachable in the browser. Qt folds the value
# into the emscripten EXPORTED_RUNTIME_METHODS link option; the installed artifact set is
# unchanged, so the install(FILES ...) block below needs no adjustment.
# The shell's preRun IDBFS mount (W2) runs OUTSIDE the wasm module (it is defined in
# daemon-app.html and handed to emscripten via qtLoad's config.preRun), so it reaches the runtime
# only through the module instance: it calls instance.FS.mount(instance.FS.filesystems.IDBFS, ...)
# and gates main() on instance.addRunDependency/removeRunDependency around FS.syncfs(true). FS is
# already in Qt's default exported set; addRunDependency/removeRunDependency are not, so export
# them here (comma-separated - a ';' list would break the emscripten flag). "ENV" stays for the
# qt.environment seam. (scheduleIdbfsSync()'s EM_ASM runs inside the module, where FS is already in
# scope, so it needs no export.)
if(DAEMON_APP_WASM)
    set_target_properties(daemon-app PROPERTIES
        QT_WASM_EXTRA_EXPORTED_METHODS "ENV,addRunDependency,removeRunDependency")
endif()

# Build dependency on the KSyntaxHighlighting QML plugin so its module is emitted
# under QT_QML_OUTPUT_DIRECTORY before we run; main.cpp adds that dir to the
# engine import path via DAEMON_APP_BUILD_QML_DIR.
add_dependencies(daemon-app kquicksyntaxhighlightingplugin)
target_compile_definitions(daemon-app
    PRIVATE DAEMON_APP_BUILD_QML_DIR="${CMAKE_BINARY_DIR}/qml"
)

# On desktop the QApplication base lives in QtWidgets (for the tray). Mobile
# and the browser build run QGuiApplication (see main.cpp) and never link it.
if(NOT DAEMON_APP_MOBILE AND NOT DAEMON_APP_WASM)
    target_link_libraries(daemon-app PRIVATE Qt6::Widgets)
endif()

include(GNUInstallDirs)
if(DAEMON_APP_WASM)
    # The browser deliverable is the emscripten artifact set qtloader.js boots
    # (.html shell, .js runtime, .wasm module, .data MEMFS preload). Installing
    # the TARGET would ship only the .js shim, so install the set as plain
    # files into one directly servable directory instead.
    #
    # The html shell is the branded in-repo page (wasm/daemon-app.html): dark
    # theme-token chrome, byte progress, a WebGL probe that falls back to the
    # software scenegraph, and a styled error panel. It shadows the default
    # wasm_shell page Qt generates into the build dir at target finalization
    # (which stays there, unused, alongside its qtlogo.svg - neither is
    # installed, and the branded page references no logo).
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/wasm/daemon-app.html"
        "$<TARGET_FILE_DIR:daemon-app>/daemon-app.js"
        "$<TARGET_FILE_DIR:daemon-app>/daemon-app.wasm"
        "$<TARGET_FILE_DIR:daemon-app>/daemon-app.data"
        "$<TARGET_FILE_DIR:daemon-app>/qtloader.js"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/daemon-app/wasm
    )
else()
    # Install the single self-contained binary. wrapQtAppsHook (flake) wraps it
    # with an isolated Qt environment, so it runs independently of the host's Qt.
    install(TARGETS daemon-app
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
