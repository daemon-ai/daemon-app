# DaemonApp.Memory - Mnemosyne memory visualization (Overview / Memories / Graph /
# Timeline). Shared view-models over the IMemoryService seam (da_memory); the
# graph render is GUI-only (Canvas + Repeater), the rest reach TUI parity by
# reusing these models. Page chrome (PageHeader / NavRail) is reused from
# DaemonApp.Pages.
qt_add_qml_module(da_memory_views
    STATIC
    URI DaemonApp.Memory
    VERSION 1.0
    SOURCES
        memory_list_model.cpp
        memory_list_model.h
        memory_stats_model.cpp
        memory_stats_model.h
        memory_timeline_model.cpp
        memory_timeline_model.h
        memory_graph_model.cpp
        memory_graph_model.h
    QML_FILES
        qml/MemoryPage.qml
        qml/MemoryOverview.qml
        qml/MemoryList.qml
        qml/MemoryDetail.qml
        qml/MemoryTimeline.qml
        qml/MemoryGraphView.qml
        qml/AgentProfilePage.qml
)
add_library(daemon-app::memory_views ALIAS da_memory_views)

target_include_directories(da_memory_views PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(da_memory_views
    PRIVATE
        Qt6::Quick
        daemon-app::memory
        daemon-app::theme
        daemon-app::controls
        daemon-app::pages
        daemon-app::warnings
)
