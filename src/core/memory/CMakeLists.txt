# Transport-agnostic memory-inspection seam + a seeded MOCK implementation. The
# GUI/TUI memory views read through this seam; a daemon NodeApi adapter replaces
# the mock later. Quick-free: Qt6::Core only.
add_library(da_memory STATIC
    memory_dtos.h
    imemory_service.h
    imemory_service.cpp
    mock_memory_service.h
    mock_memory_service.cpp
)
add_library(daemon-app::memory ALIAS da_memory)

set_target_properties(da_memory PROPERTIES AUTOMOC ON)

target_include_directories(da_memory PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_memory
    PUBLIC
        Qt6::Core
    PRIVATE
        daemon-app::warnings
)
