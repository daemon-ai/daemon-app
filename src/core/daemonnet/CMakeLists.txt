# Routing mutation seam (IRoutingActions) + the mock fleet/roster/session seed (MockFleetSource).
# The legacy network-seam family (the mock projection + the patch-bay graph) was deleted in M3
# (spec 09 §13): routing now lives on the mirror store, and the mock fleet/roster/sessions surfaces
# (A7's M4 wave) render from this stripped seed. Qt6::Core + uimodels (VariantListModel) + domain.
add_library(da_daemonnet STATIC
    daemonnet_dtos.h
    seed_transcripts.h
    routing_dtos.h
    irouting_actions.h
    mock_fleet_source.h
    mock_fleet_source.cpp
)
add_library(daemon-app::daemonnet ALIAS da_daemonnet)

set_target_properties(da_daemonnet PROPERTIES AUTOMOC ON)

target_include_directories(da_daemonnet PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_daemonnet
    PUBLIC
        Qt6::Core
        daemon-app::uimodels
        daemon-app::domain
    PRIVATE
        daemon-app::warnings
)
