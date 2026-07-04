add_subdirectory(unit)
add_subdirectory(qml)
add_subdirectory(blockeditor)

# i18n coverage gate: fail if any user-facing QML string is assigned a raw
# literal instead of flowing through qsTr(). Runs the source scanner directly
# (no build artifact needed); see scripts/i18n-audit.py + its allowlist.
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(
        NAME i18n_audit
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/i18n-audit.py
                --root ${CMAKE_SOURCE_DIR}/src
                --allowlist ${CMAKE_SOURCE_DIR}/scripts/i18n-audit-allowlist.txt)
else()
    message(STATUS "daemon-app: Python3 not found; skipping the i18n_audit test")
endif()

# TUI transcript-render tests need TuiWidgets (pkg-config), only present when the
# experimental terminal frontend is enabled.
if(DAEMON_APP_TUI)
    add_subdirectory(tui)
endif()
