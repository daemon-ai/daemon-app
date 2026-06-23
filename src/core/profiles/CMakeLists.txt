# Profiles/agents seam + in-memory mock (Phase 5 profile editor + curator).
add_library(da_profiles STATIC
    iprofile_store.h
    mock_profile_store.h
    mock_profile_store.cpp
)
add_library(daemon-app::profiles ALIAS da_profiles)

set_target_properties(da_profiles PROPERTIES AUTOMOC ON)

target_include_directories(da_profiles PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_profiles
    PUBLIC
        Qt6::Core
        daemon-app::uimodels
    PRIVATE
        daemon-app::warnings
)
