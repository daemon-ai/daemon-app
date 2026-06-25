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
target_include_directories(da_daemon_codec PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/codec/vendor"
    "${CMAKE_CURRENT_SOURCE_DIR}/codec/generated"
)

add_library(da_daemon_client STATIC
    daemon_transport.h
    daemon_transport.cpp
    node_api_client.h
    node_api_client.cpp
    node_api_codec.h
    node_api_codec.cpp
    daemon_connection_service.h
    daemon_connection_service.cpp
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
        daemon-app::daemon_codec
        daemon-app::warnings
)
