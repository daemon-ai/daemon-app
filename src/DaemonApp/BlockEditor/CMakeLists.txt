# DaemonApp.BlockEditor - the ported qml-block-editor Markdown renderer/editor.
#
# Two plain-C++ static engines (be_core: md4qt parse/project/store; be_diagram:
# mermaid flowchart layout via earcut) plus the QML module that exposes the
# controllers (EditorController, BlockModel, ...) and the block delegates. The
# upstream include style is "core/..", "app/..", "diagram/.." rooted at this dir,
# so every target's include root is CMAKE_CURRENT_SOURCE_DIR. Project warning
# flags are intentionally NOT applied: this is vendored upstream code.

# --- be_diagram: mermaid flowchart engine -----------------------------------
add_library(be_diagram STATIC
    diagram/engine/diagram_engine.cpp
    diagram/engine/diagram_engine.h
    diagram/geometry/path_mesh.cpp
    diagram/geometry/path_mesh.h
    diagram/geometry/shapes.cpp
    diagram/geometry/shapes.h
    diagram/ir/diagram_model.h
    diagram/layout/layered_layout.cpp
    diagram/layout/layered_layout.h
    diagram/layout/work_graph.h
    diagram/parser/detect.cpp
    diagram/parser/detect.h
    diagram/parser/flowchart_parser.cpp
    diagram/parser/flowchart_parser.h
    diagram/snapshot/render_snapshot.h
    diagram/text/text_measurer.cpp
    diagram/text/text_measurer.h
)
target_include_directories(be_diagram PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(be_diagram PUBLIC Qt6::Core Qt6::Gui earcut::earcut)

# --- be_core: markdown parse / project / document store ---------------------
add_library(be_core STATIC
    core/agent_block.cpp
    core/agent_block.h
    core/agent_ingest.cpp
    core/agent_ingest.h
    core/block_record.cpp
    core/block_record.h
    core/block_height_index.cpp
    core/block_height_index.h
    core/command_stack.cpp
    core/command_stack.h
    core/coordinate_map.cpp
    core/coordinate_map.h
    core/document_store.cpp
    core/document_store.h
    core/image_url.h
    core/inline_projector.cpp
    core/inline_projector.h
    core/markdown_parser.cpp
    core/markdown_parser.h
    core/markdown_table.cpp
    core/markdown_table.h
    core/math_url.h
    core/persistence.cpp
    core/persistence.h
    core/piece_table.cpp
    core/piece_table.h
    core/selection.cpp
    core/selection.h
    core/transcript_search.cpp
    core/transcript_search.h
)
target_include_directories(be_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(be_core PUBLIC Qt6::Core Qt6::Gui Qt6::Sql md4qt::md4qt)

# --- QML module: controllers + block delegates ------------------------------
qt_add_qml_module(da_blockeditor
    STATIC
    URI DaemonApp.BlockEditor
    VERSION 1.0
    SOURCES
        app/active_block_text_controller.cpp
        app/active_block_text_controller.h
        app/active_markdown_highlighter.cpp
        app/active_markdown_highlighter.h
        app/block_model.cpp
        app/block_model.h
        app/cached_image_provider.cpp
        app/cached_image_provider.h
        app/diagram_controller.cpp
        app/diagram_controller.h
        app/diagram_item.cpp
        app/diagram_item.h
        app/editor_controller.cpp
        app/editor_controller.h
        app/image_cache.cpp
        app/image_cache.h
        app/math_image_provider.cpp
        app/math_image_provider.h
    QML_FILES
        qml/components/BlockDelegate.qml
        qml/components/TableBlock.qml
        qml/components/MermaidBlock.qml
        qml/components/ImageBlock.qml
        qml/components/MathBlock.qml
        qml/components/CodeBlock.qml
        qml/components/ReasoningBlock.qml
        qml/components/ToolCallBlock.qml
        qml/components/ContentBlock.qml
        qml/components/GeneratedImageBlock.qml
        qml/components/ClarifyBlock.qml
        qml/components/ToolApprovalBar.qml
        qml/components/Lightbox.qml
        qml/components/MessageHeader.qml
        qml/components/DirectiveChips.qml
        qml/components/AssistantFooter.qml
        qml/components/SystemMessage.qml
        qml/components/ProcessNotice.qml
        qml/components/AnsiText.qml
        qml/components/DiffBlock.qml
        qml/components/SearchResultsList.qml
        qml/components/EditorSurface.qml
)
add_library(daemon-app::blockeditor ALIAS da_blockeditor)

# Module root resolves the "app/..","core/..","diagram/.." includes used by the
# sources; app/ is also needed so the generated QML type registration can find
# the QML_ELEMENT headers (EditorController, BlockModel, ...) by basename.
target_include_directories(da_blockeditor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(da_blockeditor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/app)

# CodeBlock.qml imports the external org.kde.syntaxhighlighting module; point
# qmllint at the build-tree dir where KSyntaxHighlighting emits its QML plugin.
set_property(TARGET da_blockeditor APPEND PROPERTY QT_QML_IMPORT_PATH "${CMAKE_BINARY_DIR}/qml")
target_link_libraries(da_blockeditor
    PRIVATE
        Qt6::Quick
        Qt6::Network
        Qt6::Gui
        be_core
        be_diagram
        daemon-app::theme
        daemon-app::settings
        daemon-app::controls
        # MicroTeX LaTeX math renderer (tex::LaTeX / tex::Graphics2D_qt), used by
        # the MathImageProvider to rasterize formulas into QImages.
        LaTeX
)
