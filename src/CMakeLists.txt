# core/ holds the plain-C++ libraries (no QML): domain, persistence, platform.
add_subdirectory(core)

# QML feature modules (and the app executable).
add_subdirectory(DaemonApp)

# Experimental Tui Widgets terminal frontend, gated behind DAEMON_APP_TUI.
if(DAEMON_APP_TUI)
    add_subdirectory(tui)
endif()
