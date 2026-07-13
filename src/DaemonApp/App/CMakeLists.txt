# The App module is the application executable and the DaemonApp.App QML module.
# (Target name 'daemon-app' intentionally differs from the URI 'DaemonApp.App'.)

qt_add_executable(daemon-app
    main.cpp
    application.h
    application.cpp
)

# main.cpp includes the generated daemon_app_version.h; depend on the target that refreshes it.
add_dependencies(daemon-app daemon_app_version_header)

# --- macOS bundle ------------------------------------------------------------
# Desktop macOS builds a proper .app bundle: Finder/Dock integration,
# codesigning, notarization, and the CPack DragNDrop (.dmg) packaging all
# require one. iOS is APPLE too but bundles through the mobile toolchain,
# hence the DAEMON_APP_MOBILE guard. Set here (not in cmake/Packaging.cmake)
# so the property lands before Qt's deferred target finalization runs at this
# directory's scope end - the finalizer reads MACOSX_BUNDLE for bundle rpath
# handling. Bundle payload staging and the DMG live in cmake/Packaging.cmake.
if(APPLE AND NOT DAEMON_APP_MOBILE)
    # Build-time copy into <bundle>/Contents/Resources, so the build-tree
    # bundle carries the icon too; installs stage the same file.
    set(_da_macos_icns "${CMAKE_SOURCE_DIR}/packaging/macos/daemon-app.icns")
    set_source_files_properties("${_da_macos_icns}"
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources
    )
    target_sources(daemon-app PRIVATE "${_da_macos_icns}")
    # Info.plist template: CFBundleIdentifier io.daemon.app (the AppStream
    # id), version fields from @PROJECT_VERSION@ (see the template header).
    set_target_properties(daemon-app PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/packaging/macos/Info.plist.in"
    )
    # Launch-at-login agent definition for SMAppService (see the plist header).
    # Static bundle content only - registration stays 100% app-driven
    # (src/core/platform/autostart). MACOSX_PACKAGE_LOCATION is Contents-
    # relative, so this lands at Contents/Library/LaunchAgents/.
    set(_da_macos_agent_plist
        "${CMAKE_SOURCE_DIR}/packaging/macos/io.daemon.app.autostart.plist")
    set_source_files_properties("${_da_macos_agent_plist}"
        PROPERTIES MACOSX_PACKAGE_LOCATION "Library/LaunchAgents"
    )
    target_sources(daemon-app PRIVATE "${_da_macos_agent_plist}")
