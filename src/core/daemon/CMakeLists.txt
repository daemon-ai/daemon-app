# Vendored NodeApi CBOR codec: the zcbor C runtime plus the generated daemon-api client codec
# (codec/generated), produced and drift-checked by the superproject from the daemon-node contract.
# Built as plain C and kept free of the project's -Werror warning set since it is third-party /
# generated code; see codec/README.md for the regeneration recipe.
enable_language(C)

add_library(da_daemon_codec STATIC
    codec/vendor/zcbor_common.c
    codec/vendor/zcbor_decode.c
    codec/vendor/zcbor_encode.c
    codec/vendor/zcbor_print.c
    codec/generated/daemon_api_client_decode.c
    codec/generated/daemon_api_client_encode.c
)
add_library(daemon-app::daemon_codec ALIAS da_daemon_codec)
set_target_properties(da_daemon_codec PROPERTIES C_STANDARD 11 POSITION_INDEPENDENT_CODE ON)
# Encode in canonical (definite-length) CBOR. Without this, zcbor emits indefinite-length maps
# (0xBF..0xFF), which the daemon's serde/ciborium decoder rejects for externally-tagged enum
# variants ("invalid type: map, expected enum") - so every data-carrying ApiRequest (CredentialSet,
# ModelCurrent, SessionsQuery, ...) would fail to decode. Canonical maps (0xA1/0xA2) match the
# daemon's own serde encoding byte-for-byte. PUBLIC so the codec headers + callers agree.
target_compile_definitions(da_daemon_codec PUBLIC ZCBOR_CANONICAL)
target_include_directories(da_daemon_codec PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/codec/vendor"
    "${CMAKE_CURRENT_SOURCE_DIR}/codec/generated"
)

# Platform-conditional TUs: Qt-for-wasm lacks the `localserver`, `ssl`, and `process` features,
# so the browser build swaps the Unix/TLS stream carriers and the QProcess-based launcher for
# fail-closed stubs (the WebSocket carrier in daemon_transport.cpp is the only wasm transport).
# Android shares the stubs and the WebSocket-only posture: the flake's android Qt builds without
# the ssl feature (no bundled OpenSSL, packaging/android/README.md), and an APK cannot spawn or
# probe a local daemon anyway - it is a thin remote client exactly like the browser.
if(DAEMON_APP_WASM OR ANDROID)
    set(DA_DAEMON_PLATFORM_SOURCES
        daemon_transport_stream_wasm.cpp
        local_daemon_launcher_wasm.cpp
    )
else()
    set(DA_DAEMON_PLATFORM_SOURCES
        daemon_transport_stream.cpp
        local_daemon_launcher.cpp
    )
endif()

add_library(da_daemon_client STATIC
    daemon_transport.h
    daemon_transport.cpp
    node_api_client.h
    node_api_client.cpp
    node_api_auth.h
    node_api_auth.cpp
    principal_model.h
    page_loop.h
    node_api_codec.h
    node_api_codec/internal.h
    node_api_codec/mappers.cpp
    node_api_codec/encode_requests.cpp
    node_api_codec/wire_l0.cpp
    node_api_codec/decode_responses.cpp
    node_api_codec/fs_fleet_channels.cpp
    local_daemon_launcher.h
    daemon_connection_service.h
    daemon_connection_service.cpp
    ${DA_DAEMON_PLATFORM_SOURCES}
    daemon_accounts_service.h
    daemon_accounts_service.cpp
    daemon_model_catalog.h
    daemon_model_catalog.cpp
    daemon_provider_catalog.h
    daemon_provider_catalog.cpp
    daemon_profile_store.h
    daemon_profile_store.cpp
    daemon_session_settings.h
    daemon_session_settings.cpp
    daemon_approvals_inbox.h
    daemon_approvals_inbox.cpp
    subscription_manager.h
    subscription_manager.cpp
    daemon_fs_service.h
    daemon_fs_service.cpp
    daemon_fleet_tree.h
    daemon_fleet_tree.cpp
    daemon_transport_registry.h
    daemon_transport_registry.cpp
    daemon_presence_service.h
    daemon_presence_service.cpp
    daemon_cache_store.h
    daemon_cache_store.cpp
    cached_session_store.h
    cached_session_store.cpp
    app_service_graph.h
    app_service_graph.cpp
    client_cache_schema.h
    repositories.h
    repositories.cpp
    seam_migration.h
    integration_slice.h
    cache_consolidation_policy.h
)
add_library(daemon-app::daemon_client ALIAS da_daemon_client)

set_target_properties(da_daemon_client PROPERTIES AUTOMOC ON)

target_include_directories(da_daemon_client PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_daemon_client
    PUBLIC
        Qt6::Core
        Qt6::Network
        Qt6::Sql
        daemon-app::connection
        daemon-app::persistence
        daemon-app::settings_store
        daemon-app::config
        daemon-app::fs
        daemon-app::memory
        daemon-app::models
        daemon-app::accounts
        daemon-app::profiles
        daemon-app::fleet
        daemon-app::automation
        daemon-app::session
        daemon-app::transports
        daemon-app::daemonnet
        daemon-app::nav
        daemon-app::firstrun
    PRIVATE
        # The WebSocket mux carrier (daemon_transport.cpp); private because no public header
        # exposes QtWebSockets types.
        Qt6::WebSockets
        daemon-app::daemon_codec
        # Browser-persistence + network-wakeup seams (all no-ops off wasm, private - no public
        # daemon_client header exposes platform types):
        #  - DaemonCacheStore calls platform::scheduleIdbfsSync() / installUnloadFlush() (W1/W2).
        #  - daemon_connection_service.cpp calls platform::installOnlineCallback() (W5).
        daemon-app::platform
        daemon-app::warnings
)
