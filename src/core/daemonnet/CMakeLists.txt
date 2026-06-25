# DaemonNet seam (daemon-app/docs/multi-protocol-client-surface.md §1) + inert mock. The unified
# network of actors (agents/peers/users) and places (rooms/channels) joined by
# runs/over/participant/inPlace/delegation edges, with sessions as first-class nodes. The single mock
# source the Fleet/Channels/By-Peer projections + the future patch-bay derive from. Qt6::Core +
# uimodels (VariantListModel).
add_library(da_daemonnet STATIC
    daemonnet_dtos.h
    seed_transcripts.h
    idaemonnet.h
    mock_daemonnet.h
    mock_daemonnet.cpp
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
