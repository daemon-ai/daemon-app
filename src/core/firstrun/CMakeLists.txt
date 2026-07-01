# Shared first-run / onboarding gate (connect -> connecting -> inference -> done),
# driven by ISettingsStore + IConnectionService. Both front ends mount it before
# the shell. Quick-free: Qt6::Core only.
add_library(da_firstrun STATIC
    first_run_model.h
    first_run_model.cpp
)
add_library(daemon-app::firstrun ALIAS da_firstrun)

set_target_properties(da_firstrun PROPERTIES AUTOMOC ON)

target_include_directories(da_firstrun PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_firstrun
    PUBLIC
        Qt6::Core
    PRIVATE
        daemon-app::settings_store
        daemon-app::connection
        daemon-app::models
        daemon-app::profiles
        daemon-app::accounts
        daemon-app::warnings
)
