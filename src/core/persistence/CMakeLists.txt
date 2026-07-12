add_library(da_persistence STATIC
    isession_store.h
)
add_library(daemon-app::persistence ALIAS da_persistence)

set_target_properties(da_persistence PROPERTIES AUTOMOC ON)

target_include_directories(da_persistence PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_persistence
    PUBLIC
        daemon-app::domain
        daemon-app::daemonnet
        Qt6::Core
    PRIVATE
        daemon-app::warnings
)
