# Transport-agnostic connection seam + a scripted mock. Drives the liveness
# state machine behind StatusBarModel.gatewayState; a daemon adapter replaces the
# mock later. Quick-free: Qt6::Core only (QTimer).
add_library(da_connection STATIC
    connection_dtos.h
    iconnection_service.h
    mock_connection_service.h
    mock_connection_service.cpp
)
add_library(daemon-app::connection ALIAS da_connection)

set_target_properties(da_connection PROPERTIES AUTOMOC ON)

target_include_directories(da_connection PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_connection
    PUBLIC
        Qt6::Core
    PRIVATE
        daemon-app::warnings
)
