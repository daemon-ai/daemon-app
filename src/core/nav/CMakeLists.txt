# The shared app-level navigation seam (which manager/settings page is open).
# Both front ends bind one instance. Quick-free: Qt6::Core only.
add_library(da_nav STATIC
    nav_controller.h
    nav_controller.cpp
)
add_library(daemon-app::nav ALIAS da_nav)

set_target_properties(da_nav PROPERTIES AUTOMOC ON)

target_include_directories(da_nav PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_nav
    PUBLIC
        Qt6::Core
    PRIVATE
        daemon-app::warnings
)
