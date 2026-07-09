# Shared client-local preference store seam (app prefs: setupComplete, last
# connection, ...). A QSettings-backed implementation is used now; both front
# ends bind one instance. Quick-free: Qt6::Core only.
add_library(da_settings_store STATIC
    isettings_store.h
    qt_settings_store.h
    qt_settings_store.cpp
    ws_default.h
    ws_default.cpp
)
add_library(daemon-app::settings_store ALIAS da_settings_store)

# Browser-only page-origin reader (window.location via emscripten::val): feeds
# deriveWsDefault() so the picker prefills the remote-ws target on wasm. The pure
# derivation above stays in the all-platform list so desktop unit tests cover it.
if(DAEMON_APP_WASM)
    target_sources(da_settings_store PRIVATE wasm_page_origin.h wasm_page_origin.cpp)
endif()

set_target_properties(da_settings_store PROPERTIES AUTOMOC ON)

target_include_directories(da_settings_store PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_settings_store
    PUBLIC
        Qt6::Core
    PRIVATE
        daemon-app::warnings
)

# Optional OS keychain for the server-token store (auth6). When Qt Keychain is present the token
# persists in the platform secret store; otherwise QtSettingsStore falls back to QSettings. Only
# WASM and mobile (Android/iOS) skip it outright: the browser sandbox has no OS secret store, and
# a sandboxed mobile .app cannot answer a keychain prompt during a headless mock boot (see
# packaging/ios/README.md). The POSIX static desktop builds (Linux/macOS) DO get a real keychain:
# the flake supplies a qtkeychain built static against the same static Qt (Linux uses the D-Bus
# Secret Service backend so it drags no libsecret floor; macOS uses Security.framework), found
# here via find_package on CMAKE_PREFIX_PATH.
if(DAEMON_APP_WASM OR DAEMON_APP_MOBILE)
    message(STATUS "daemon-app: wasm/mobile build; server-token store uses QSettings fallback")
else()
    find_package(Qt6Keychain QUIET)
endif()
if(Qt6Keychain_FOUND)
    target_link_libraries(da_settings_store PRIVATE Qt6Keychain::Qt6Keychain)
    target_compile_definitions(da_settings_store PRIVATE DAEMON_APP_HAVE_KEYCHAIN)
    message(STATUS "daemon-app: server-token store uses Qt Keychain")
elseif(NOT DAEMON_APP_WASM AND NOT DAEMON_APP_MOBILE)
    message(STATUS "daemon-app: Qt Keychain not found; server-token store uses QSettings fallback")
endif()
