# Transport-agnostic filesystem seam + a DEV local-disk implementation. The GUI
# file tree / finder / editor read and write through this seam; a daemon adapter
# replaces the dev impl later. Quick-free: Qt6::Core only (QDir, QFileSystemWatcher).
add_library(da_fs STATIC
    fs_dtos.h
    ifs_service.h
    ifs_service.cpp
    local_disk_fs_service.h
    local_disk_fs_service.cpp
)
add_library(daemon-app::fs ALIAS da_fs)

set_target_properties(da_fs PROPERTIES AUTOMOC ON)

target_include_directories(da_fs PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_fs
    PUBLIC
        Qt6::Core
    PRIVATE
        daemon-app::warnings
)
