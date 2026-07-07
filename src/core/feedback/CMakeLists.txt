# User-feedback seam + an in-memory mock (thumbs up/down on a response, a general
# app-feedback entry, and the node-owned telemetry-consent source of truth). A
# daemon adapter (FeedbackSubmit / TelemetryConsentGet/Set) replaces the mock later.
# Quick-free: Qt6::Core only.
add_library(da_feedback STATIC
    ifeedback.h
    mock_feedback.h
    mock_feedback.cpp
)
add_library(daemon-app::feedback ALIAS da_feedback)

set_target_properties(da_feedback PROPERTIES AUTOMOC ON)

target_include_directories(da_feedback PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_feedback
    PUBLIC
        Qt6::Core
    PRIVATE
        daemon-app::warnings
)
