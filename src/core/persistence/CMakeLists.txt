add_library(da_persistence STATIC
    isession_store.h
    in_memory_session_store.h
    in_memory_session_store.cpp
    sqlite_session_store.h
    sqlite_session_store.cpp
)
add_library(daemon-app::persistence ALIAS da_persistence)

set_target_properties(da_persistence PROPERTIES AUTOMOC ON)

target_include_directories(da_persistence PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_persistence
    PUBLIC
        daemon-app::domain
        daemon-app::daemonnet
        Qt6::Core
        Qt6::Sql
    PRIVATE
        daemon-app::warnings
)
