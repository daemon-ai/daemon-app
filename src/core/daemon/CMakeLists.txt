add_library(da_daemon_client STATIC
    daemon_transport.h
    daemon_transport.cpp
    node_api_client.h
    node_api_client.cpp
    daemon_connection_service.h
    daemon_connection_service.cpp
    daemon_cache_store.h
    daemon_cache_store.cpp
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
        daemon-app::nav
        daemon-app::firstrun
    PRIVATE
        daemon-app::warnings
)
