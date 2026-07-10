# Launch-at-login (autostart) seam: pure command/entry logic + the shared
# GUI/TUI controller + one thin per-OS backend TU. Deliberately its own
# widgets-free static lib (QtCore only): the TUI must surface the toggle too
# (parity rule) but must not link QtWidgets, which da_platform publicly links
# for the tray.
add_library(da_autostart STATIC
    autostart_status.h
    autostart_command.h
    autostart_command.cpp
    autostart_entry.h
    autostart_entry.cpp
    autostart_windows_logic.h
    autostart_windows_logic.cpp
    autostart_backend.h
    autostart_controller.h
    autostart_controller.cpp
)
add_library(daemon-app::autostart ALIAS da_autostart)

set_target_properties(da_autostart PROPERTIES AUTOMOC ON)

target_include_directories(da_autostart PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../..")

target_link_libraries(da_autostart
    PUBLIC
        Qt6::Core
    PRIVATE
        daemon-app::settings_store
        daemon-app::warnings
)

# One backend per platform. The pure logic above compiles everywhere so Linux
# CI covers the Windows/XDG behavior; only these TUs touch the OS.
if(DAEMON_APP_MOBILE OR DAEMON_APP_WASM)
    target_sources(da_autostart PRIVATE autostart_backend_noop.cpp)
elseif(WIN32)
    target_sources(da_autostart PRIVATE autostart_backend_win.cpp)
elseif(APPLE)
    # SMAppService (macOS 13+) + the tested bundle/translocation predicates
    # from the updater.
    target_sources(da_autostart PRIVATE autostart_backend_mac.mm)
    target_link_libraries(da_autostart PRIVATE daemon-app::update "-framework ServiceManagement")
else()
    target_sources(da_autostart PRIVATE autostart_backend_xdg.cpp)
endif()
