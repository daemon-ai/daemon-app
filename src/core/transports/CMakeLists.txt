# Transport-adapter registry + presence seams (daemon-transport-adapter-spec.md) + inert mocks. The
# client-side view of the node's events-IO transport adapters: the multi-protocol "Add channel"
# picker, the configured instances the unified roster groups by, and per-account connection/presence.
# A daemon adapter decoding transport_adapters / transport_instances replaces the mocks later.
# Quick-free: Qt6::Core only.
add_library(da_transports STATIC
    itransport_registry.h
    ipresence_service.h
    icontacts_service.h
    ipersons_service.h
    ichat_service.h
    mock_transport_registry.h
    mock_transport_registry.cpp
    mock_presence_service.h
    mock_presence_service.cpp
    mock_contacts_service.h
    mock_contacts_service.cpp
    mock_persons_service.h
    mock_persons_service.cpp
)
add_library(daemon-app::transports ALIAS da_transports)

set_target_properties(da_transports PROPERTIES AUTOMOC ON)

target_include_directories(da_transports PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_transports
    PUBLIC
        Qt6::Core
    PRIVATE
        daemon-app::warnings
)