endif()

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
        # Consent-gated crash reporting (Sentry; no-op on wasm).
        daemon-app::crash
        # Core services
        daemon-app::i18n
        daemon-app::persistence
        daemon-app::platform
        daemon-app::autostart
        daemon-app::command
        daemon-app::export
        daemon-app::settings_store
        # Release-feed / update scaffold: capability-dial surface only, no UI
        # wiring yet (packaging/UPDATES.md).
        daemon-app::update
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
        daemon-app::chat_view
        daemon-app::terminal
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
        da_graphplugin
        da_blockeditorplugin
        da_sidebarplugin
        da_sessionslistplugin
        da_session_viewplugin
        da_tabsplugin
        da_chat_viewplugin
        da_terminalplugin
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
# Desktop/mobile resolve it at runtime (application.cpp): the installed
# share/daemon-app/microtex-res copy (install rule below) wins over the baked
# build-time path, which stays only for dev runs out of the build tree -
# shipping the bake would drag the whole MicroTeX source tree into the Nix
# runtime closure. The browser cannot read host paths, so the wasm link packs
# the directory into the emscripten .data preload bundle and init() reads it
# back from the same fixed MEMFS mount point.
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
    # Desktop bakes a BUILD-TREE copy as the last-resort fallback (dev runs
    # straight out of the build dir, where nothing is installed). Deliberately
    # not the source res dir itself: under Nix that is a store path, and a
    # store path baked into the binary chains the whole MicroTeX source tree
    # into the runtime closure of the installed app.
    file(COPY "${MICROTEX_RES_DIR}" DESTINATION "${CMAKE_BINARY_DIR}/microtex")
    target_compile_definitions(daemon-app
        PRIVATE MICROTEX_RES_DIR="${CMAKE_BINARY_DIR}/microtex/res"
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

# --- Android package -----------------------------------------------------
# Qt's target finalization turns these properties into the
# android-*-deployment-settings.json that androiddeployqt packages from (the
# flake's `apk` output drives it gradle-lessly; see nix/android.nix and
# packaging/android/README.md).
if(ANDROID)
    # packaging/android carries the AndroidManifest.xml (QtActivity-based,
    # application id io.daemon.app) + launcher icons; androiddeployqt overlays
    # it onto Qt's package template.
    # versionCode: monotonic integer derived from the SemVer base
    # (major*10000 + minor*100 + patch, e.g. 0.3.7 -> 307); versionName: the
    # git-enriched build string when the flake provides it, else the base.
    math(EXPR _da_android_version_code
        "${PROJECT_VERSION_MAJOR} * 10000 + ${PROJECT_VERSION_MINOR} * 100 + ${PROJECT_VERSION_PATCH}")
    if(DEFINED DAEMON_APP_VERSION_STR)
        set(_da_android_version_name "${DAEMON_APP_VERSION_STR}")
    else()
        set(_da_android_version_name "${PROJECT_VERSION}")
    endif()
    # MicroTeX's res tree rides androiddeployqt's assets mechanism
    # (QT_ANDROID_EXTRA_PLUGINS: non-.so files of each listed directory land
    # under assets/<dirname>/). Android assets are not POSIX paths, so
    # application.cpp extracts assets:/microtex-res to the app data dir on
    # first boot and points tex::LaTeX::init() there (microtexResDir).
    file(COPY "${MICROTEX_RES_DIR}/" DESTINATION "${CMAKE_BINARY_DIR}/android-assets/microtex-res")
    set_target_properties(daemon-app PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/packaging/android"
        QT_ANDROID_PACKAGE_NAME "io.daemon.app"
        QT_ANDROID_VERSION_CODE "${_da_android_version_code}"
        QT_ANDROID_VERSION_NAME "${_da_android_version_name}"
        QT_ANDROID_MIN_SDK_VERSION "28"
        QT_ANDROID_TARGET_SDK_VERSION "35"
        QT_ANDROID_EXTRA_PLUGINS "${CMAKE_BINARY_DIR}/android-assets/microtex-res"
    )
    # The KSyntaxHighlighting QML plugin (org.kde.syntaxhighlighting) is a
    # SHARED runtime plugin on desktop, resolved via the engine import path.
    # The android build forces BUILD_SHARED_LIBS=OFF (nix/android.nix), making
    # it a static archive like our own feature plugins - link it explicitly
    # and register it via Q_IMPORT_QML_PLUGIN in main.cpp (androiddeployqt
    # cannot bundle a loose non-Qt QML plugin without gradle-side wiring).
    target_link_libraries(daemon-app PRIVATE kquicksyntaxhighlightingplugin)
endif()

# --- iOS bundle ----------------------------------------------------------
# iOS is APPLE + mobile: Qt's iOS toolchain drives the .app + Info.plist
# through the Xcode generator, so (unlike the desktop-macOS block above) we do
# not point MACOSX_BUNDLE_INFO_PLIST at packaging/macos - Qt's default iOS
# template launches the simulator fine (a custom plist only if it ever blocks
# launch; see packaging/ios/README.md). Bundle id is the AppStream id
# io.daemon.app, matching the macOS/Android identity.
if(IOS)
    set_target_properties(daemon-app PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_GUI_IDENTIFIER "io.daemon.app"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "io.daemon.app"
    )

    # App icon: the asset catalog (packaging/ios, generated by nix/icons.nix)
    # is added as a bundle resource so Xcode's asset-catalog compiler (actool)
    # emits Assets.car + the CFBundleIcons plist keys; APPICON_NAME points it at
    # the AppIcon set. A single 1024 universal image is the Xcode 14+ contract
    # (the toolchain downscales the launcher/settings sizes from it).
    set(_da_ios_assets "${CMAKE_SOURCE_DIR}/packaging/ios/Assets.xcassets")
    set_source_files_properties("${_da_ios_assets}"
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources
    )
    target_sources(daemon-app PRIVATE "${_da_ios_assets}")
    set_target_properties(daemon-app PROPERTIES
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
    )

    # MicroTeX's res tree ships INSIDE the (flat) .app so microtexResDir()
    # resolves it app-dir-relative (applicationDirPath()/microtex-res - the
    # existing candidate already covers the iOS layout). iOS bundles are flat,
    # so stage each file with a per-file MACOSX_PACKAGE_LOCATION that preserves
    # the tree under microtex-res/; Xcode copies them as bundle resources.
    file(GLOB_RECURSE _da_ios_microtex_files RELATIVE "${MICROTEX_RES_DIR}"
        "${MICROTEX_RES_DIR}/*")
    foreach(_rel IN LISTS _da_ios_microtex_files)
        get_filename_component(_reldir "${_rel}" DIRECTORY)
        if(_reldir STREQUAL "")
            set(_loc "microtex-res")
        else()
            set(_loc "microtex-res/${_reldir}")
        endif()
        set_source_files_properties("${MICROTEX_RES_DIR}/${_rel}" PROPERTIES
            MACOSX_PACKAGE_LOCATION "${_loc}")
    endforeach()
    list(TRANSFORM _da_ios_microtex_files PREPEND "${MICROTEX_RES_DIR}/")
    target_sources(daemon-app PRIVATE ${_da_ios_microtex_files})

    # Static Qt: link the KSyntaxHighlighting QML plugin explicitly and
    # register it via Q_IMPORT_QML_PLUGIN in main.cpp (same as Android).
    target_link_libraries(daemon-app PRIVATE kquicksyntaxhighlightingplugin)
endif()

# Build dependency on the KSyntaxHighlighting QML plugin so its module is emitted
# under QT_QML_OUTPUT_DIRECTORY before we run; main.cpp adds that dir to the
# engine import path via DAEMON_APP_BUILD_QML_DIR.
add_dependencies(daemon-app kquicksyntaxhighlightingplugin)
target_compile_definitions(daemon-app
    PRIVATE DAEMON_APP_BUILD_QML_DIR="${CMAKE_BINARY_DIR}/qml"
)

# DEV ONLY (option defaults OFF): enable Qt's QML debug connector so `qmlpreview`
# can attach and live-reload QML/JS from disk. Never set in shipped/bundle builds.
if(DAEMON_APP_QML_DEBUG)
    target_compile_definitions(daemon-app PRIVATE QT_QML_DEBUG)
endif()

# Embedded terminal on the POSIX static desktop: qmltermwidget is a prebuilt
# STATIC QML plugin (the flake builds it against the static Qt). Link the archive
# and register it via Q_IMPORT_QML_PLUGIN in main.cpp (same mechanism as
# KSyntaxHighlighting on static/mobile), with its qmldir dir on the engine import
# path. Skipped on Windows (no Unix PTY -> terminal stub) and wasm/mobile.
if(DAEMON_APP_STATIC AND NOT WIN32 AND QMLTERMWIDGET_STATIC_LIB)
    target_link_libraries(daemon-app PRIVATE "${QMLTERMWIDGET_STATIC_LIB}")
    target_compile_definitions(daemon-app PRIVATE
        DAEMON_APP_STATIC_QMLTERMWIDGET
        DAEMON_APP_QMLTERMWIDGET_QML_DIR="${QMLTERMWIDGET_STATIC_QML_DIR}"
    )
endif()

# On desktop the QApplication base lives in QtWidgets (for the tray). Mobile
# and the browser build run QGuiApplication (see main.cpp) and never link it.
if(NOT DAEMON_APP_MOBILE AND NOT DAEMON_APP_WASM)
    target_link_libraries(daemon-app PRIVATE Qt6::Widgets)

    # Embed the multi-size launcher PNGs as a Qt resource so the window/taskbar
    # chrome and the system tray carry the brand with no on-disk asset lookup
    # (platform::appIcon() reads ":/icons/daemon-app-<N>.png" as its fallback
    # after QIcon::fromTheme). Aliased straight from the committed hicolor set
    # (BASE strips the dir, so the alias is just the file name under /icons) -
    # no duplicate copies. Mobile brands via the launcher mipmaps and the
    # browser via the favicon, so neither needs this resource.
    qt_add_resources(daemon-app "app_icons"
        PREFIX "/icons"
        BASE "${CMAKE_SOURCE_DIR}/packaging/linux/icons"
        FILES
            "${CMAKE_SOURCE_DIR}/packaging/linux/icons/daemon-app-16.png"
            "${CMAKE_SOURCE_DIR}/packaging/linux/icons/daemon-app-32.png"
            "${CMAKE_SOURCE_DIR}/packaging/linux/icons/daemon-app-48.png"
            "${CMAKE_SOURCE_DIR}/packaging/linux/icons/daemon-app-64.png"
            "${CMAKE_SOURCE_DIR}/packaging/linux/icons/daemon-app-128.png"
            "${CMAKE_SOURCE_DIR}/packaging/linux/icons/daemon-app-256.png"
    )
endif()

# --- Windows executable polish ----------------------------------------------
if(WIN32)
    # GUI subsystem: no console window flashes up on launch. The headless
    # probes' stdout sentinel contract (DAEMON_APP_WAIT_READY prints
    # "DAEMON_APP_READY ok", see main.cpp) is unaffected in practice: every
    # consumer (wine smoke, E2E harnesses) runs the exe with stdout
    # REDIRECTED to a pipe/file, and a GUI-subsystem process inherits and
    # writes such handles fine - only an interactive console loses the text,
    # which is not a supported flow.
    set_target_properties(daemon-app PROPERTIES WIN32_EXECUTABLE TRUE)

    # Embedded icon + VERSIONINFO resource, compiled in via windres
    # (CMAKE_RC_COMPILER from the MinGW toolchain). The numeric version
    # fields derive from the top-level VERSION file through project(VERSION).
    enable_language(RC)
    set(DAEMON_APP_RC_ICON "${CMAKE_SOURCE_DIR}/packaging/windows/daemon-app.ico")
    configure_file(
        "${CMAKE_SOURCE_DIR}/packaging/windows/daemon-app.rc.in"
        "${CMAKE_BINARY_DIR}/packaging/daemon-app.rc"
        @ONLY
    )
    target_sources(daemon-app PRIVATE "${CMAKE_BINARY_DIR}/packaging/daemon-app.rc")
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
    # Favicons (generated by nix/icons.nix) ride next to the shell so the
    # browser tab + iOS home-screen bookmark are branded; daemon-app.html
    # references them by name. They also flow into the hosted-node OCI web root.
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/wasm/daemon-app.html"
        "${CMAKE_SOURCE_DIR}/packaging/web/favicon-32.png"
        "${CMAKE_SOURCE_DIR}/packaging/web/favicon-180.png"
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
    # Ship MicroTeX's runtime resources next to the binary, where the startup
    # probe (application.cpp microtexResDir) finds them app-dir-relative.
    # NOT on APPLE: the macOS bundle already stages microtex-res into
    # Contents/Resources (cmake/Packaging.cmake), which is where the probe
    # resolves inside a .app; a second copy under share/daemon-app would only
    # bloat the DragNDrop DMG root. Linux + Windows keep the app-dir-relative
    # share copy their portable payloads rely on.
    if(NOT APPLE)
        install(DIRECTORY "${MICROTEX_RES_DIR}/"
            DESTINATION ${CMAKE_INSTALL_DATADIR}/daemon-app/microtex-res
        )
    endif()
endif()
