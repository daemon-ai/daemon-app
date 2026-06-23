# Shared client-local preference store seam (app prefs: setupComplete, last
# connection, ...). A QSettings-backed implementation is used now; both front
# ends bind one instance. Quick-free: Qt6::Core only.
add_library(da_settings_store STATIC
    isettings_store.h
    qt_settings_store.h
    qt_settings_store.cpp
)
add_library(daemon-app::settings_store ALIAS da_settings_store)

set_target_properties(da_settings_store PROPERTIES AUTOMOC ON)

target_include_directories(da_settings_store PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_settings_store
    PUBLIC
        Qt6::Core
    PRIVATE
        daemon-app::warnings
)
