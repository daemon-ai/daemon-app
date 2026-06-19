# core/ holds the plain-C++ libraries (no QML): domain, persistence, platform.
add_subdirectory(core)

# QML feature modules (and the app executable).
add_subdirectory(DaemonApp)
