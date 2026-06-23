# Routing + cron seams + in-memory mocks (Phase 7 routing matrix + cron manager).
add_library(da_automation STATIC
    irouting_store.h
    icron_store.h
    mock_routing_store.h
    mock_routing_store.cpp
    mock_cron_store.h
    mock_cron_store.cpp
)
add_library(daemon-app::automation ALIAS da_automation)

set_target_properties(da_automation PROPERTIES AUTOMOC ON)

target_include_directories(da_automation PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_automation
    PUBLIC
        Qt6::Core
        daemon-app::uimodels
    PRIVATE
        daemon-app::warnings
)
