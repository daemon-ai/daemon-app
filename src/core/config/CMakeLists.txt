# Daemon-authoritative configuration seam + an in-memory mock (the Phase 2
# settings sections' backing facade). A daemon adapter replaces the mock later.
# Quick-free: Qt6::Core only.
add_library(da_config STATIC
    idaemon_config.h
    mock_daemon_config.h
    mock_daemon_config.cpp
)
add_library(daemon-app::config ALIAS da_config)

set_target_properties(da_config PROPERTIES AUTOMOC ON)

target_include_directories(da_config PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_config
    PUBLIC
        Qt6::Core
    PRIVATE
        daemon-app::warnings
)
