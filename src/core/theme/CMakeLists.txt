# Shared theme palette: the single source of truth for the four named themes'
# color tokens, consumed by both the QML GUI (as the DaemonApp.ThemeCore
# ThemeTokens singleton, read by Theme.qml) and the TUI (as a plain C++ lib, read
# by tui_palette). Quick-free: Qt6::Gui for QColor (works under QCoreApplication,
# as be_core already relies on) + Qt6::Qml for the singleton registration macros.
qt_add_qml_module(da_theme_core
    STATIC
    URI DaemonApp.ThemeCore
    VERSION 1.0
    SOURCES
        theme_palette.h
        theme_palette.cpp
)
add_library(daemon-app::theme_core ALIAS da_theme_core)

target_include_directories(da_theme_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_theme_core
    PUBLIC
        Qt6::Gui
        Qt6::Qml
    PRIVATE
        daemon-app::warnings
)
