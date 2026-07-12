# Cron seam + in-memory mock (Phase 7 cron manager). The legacy intent->model routing store was
# retired at wire v28; the routing surface is the origin->session pin table on the mirror store (M3).
add_library(da_automation STATIC
    icron_store.h
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
