# Fleet/sessions/dashboard/approvals seams + in-memory mocks (Phase 6). Grouped
# in one module since the dashboard derives its counters from the others.
add_library(da_fleet STATIC
    isession_roster.h
    ifleet_tree.h
    iapprovals_inbox.h
    idashboard.h
    mock_fleet_tree.h
    mock_fleet_tree.cpp
    mock_approvals_inbox.h
    mock_approvals_inbox.cpp
    mock_dashboard.h
    mock_dashboard.cpp
)
add_library(daemon-app::fleet ALIAS da_fleet)

set_target_properties(da_fleet PROPERTIES AUTOMOC ON)

target_include_directories(da_fleet PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_fleet
    PUBLIC
        Qt6::Core
        daemon-app::uimodels
        daemon-app::daemonnet
    PRIVATE
        daemon-app::warnings
)
