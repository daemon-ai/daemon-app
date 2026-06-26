# Header-only numeric helpers (daemon::to_int checked narrowing). Rooted at src/core so the header
# is addressed as <util/numeric.h>, matching the da_domain include convention.
add_library(da_numeric INTERFACE)
add_library(daemon-app::numeric ALIAS da_numeric)

target_sources(da_numeric INTERFACE FILE_SET HEADERS BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/.." FILES numeric.h)
target_include_directories(da_numeric INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(da_numeric INTERFACE Qt6::Core)
