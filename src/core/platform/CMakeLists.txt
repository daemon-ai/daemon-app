add_library(da_platform STATIC
    iplatform_services.h
    platform_services_factory.h
    platform_services_factory.cpp
    # Branded application QIcon (window chrome + tray), shared by every surface.
    # All-platform: the body degrades to a null icon where the icon resource is
    # absent (mobile / wasm), so callers link a definition everywhere.
    app_icon.h
    app_icon.cpp
    # Browser-persistence contract surface (Browser Persistence Polish wave). Headers are
    # all-platform; the .cpp bodies are compiled on every platform (unlike wasm_page_origin.cpp,
    # which is wasm-only) so the all-platform call sites link a definition everywhere — the bodies
    # are no-ops off wasm and stubs on wasm. Pre-registered once here so no downstream stream needs
    # to touch this source list. See wasm_contracts.h for the pinned string contracts.
    wasm_contracts.h
    wasm_persistence.h
    wasm_persistence.cpp
    wasm_online.h
    wasm_online.cpp
    wasm_file_bridge.h
    wasm_file_bridge.cpp
)
add_library(daemon-app::platform ALIAS da_platform)

set_target_properties(da_platform PROPERTIES AUTOMOC ON)

target_include_directories(da_platform PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_platform
    PUBLIC daemon-app::domain
    PRIVATE daemon-app::warnings
)

# Mobile and the browser (wasm) get the no-op services; the desktop tray
# implementation (QtWidgets) is excluded from those builds entirely.
if(DAEMON_APP_MOBILE OR DAEMON_APP_WASM)
    target_sources(da_platform PRIVATE
        mobile/noop_platform_services.h
    )
    target_link_libraries(da_platform PUBLIC Qt6::Gui)
else()
    target_sources(da_platform PRIVATE
        desktop/desktop_platform_services.h
        desktop/desktop_platform_services.cpp
    )
    # QSystemTrayIcon / QMenu / QAction live in QtWidgets (desktop only).
    target_link_libraries(da_platform PUBLIC Qt6::Widgets)
endif()
