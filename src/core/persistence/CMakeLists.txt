add_library(da_persistence STATIC
    iconversation_store.h
    in_memory_conversation_store.h
    in_memory_conversation_store.cpp
    sqlite_conversation_store.h
    sqlite_conversation_store.cpp
)
add_library(daemon-app::persistence ALIAS da_persistence)

set_target_properties(da_persistence PROPERTIES AUTOMOC ON)

target_include_directories(da_persistence PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_persistence
    PUBLIC
        daemon-app::domain
        Qt6::Core
        Qt6::Sql
    PRIVATE
        daemon-app::warnings
)
