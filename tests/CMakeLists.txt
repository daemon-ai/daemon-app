add_subdirectory(unit)
add_subdirectory(qml)
add_subdirectory(blockeditor)

# TUI transcript-render tests need TuiWidgets (pkg-config), only present when the
# experimental terminal frontend is enabled.
if(DAEMON_APP_TUI)
    add_subdirectory(tui)
endif()
