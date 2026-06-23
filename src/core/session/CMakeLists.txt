# Per-session overrides + checkpoint-timeline seams + mocks (Phase 8 composer
# session-settings popover + checkpoints/rewind UI).
add_library(da_session STATIC
    isession_settings.h
    icheckpoint_timeline.h
    mock_session_settings.h
    mock_session_settings.cpp
    mock_checkpoint_timeline.h
    mock_checkpoint_timeline.cpp
)
add_library(daemon-app::session ALIAS da_session)

set_target_properties(da_session PROPERTIES AUTOMOC ON)

target_include_directories(da_session PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_session
    PUBLIC
        Qt6::Core
        daemon-app::uimodels
    PRIVATE
        daemon-app::warnings
)
