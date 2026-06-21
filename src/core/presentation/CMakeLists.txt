# Shared display presenter: domain enums -> semantic icon/state tokens, consumed
# by both the QML GUI (as the DaemonApp.Presentation singleton) and the TUI (as a
# plain C++ lib). Quick-free: only Qt6::Qml (for the QML_ELEMENT/QML_SINGLETON
# macros and the singleton registration) plus the domain enums.
qt_add_qml_module(da_presentation
    STATIC
    URI DaemonApp.Presentation
    VERSION 1.0
    SOURCES
        display_presenter.h
        display_presenter.cpp
)
add_library(daemon-app::presentation ALIAS da_presentation)

target_include_directories(da_presentation PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_presentation
    PUBLIC
        Qt6::Qml
        daemon-app::domain
    PRIVATE
        daemon-app::warnings
)
