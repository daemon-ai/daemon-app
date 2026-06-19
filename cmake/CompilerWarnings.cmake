# Interface target carrying the project's warning flags.
# Link it PRIVATE into our own targets (never into third-party deps).

add_library(da_warnings INTERFACE)
add_library(daemon-app::warnings ALIAS da_warnings)

if(MSVC)
    target_compile_options(da_warnings INTERFACE
        /W4
        /permissive-
    )
else()
    target_compile_options(da_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wdouble-promotion
    )
endif()
