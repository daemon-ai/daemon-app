# Shared transcript exporter: serializes a session to JSON and writes a file,
# reading through the ISessionStore seam. GUI binds it as the `Exporter`
# context property; the TUI calls it directly. Core-only.
add_library(da_export STATIC
    transcript_exporter.h
    transcript_exporter.cpp
)
add_library(daemon-app::export ALIAS da_export)

set_target_properties(da_export PROPERTIES AUTOMOC ON)

target_include_directories(da_export PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(da_export
    PUBLIC
        Qt6::Core
        daemon-app::domain
    PRIVATE
        daemon-app::persistence
        daemon-app::warnings
)

# The browser download path (exportToBrowser -> platform::saveFileContent) is wasm-only, so the
# platform layer is linked only there. This keeps the desktop/TUI export library off da_platform
# (and its QtWidgets tray dependency); off wasm exportToBrowser compiles to an empty stub.
if(DAEMON_APP_WASM)
    target_link_libraries(da_export PRIVATE daemon-app::platform)
endif()
