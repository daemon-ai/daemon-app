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
# persists in the platform secret store; otherwise QtSettingsStore falls back to QSettings. The
# static build never even looks: qtkeychain is a shared-Qt library (and would drag a DBus-linked
# libsecret floor into the portable binary), so the QSettings fallback is the deliberate choice.
if(DAEMON_APP_STATIC)
    message(STATUS "daemon-app: static build; server-token store uses QSettings fallback")
else()
    find_package(Qt6Keychain QUIET)
endif()
if(Qt6Keychain_FOUND)
    target_link_libraries(da_settings_store PRIVATE Qt6Keychain::Qt6Keychain)
    target_compile_definitions(da_settings_store PRIVATE DAEMON_APP_HAVE_KEYCHAIN)
    message(STATUS "daemon-app: server-token store uses Qt Keychain")
elseif(NOT DAEMON_APP_STATIC)
    message(STATUS "daemon-app: Qt Keychain not found; server-token store uses QSettings fallback")
endif()
