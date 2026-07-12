# Routing mutation seam (IRoutingActions) + the canonical mock DOMAIN seed (SeedBundle; AD —
# MockFleetSource died with the legacy delegate stores: the mock scenario derives the mirror
# rows from this ONE declaration).
add_library(da_daemonnet STATIC
    daemonnet_dtos.h
    seed_transcripts.h
    routing_dtos.h
    irouting_actions.h
    seed_bundle.h
    seed_bundle.cpp
)
add_library(daemon-app::daemonnet ALIAS da_daemonnet)

set_target_properties(da_daemonnet PROPERTIES AUTOMOC ON)

target_include_directories(da_daemonnet PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_daemonnet
    PUBLIC
        Qt6::Core
        daemon-app::domain
    PRIVATE
        daemon-app::warnings
)
