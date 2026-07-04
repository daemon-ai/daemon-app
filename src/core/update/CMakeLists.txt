# Release-feed / auto-update scaffold (design: packaging/UPDATES.md). Exposes the
# package-time capability dial + version/feed metadata; check/download/apply are
# honest not-implemented stubs. Both front ends link it; no QML module (UI lands
# GUI+TUI together later). Quick-free: Qt6::Core only.
add_library(da_update STATIC
    update_manager.h
    update_manager.cpp
)
add_library(daemon-app::update ALIAS da_update)

set_target_properties(da_update PROPERTIES AUTOMOC ON)

target_include_directories(da_update PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

# update_manager.cpp includes the generated daemon_app_version.h (currentVersion).
add_dependencies(da_update daemon_app_version_header)

# The per-artifact update-capability dial, FIXED AT PACKAGE TIME (UPDATES.md).
# Passed as a bare enumerator name so a typo fails the compile instead of
# silently degrading. Default None: a build that doesn't opt in has no updater.
set(DAEMON_APP_UPDATE_CAPABILITY "None"
    CACHE STRING "Update capability compiled into this artifact (None|Notify|DownloadAndOpen|SelfApply)"
)
set_property(
    CACHE DAEMON_APP_UPDATE_CAPABILITY
    PROPERTY STRINGS None Notify DownloadAndOpen SelfApply
)
if(NOT DAEMON_APP_UPDATE_CAPABILITY MATCHES "^(None|Notify|DownloadAndOpen|SelfApply)$")
    message(FATAL_ERROR
        "DAEMON_APP_UPDATE_CAPABILITY must be one of None|Notify|DownloadAndOpen|SelfApply, "
        "got '${DAEMON_APP_UPDATE_CAPABILITY}'"
    )
endif()

# The signed-manifest feed URL baked into the artifact (empty = no feed).
set(DAEMON_APP_UPDATE_FEED_URL ""
    CACHE STRING "Signed release-manifest URL compiled into this artifact (empty = no feed)"
)

target_compile_definitions(da_update PRIVATE
    DAEMON_APP_UPDATE_CAPABILITY=${DAEMON_APP_UPDATE_CAPABILITY}
    DAEMON_APP_UPDATE_FEED_URL="${DAEMON_APP_UPDATE_FEED_URL}"
)

target_link_libraries(da_update
    PUBLIC
        Qt6::Core
    PRIVATE
        daemon-app::version
        daemon-app::warnings
)
