# Reusable Daemon Kit graph component: a generic force-directed node/edge view (GraphModel + GraphView
# + NodeChip), with no service/domain coupling, so multiple surfaces (routing topology now, the memory
# graph later) share one graph widget. Modeled on QuickQanava's GraphView/Graph split + visual
# connector, implemented in-house with no external dependency.
qt_add_qml_module(da_graph
    STATIC
    URI DaemonApp.Graph
    VERSION 1.0
    SOURCES
        graph_model.h
        graph_model.cpp
    QML_FILES
        GraphView.qml
        NodeChip.qml
)
add_library(daemon-app::graph ALIAS da_graph)

target_link_libraries(da_graph
    PRIVATE
        Qt6::Quick
        daemon-app::theme
        daemon-app::controls
        daemon-app::warnings
)
