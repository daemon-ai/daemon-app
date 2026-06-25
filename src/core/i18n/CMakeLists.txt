# Runtime translation loading (QTranslator install + locale resolution), shared
# by the GUI and TUI front ends. Quick-free: Qt6::Core only.
add_library(da_i18n STATIC
    localization.h
    localization.cpp
)
add_library(daemon-app::i18n ALIAS da_i18n)

set_target_properties(da_i18n PROPERTIES AUTOMOC ON)

target_include_directories(da_i18n PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_i18n
    PUBLIC
        Qt6::Core
    PRIVATE
        daemon-app::warnings
)
