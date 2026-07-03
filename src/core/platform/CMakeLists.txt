add_library(da_platform STATIC
    iplatform_services.h
    platform_services_factory.h
    platform_services_factory.cpp
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
