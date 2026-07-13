# Consent-gated crash reporting (Sentry native SDK). A thin Qt wrapper over sentry-native: links the
# vendored static `sentry` lib when it was wired (desktop, non-wasm — see cmake/Dependencies.cmake)
# and otherwise degrades to no-op stubs, so the GUI + TUI mains call the SAME seam on every platform
# (wasm/mobile included) without per-platform #ifdefs at the call site.
add_library(da_crash STATIC
    crash_reporter.h
    crash_reporter.cpp
)
add_library(daemon-app::crash ALIAS da_crash)

target_include_directories(da_crash PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_crash
    PUBLIC
        Qt6::Core
    PRIVATE
        daemon-app::warnings
)

# The compiled-in product default DSN (public value; overridable at runtime by the
# DAEMON_APP_SENTRY_DSN env). Empty / unset ⇒ crash reporting is compiled out (disabled) by default.
if(DEFINED DAEMON_APP_SENTRY_DSN)
    target_compile_definitions(da_crash PRIVATE "DAEMON_APP_SENTRY_DSN=\"${DAEMON_APP_SENTRY_DSN}\"")
endif()

# Link the native SDK when Dependencies.cmake wired it (desktop, non-wasm). The define switches the
# module from the no-op stubs to the live sentry-backed path. sentry's headers are marked SYSTEM
# (Dependencies.cmake) so they never trip our -Werror project flags.
if(TARGET sentry)
    target_link_libraries(da_crash PRIVATE sentry)
    target_compile_definitions(da_crash PRIVATE DAEMON_APP_HAVE_SENTRY=1)
endif()
